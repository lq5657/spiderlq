#include "qURL.h"
#include "qThread.h"
#include "qSocket.h"
#include "qHttpRespond.h"
#include "qThreadpool.h"
#include "qCommon.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include <vector>
#include <string>
using namespace std;

int g_epfd;

int maxDeepth;                    /* the deepth of crawling */

vector<string> vKwords;           /* kwyword split with ',' */
       
vector<string> vSeeds;            /* seed split with ',' */

void* sendRequest_registerEpoll(void* none)
{
    struct epoll_event ev;
    int ret = 0;
    int sockfd;
    parseURL_t* pUrl;
    while(1){
        pUrl = pop_parseURL_queue();
        if(pUrl == NULL){
            printf("Pop parseURL_queue fail!\n");
            continue;
        }
        /* unblock connect remote ip, and get sockfd */
        sockfd = unblock_connect(pUrl->ip, pUrl->port, 10);
        if(sockfd == -1){
            continue;
        }
        /* send the page request of the remote host */
        if((ret = send_request(sockfd, pUrl)) < 0){
            printf("Send request failed: %s%s%s\n", pUrl->domain, "/", pUrl->path);
            continue;
        }
        
        epoll_arg_t* arg = (epoll_arg_t*)calloc(1, sizeof(epoll_arg_t));
        arg->fd = sockfd;
        arg->pUrl = pUrl;
        ev.data.ptr = arg;
        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        if(epoll_ctl(g_epfd, EPOLL_CTL_ADD, sockfd, &ev) == 0){
            printf("add an epoll event success: %s%s%s\n", pUrl->domain, "/", pUrl->path);
        } else{
            printf("add an epoll event failed: %s%s%s\n", pUrl->domain, "/", pUrl->path);
            continue;
        }
    }
    return NULL;
}

int main()
{
    /* load the config file, and init the vKwords and vSeeds */
    //do 
    load_conf("mySpider.conf");

    int nSeeds = vSeeds.size();
    for(int i = 0; i < nSeeds; ++i){
        char *pSeed = strdup(vSeeds[i].c_str());                /* be free in url_normalized */
        sourURL_t* sUrl = (sourURL_t*)malloc(sizeof(sourURL_t));
        sUrl->url = url_normalized(pSeed);
        sUrl->level = 0;
        sUrl->type = TYPE_HTML;
        if(sUrl->url != NULL)
            push_sourURL_queue(sUrl);
    }
    
    struct epoll_event events[10];

    /* create a thread for dns parsing , parse the node in sourURL_queue and push into parseURL_queue */
    int err = -1;
    if((err = qCreateThread(sourURLparser, NULL, NULL, NULL)) < 0){
        printf("Create sURL_parser thread fail: %s\n", strerror(err));
    }

    /* waiting seed be parsed, and ready in parseURL_queue */
    int try_num = 1;
    while(try_num < 8 && is_parseURLqueue_empty())
        usleep((10000 << try_num++));
    if(try_num >= 8){
        printf("No parsed URL! DNS parse error?\n");
    }
    
    /* begin create epoll to run */
    g_epfd = epoll_create(1);
    /* create a thread for send request to server, and register the connect sockfd to epoll */
    if((err = qCreateThread(sendRequest_registerEpoll, NULL, NULL, NULL)) < 0){
        printf("Create send request and register thread fail: %s\n", strerror(err));
    }
    
    /* create thread pool */
    qThreadpool<qHttpRespond>* pool = NULL;
    try{
        pool = new qThreadpool<qHttpRespond>;
    } catch(...) {
        return 1;
    }

    /* epoll wait */
    int n, i;
    while(1){
        n = epoll_wait(g_epfd, events, 10, 2000);
        if(n == -1){
            printf("epoll errno:%s\n", strerror(errno));
        }
        fflush(stdout);

        if(n <= 0){
            if(is_parseURLqueue_empty() && is_sourURLqueue_empty()){
                sleep(1);
                if(is_parseURLqueue_empty() && is_sourURLqueue_empty()){
                    break;
                }
            } 
        }

        for(i = 0; i < n; ++i){
            epoll_arg_t* arg = (epoll_arg_t*)(events[i].data.ptr);
            if( ((events[i].events & EPOLLERR) ||
               (events[i].events & EPOLLHUP) ||
            (!(events[i].events & EPOLLIN))) ){
                printf("epoll fail, close socket %d\n", arg->fd);

                close(arg->fd);
                free(arg->pUrl);
                free(arg);

                continue;
            }
        
            /* threadpoll's threads begin deal the EPOLLIN event. first, create a request for worker */
            qHttpRespond *qtRequest = new qHttpRespond(arg->fd, arg->pUrl);
            free(arg);                                                     //free the epoll_arg_t memory alloc in sendRequest_registerEpoll
            /* put the qtRequest into the work queue in pool, and the pool's threads will deal it*/
            pool->append(qtRequest);
        }
    }

    return 0;
}

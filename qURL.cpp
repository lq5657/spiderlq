#include "qURL.h"
#include "qBloomfilter.h"

#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>
#include <event2/util.h>
#include <event2/dns.h>

#include <queue>
#include <map>
#include <string>
using namespace std;

static queue<sourURL_t*> sourURL_queue;
static queue<parseURL_t*> parseURL_queue;
static map<string, string> host_ip_map;

static struct event_base *base = NULL;
//static int n_pending_requests = 0;

static void dns_callback(int result, struct evutil_addrinfo *res, void *arg);
static void get_timespec(timespec * ts, int millisecond);
static int dns_getaddrinfo(parseURL_t* pUrl);

pthread_mutex_t parseQ_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sourQ_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t parseQ_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t sourQ_cond = PTHREAD_COND_INITIALIZER;

void push_sourURL_queue(sourURL_t* sUrl)
{
    if(sUrl != NULL){
        pthread_mutex_lock(&sourQ_lock);

        sourURL_queue.push(sUrl);
        if(sourURL_queue.size() == 1)
            pthread_cond_signal(&sourQ_cond);

        pthread_mutex_unlock(&sourQ_lock);
    }
}

sourURL_t* pop_sourURL_queue()
{
    sourURL_t* sUrl = NULL;
    pthread_mutex_lock(&sourQ_lock);
    while(sourURL_queue.empty()){
        pthread_cond_wait(&sourQ_cond, &sourQ_lock);
    }
    sUrl = sourURL_queue.front();
    sourURL_queue.pop();
    pthread_mutex_unlock(&sourQ_lock);
    return sUrl;
}

void push_parseURL_queue(parseURL_t* pUrl)
{
    if(pUrl != NULL){
        pthread_mutex_lock(&parseQ_lock);

        parseURL_queue.push(pUrl);
        if(parseURL_queue.size() == 1)
            pthread_cond_broadcast(&parseQ_cond);

        pthread_mutex_unlock(&parseQ_lock);
    }
}

parseURL_t* pop_parseURL_queue()
{
    parseURL_t* pUrl = NULL;
    pthread_mutex_lock(&parseQ_lock);
    if(!parseURL_queue.empty()){
        pUrl = parseURL_queue.front();
        parseURL_queue.pop();
        pthread_mutex_unlock(&parseQ_lock);
        return pUrl;
    } else {
        int trynum = 3;
        struct timespec timeout;
        while(trynum-- && parseURL_queue.empty()){
           get_timespec(&timeout, 500);
           pthread_cond_timedwait(&parseQ_cond, &parseQ_lock, &timeout);
        }
        if(!parseURL_queue.empty()){
            pUrl = parseURL_queue.front();
            parseURL_queue.pop();
        }
        pthread_mutex_unlock(&parseQ_lock);
        return pUrl;
    }
}

static void get_timespec(timespec * ts, int millisecond)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    ts->tv_sec = now.tv_sec;
    ts->tv_nsec = now.tv_usec * 1000;
    ts->tv_nsec += millisecond * 1000;
}

int is_sourURLqueue_empty()
{
    int ret = 0;
    pthread_mutex_lock(&sourQ_lock);
    ret = sourURL_queue.empty();
    pthread_mutex_unlock(&sourQ_lock);
    return ret;
}

int is_parseURLqueue_empty()
{
    int ret = 0;
    pthread_mutex_lock(&parseQ_lock);
    ret = parseURL_queue.empty();
    pthread_mutex_unlock(&parseQ_lock);
    return ret;
}

int get_sourURLqueue_size()
{
    int ret = 0;
    pthread_mutex_lock(&sourQ_lock);
    ret = sourURL_queue.size();
    pthread_mutex_unlock(&sourQ_lock);
    return ret;
}

int get_parseURLqueue_size()
{
    int ret = 0;
    pthread_mutex_lock(&parseQ_lock);
    ret = parseURL_queue.size();
    pthread_mutex_unlock(&parseQ_lock);
    return ret;
}

char* url_normalized(char* url)
{
    if(url == NULL)
        return NULL;
    int len = strlen(url);
    
    while(len && isspace(url[len--])) ;
    url[len] = '\0';
    if(len == 0){
        free(url);
        return NULL;
    }

    /*remove http(s):// */
    if(len > 7 && strncmp(url, "http", 4) == 0){
        int subLen = 7;
        if(url[4] == 's')
            ++subLen;
        len -= subLen;
        char *tmp = (char*)malloc(len + 1);
        strncpy(tmp, url+subLen, len);
        tmp[len] = '\0';
        free(url);
        url = tmp;
    }

    /* remove '/' at end of url if have */
    if(url[len - 1] == '/')
        url[--len] = '\0';

    if(len > MAX_LINK_LEN){
        free(url);
        return NULL;
    }

    return url;
}

static int dns_getaddrinfo(parseURL_t* pUrl)
{
    struct evdns_base *dnsbase;
    base = event_base_new();
    if(!base)
        return 1;
    dnsbase = evdns_base_new(base, 1);
    if(!dnsbase)
       return 2;
    struct evutil_addrinfo hints;
    struct evdns_getaddrinfo_request *req;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = EVUTIL_AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    req = evdns_getaddrinfo(dnsbase, pUrl->domain, NULL, &hints, dns_callback, pUrl);
    if(req == NULL)
       printf(" [request for %s returned immediately]\n", pUrl->domain);
    event_base_dispatch(base);

    evdns_base_free(dnsbase, 0);
    event_base_free(base);
    return 0;
}

void* sourURLparser(void* none)
{
    map<string, string>::const_iterator iter;
    sourURL_t* sUrl = NULL;
    parseURL_t* pUrl = NULL;
    while(1){
        sUrl = pop_sourURL_queue();

        pUrl = (parseURL_t*)calloc(1, sizeof(parseURL_t));
        
        char *p = strchr(sUrl->url, '/');
        if(p == NULL){                            //not found '/' in url
            pUrl->domain = sUrl->url;
            pUrl->path = sUrl->url + strlen(sUrl->url);
        } else {
            *p = '\0';
            pUrl->domain = sUrl->url;
            pUrl->path = p + 1;
        }

        p = strrchr(pUrl->domain, ':');
        if(p != NULL){
            *p = '\0';
            pUrl->port = atoi(p+1);
            if(pUrl->port == 0)
                pUrl->port = 80;
        } else {
            pUrl->port = 80;
        }

        pUrl->level = sUrl->level;

        /* for pUrl->ip */
        iter = host_ip_map.find(pUrl->domain);
        if(iter == host_ip_map.end()){
            /*libevent dns resolve*/
            if(dns_getaddrinfo(pUrl))
                return NULL;
        } else {
            pUrl->ip = strdup(iter->second.c_str());           //Notice the strdup alloc memory for pUrl->ip, so remmber free pUrl->ip when don't use it
            push_parseURL_queue(pUrl);
        }
    }
    return NULL;
}

static void dns_callback(int result, struct evutil_addrinfo *res, void *arg)
{
    parseURL_t* pUrl = (parseURL_t*)arg;
    if(result)
        printf("%s -> %s\n", pUrl->domain, evutil_gai_strerror(result));
    else {
        struct evutil_addrinfo *ai;
        if(res->ai_canonname){
            printf(" [%s]", res->ai_canonname);
        }
        puts(" ");
        for(ai = res; ai; ai = ai->ai_next){
            char buf[128];
            memset(buf, 0, 128);
            if(ai->ai_family == AF_INET){
                struct sockaddr_in *sin = (sockaddr_in*)ai->ai_addr;
                if(sin != NULL){
                    evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);
                    pUrl->ip = strdup(buf);
                    push_parseURL_queue(pUrl);
                    break;
                }
            }
        }
        evutil_freeaddrinfo(res);
    }
    event_base_loopexit(base, NULL);
}

void free_pUrl(parseURL_t* pUrl)
{
    free(pUrl->domain);
    free(pUrl->ip);
    free(pUrl);
}

static const char * BIN_SUFFIXES = ".jpg.jpeg.gif.png.ico.bmp.swf";
int is_bin_url(char *url)
{
    char *p = NULL;
    if ((p = strrchr(url, '.')) != NULL) {
        if (strstr(BIN_SUFFIXES, p) == NULL)
            return 0;
        else
            return 1;
    }
    return 0;
}

char * attach_domain(char *url, const char *domain)
{
    if (url == NULL)
        return NULL;

    if (strncmp(url, "http", 4) == 0) {
        return url;

    } else if (*url == '/') {
        int i;
        int ulen = strlen(url);
        int dlen = strlen(domain);
        char *tmp = (char *)malloc(ulen+dlen+1);
        for (i = 0; i < dlen; i++)
            tmp[i] = domain[i];
        for (i = 0; i < ulen; i++)
            tmp[i+dlen] = url[i];
        tmp[ulen+dlen] = '\0';
        free(url);
        return tmp;

    } else {
        //do nothing
        free(url);
        return NULL;
    }
}

int is_crawled(char * url) 
{
    return bloomfilter_search(url); /* use bloom filter algorithm */
}


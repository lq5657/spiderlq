#include "qHttpRespond.h"
#include "qString.h"
#include "qPage.h"

#include <exception>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <errno.h>


#define HTML_MAXLEN   512 * 1024

extern int maxDeepth;

qHttpRespond::qHttpRespond(int fd, parseURL_t* pUrl) : isNormalHtml_(true),
    header_(NULL), pageHtml_(NULL), phLen_(0), sockfd_(fd), pUrl_(pUrl) 
{
    if(fd == -1 || pUrl == NULL){
        throw std::exception();
    }
    pageHtml_ = new char[HTML_MAXLEN];
    if(!pageHtml_){
        throw std::exception();
    }
}

qHttpRespond::~qHttpRespond()
{
    if(header_ != NULL){
        if(header_->content_type != NULL)
            free(header_->content_type);
        free(header_);
    }
    if(pageHtml_ != NULL)
        delete [] pageHtml_;
    if(pUrl_ != NULL)
        free_pUrl(pUrl_);
    
    close(sockfd_);
}

int qHttpRespond::process()
{
    recv_respond();

    if(isNormalHtml_ && pUrl_->level <= maxDeepth){
        /* extract the url */
        /* the page is need ? yes, store it */
        parse_htmlpage();
    }
    return 0;
}

void qHttpRespond::recv_respond()
{
    /* receive the respond html text */
    //do it
    int n, isCompleteHead = 0, len = 0;
    char *htmlPtr = NULL;
    while(1){
        n = read(sockfd_, pageHtml_+len, 1024);
        if(n < 0){
            if(errno == EAGAIN || errno == EINTR){
                /* EAGAIN error is blocking, and EINTR error is interupt by signature, just read again is ok */
                usleep(100000);
                continue;
            }
            printf("Read socket fail: %s", strerror(errno));
            break;
        } else if(n == 0){
            /* finish read respond html */
            this->phLen_ = len;
            break;
        } else {
            /* read n bytes html */
            len += n;
            this->pageHtml_[len] = '\0';

            if(!isCompleteHead) {
                if((htmlPtr = strstr(pageHtml_, "\r\n\r\n"))!= NULL){     //if read already complete respond head, then parse it
                    isCompleteHead = 1;
                    *(htmlPtr+2) = '\0';
                    char *headerPtr = (char*)malloc(strlen(pageHtml_) + 1); 
                    strcpy(headerPtr, pageHtml_);
                    this->header_ = parse_respondHeader(headerPtr);
                    free(headerPtr);
                    *(htmlPtr+2) = '\r';
                    /* check the header is ok */
                    if(check_header()){
                        continue;
                    } else {
                        isNormalHtml_ = false;
                        break;
                    }
                }
            }
        }
    }
}

Header* qHttpRespond::parse_respondHeader(char* headerPtr)
{
    int c = 0;
    char *p = NULL;
    char **spt = NULL;
    char *start = headerPtr;
    Header *h = (struct Header*)calloc(1, sizeof(Header));
    /* parse the status line in header */ 
    if((p = strstr(start, "\r\n")) != NULL) {
        *p = '\0';
        spt = strsplit(start, ' ', &c, 2);
        if(c == 3) {
            h->status_code = atoi(spt[1]);
        } else{
            h->status_code = 600;
        }
        /* free the memory in strplit function for spt */
        if(spt != NULL){
            free(spt);
            spt = NULL;
        }

        start = p + 2;
    }
    /* parse the respond header lines */
    while((p = strstr(start, "\r\n")) != NULL){
        *p = '\0';
        spt = strsplit(start, ':', &c, 1);
        if(c == 2){
            if(strcasecmp(spt[0], "content_type") == 0){
                h->content_type = strdup(strim(spt[1]));
            }
        }
        if(spt != NULL){
            free(spt);
            spt = NULL;
        }

        start = p + 2;
    }
    return h;
}

bool qHttpRespond::check_header()
{
    /* skip if not 2xx */
    if (header_->status_code < 200 || header_->status_code >= 300)
        return false;
    if (header_->content_type != NULL) {
        if (strstr(header_->content_type, "text/html") != NULL)
            return true;
        return false;
    }
    return true;
}
/* 
 * extract the url, and find the keywords in the htmltext, if find, store the html
 */
void qHttpRespond::parse_htmlpage()
{
   page_parse(pageHtml_, pUrl_); 
}



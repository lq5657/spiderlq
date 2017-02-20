#ifndef _L_Q_URL_H_
#define _L_Q_URL_H_

#define MAX_LINK_LEN 128

#define TYPE_HTML  0
#define TYPE_IMAGE 1

/*
 * #sourURL_t be the node in the sourURL_queue, will be parsed by the dns.
 * @url nomaled url,the "http://" and the last '/' of url are removed
 * @level be relative to the main page of the domain
 * @type be the page's type , for .html or .jsp or .asp or .image 
 */
struct sourURL_t{
    char* url;
    int level;
    int type;
};

/*
 * #parseURL_t be the node in the parseURL_queue, already be parsed by the dns.
 * @domain be the host domain of the url
 * @path be the path of the page in the host
 * @port be 80
 * @level be relative to the main page of the domain
 */
struct parseURL_t{
    char* domain;
    char* path;
    char* ip;
    int port;
    int level;
};

/*
 * #epoll_arg_t be the node in the worker_queue, the threads get it, and receive the page html.
 * @fd be the connect's sockfd
 * @pUrl be the info of the qequest page.
 */
struct epoll_arg_t{
    int fd;
    parseURL_t* pUrl;
};

extern void push_sourURL_queue(sourURL_t* sUrl);
extern sourURL_t* pop_sourURL_queue();
extern void push_parseURL_queue(parseURL_t* pUrl);
extern parseURL_t* pop_parseURL_queue();
extern void* sourURLparser(void* none);
extern char* url_normalized(char* url);
extern int is_sourURLqueue_empty();
extern int is_parseURLqueue_empty();
extern int get_sourURLqueue_size();
extern int get_parseURLqueue_size();
extern int isCrawled(char* url);
extern void free_pUrl(parseURL_t* pUrl);
extern int is_bin_url(char *url);
extern int is_crawled(char * url); 
extern char * attach_domain(char *url, const char *domain);

#endif

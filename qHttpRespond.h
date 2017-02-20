#ifndef _Q_HTTP_RESPOND_H_
#define _Q_HTTP_RESPOND_H_
#include "qURL.h"

struct Header{
    char *content_type;
    int status_code;
};

class qHttpRespond
{
public:
    qHttpRespond(int fd, struct parseURL_t* pUrl);
    ~qHttpRespond();
    int process();
private:
    void recv_respond();
    Header* parse_respondHeader(char* headerPtr);
    void parse_htmlpage();
    bool check_header();
private:
    bool isNormalHtml_;           /* is normal html text, default is true */

    Header *header_;              /* respond htmltext's head */
    char *pageHtml_;              /* respond htmltext */
    int phLen_;                  /* repond htmltext's length */

    int sockfd_;                   /* this connection's sockfd */
    struct parseURL_t* pUrl_;     /* respond page's url information */
};

#endif /*_Q_HTTP_RESPOND_H_*/

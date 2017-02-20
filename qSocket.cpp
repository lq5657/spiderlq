#include "qSocket.h"
#include "qURL.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/*  
 * @ip connect the remote ip address
 * @port the remote address port
 * @time the overtime of the select
 * return 0 is success, and -1 is failed
 */
int unblock_connect(const char* ip, int port, int time)
{
    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fdopt = setnonblocking(sockfd);
    ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == 0){
        /* if connect success, then return immediately */
        printf("connect with server immediately\n");
        return sockfd;
    } else if(errno != EINPROGRESS){
        /* if not connect immediately, then only the errno is EINPROGRESS, that is connecting, or is error and return */ 
        printf("unblock connect not support\n");
        return -1;
    }
    /* the errno is EINPROGRESS */
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &writefds);
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    ret = select(sockfd+1, NULL, &writefds, NULL, &timeout);
    if(ret <= 0){
        /* select overtime or error, return immediately */
        printf("connection time out\n");
        close(sockfd);
        return -1;
    }

    if(!FD_ISSET(sockfd, &writefds)){
        printf("no events on sockfd found for select!\n");
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    /* invoke getsockopt obtain and clear the error of sockfd */
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0){
        printf("get socket option failed!\n");
        close(sockfd);
        return -1;
    }
    /* if error is not 0, then the connect is error */
    if(error != 0){
        printf("connection failed after select with the error: %d\n", error);
        close(sockfd);
        return -1;
    }
    /* the connection is success */
    printf("connection ready after select with the socket: %d \n", sockfd);
    return sockfd;
}

int send_request(int fd, void* arg)
{
    int remanderData, offset, n;
    parseURL_t* pUrl = (parseURL_t*)arg;
    char request[1024] = {0};

    sprintf(request, "GET /%s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Accept: */*\r\n"
            "Connection: Keep-Alive\r\n"
            "User-Agent: Mozilla/37.0.1 (compatible; mySpider/1.0;)\r\n"
            "Referer: %s\r\n\r\n", pUrl->path, pUrl->domain, pUrl->domain);
    remanderData = strlen(request);
    offset = 0;
    while(remanderData) {
        n = write(fd, request+offset, remanderData);
        if(n <= 0){
            if(errno == EAGAIN){  //write buffer full, block
                usleep(1000);
                continue;
            }
            close(fd);
            return -1;
        }
        offset += n;
        remanderData -= n;
    }
    return 0;
}

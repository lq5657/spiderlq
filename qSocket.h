#ifndef _Q_SOCKET_H_
#define _Q_SOCKET_H_

extern int setnonblocking(int fd);
extern int unblock_connect(const char* ip, int port, int time);
extern int send_request(int fd, void* arg);

#endif /* _Q_SOCKET_H_ */

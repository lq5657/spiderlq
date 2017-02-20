#ifndef _Q_THREAD_POLL_H_
#define _Q_THREAD_POLL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "qSynchronize.h"

/* thread poll template class */
template<typename T>
class qThreadpoll
{
public:
    qThreadpoll(int threadNum_ = 5, int maxRequests_ = 10000);
    ~qThreadpoll();
    bool append(T* request);

private:
    static void* worker(void* arg);
    void run();

private:
    int threadNum_;                   /* the thread numbers in thread poll */
    int maxRequests_;                 /* the max requests in request queue */
    pthread_t* threads_;              /* the threads attray, the size is threadNum_ */
    std::list<T*> workQueue_;        /* the request queue */
    qLock queueLocker_;               /* the mutex for protect request queue */
    qSem queueStat_;                  /* the request queue stat, is there task need to deal */
    bool isStop_;                     /* is stop the thread ? */
};

#endif

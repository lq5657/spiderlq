#ifndef _Q_THREAD_POOL_H_
#define _Q_THREAD_POOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "qSynchronize.h"

/* thread poll template class */
template<typename T>
class qThreadpool
{
public:
    qThreadpool(int threadNum = 2, int maxRequests = 10000);
    ~qThreadpool();
    bool append(T* request);

private:
    static void* worker(void* arg);
    void run();

private:
    int threadNum_;                   /* the thread numbers in thread pool */
    int maxRequests_;                 /* the max requests in request queue */
    pthread_t* threads_;              /* the threads attray, the size is threadNum_ */
    std::list<T*> workQueue_;        /* the request queue */
    qLock queueLocker_;               /* the mutex for protect request queue */
    qSem queueStat_;                  /* the request queue stat, is there task need to deal */
    bool isStop_;                     /* is stop the thread ? */
};

template<typename T>
qThreadpool<T>::qThreadpool(int threadNum, int maxRequests) :
    threadNum_(threadNum), maxRequests_(maxRequests), isStop_(false), threads_(NULL)
{
    if((threadNum <=  0) || (maxRequests <= 0)){
        throw std::exception();
    }

    threads_ = new pthread_t[threadNum_];
    if(!threads_){
        throw std::exception();
    }

    // create threadNum_ threads, and set them destached state
    for(int i = 0; i < threadNum_; ++i){
        printf("create the %dth thread\n", i);
        if(qCreateThread(worker, this, threads_+i, NULL) != 0){
            delete [] threads_;
            throw std::exception();
        }
    }
}

template<typename T>
qThreadpool<T>::~qThreadpool()
{
    delete [] threads_;
    isStop_ = true;
}

template<typename T>
bool qThreadpool<T>::append(T* request)
{
    queueLocker_.lock();
    if(workQueue_.size() > maxRequests_){
        queueLocker_.unlock();
        return false;
    }
    workQueue_.push_back(request);
    queueLocker_.unlock();
    queueStat_.post();
    return true;
}

template<typename T>
void* qThreadpool<T>::worker(void* arg)
{
    qThreadpool* tdPoll = (qThreadpool*)arg;
    tdPoll->run();
    return tdPoll;
}

template<typename T> 
void qThreadpool<T>::run()
{
    while(!isStop_){
        queueStat_.wait();
        queueLocker_.lock();
        if(workQueue_.empty()){
            queueLocker_.unlock();
            continue;
        }
        T* request = workQueue_.front();
        workQueue_.pop_front();
        queueLocker_.unlock();
        if(!request)
            continue;
        request->process();
    }
}

#endif /*_Q_THREAD_POOL_H_*/

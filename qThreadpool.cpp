#include "qThreadpool.h"
#include "qThread.h"

#include <cstdio>

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

    /* create threadNum_ threads, and set them destached state */
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

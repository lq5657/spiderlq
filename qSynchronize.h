#ifndef _Q_SYNCHRONIZE_H_
#define _Q_SYNCHRONIZE_H_

#include <exception>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

/* the class of encapsulate semaphore */
class qSem
{
public:
    qSem(){
        if(sem_init(&sem_, 0, 0) != 0){
            throw std::exception();
        }
    }
    ~qSem(){
        sem_destroy(&sem_);
    }

    bool wait(){
        return sem_wait(&sem_) == 0;
    }

    bool post(){
        return sem_post(&sem_) == 0;
    }
private:
    sem_t sem_;
};

/* the class of encapsulate mutex */
class qLock
{
public:
    qLock(){
        if(pthread_mutex_init(&mutex_, NULL) != 0){
            throw std::exception();
        }
    }

    ~qLock(){
        pthread_mutex_destroy(&mutex_);
    }

    bool lock(){
        return pthread_mutex_lock(&mutex_) == 0;
    }

    int try_lock(){
        return pthread_mutex_trylock(&mutex_);
    }

    bool unlock(){
        return pthread_mutex_unlock(&mutex_) == 0;
    }

private:
    pthread_mutex_t mutex_;
};

/* the class of encapusate cond */
class qCond
{
public:
    qCond(){
        if(pthread_mutex_init(&mutex_, NULL) != 0){
            throw std::exception();
        }
        if(pthread_cond_init(&cond_, NULL) != 0){
            pthread_mutex_destroy(&mutex_);
            throw std::exception();
        }
    }

    ~qCond(){
        pthread_cond_destroy(&cond_);
        pthread_mutex_destroy(&mutex_);
    }

    bool wait(){
        int ret = 0;
        pthread_mutex_lock(&mutex_);
        ret = pthread_cond_wait(&cond_, &mutex_);
        pthread_mutex_unlock(&mutex_);
        return ret == 0;
    }

    bool timed_wait(int millisecond){
        struct timespec expiration;
        struct timeval now;
        gettimeofday(&now, NULL);
        expiration.tv_sec = now.tv_sec;
        expiration.tv_nsec = now.tv_usec * 1000;
        expiration.tv_nsec += millisecond * 1000;
        return pthread_cond_timedwait(&cond_, &mutex_, &expiration) == 0;
    }

    bool signal(){
        return pthread_cond_signal(&cond_) == 0;
    }

    bool broadcast(){
        return pthread_cond_broadcast(&cond_) == 0;
    }
private:
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
};

#endif /* _Q_SYNCHRONIZE_H_ */

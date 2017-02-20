#ifndef _Q_THREAD_H_
#define _Q_THREAD_H_

#include <pthread.h>

extern int qCreateThread(void *(*start_routine) (void *), void *arg, pthread_t * thread, pthread_attr_t * pAttr);

#endif

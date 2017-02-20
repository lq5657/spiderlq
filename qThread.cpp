#include "qThread.h"

int qCreateThread(void *(*start_routine) (void *), void *arg, pthread_t * pid, pthread_attr_t * pAttr)
{
    pthread_attr_t attr;
    pthread_t pdt;
    if(pAttr == NULL){
        pAttr = &attr;
        pthread_attr_init(pAttr);
        pthread_attr_setstacksize(pAttr, 1024 * 1024);
        pthread_attr_setdetachstate(pAttr, PTHREAD_CREATE_DETACHED);
    }

    if(pid == NULL)
        pid = &pdt;

    int ret = pthread_create(pid, pAttr, start_routine, arg);
    pthread_attr_destroy(pAttr);
    return ret;
}

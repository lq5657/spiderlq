#ifndef _Q_BLOOM_FILTER_H_
#define _Q_BLOOM_FILTER_H_

#include <unistd.h>
#include <string.h>

/* hash functions */
extern unsigned int times33(char *str);
extern unsigned int timesnum(char *str, int num);
extern unsigned int aphash(char *str);
extern unsigned int hash16777619(char *str);
extern unsigned int mysqlhash(char *str);
extern unsigned int crc32(unsigned char *buf, int len);

/* search if this url has been crawled */
extern int bloomfilter_search(char *url);

#endif

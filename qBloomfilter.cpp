#include "qBloomfilter.h"

#include <pthread.h>
#include <stdlib.h>

unsigned int times33(char *str)
{
    unsigned int val = 0;
    while (*str) 
        val = (val << 5) + val + (*str++);
    return val;
}

unsigned int timesnum(char *str, int num)
{
    unsigned int val = 0;
    while (*str) 
        val = val * num + (*str++);
    return val;
}

unsigned int aphash(char *str)
{
    unsigned int val = 0;
    int i = 0;
    for (i = 0; *str; i++) 
        if ((i & 1) == 0)
            val ^= ((val << 7)^(*str++)^(val>>3));
        else
            val ^= (~((val << 11)^(*str++)^(val>>5)));

    return (val & 0x7FFFFFFF);	
}


unsigned int hash16777619(char *str)
{
    unsigned int val = 0;
    while (*str) {
        val *= 16777619;
        val ^= (unsigned int)(*str++);
    }
    return val;
}

unsigned int mysqlhash(char *str)
{
    register unsigned int nr = 1, nr2 = 4;
    while(*str) {
        nr ^= (((nr & 63) + nr2)*((unsigned int)*str++)) + (nr << 8);
        nr2 += 3;	
    }
    return (unsigned int)nr;
}

static unsigned int   CRC32[256];
static char   init = 0;
static void init_table()
{
    int   i,j;
    unsigned int   crc;
    for(i = 0;i < 256;i++) {
        crc = i;
        for(j = 0;j < 8;j++) {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = crc >> 1;
        }
        CRC32[i] = crc;
    }
}

unsigned int crc32( unsigned char *buf, int len)
{
    unsigned int ret = 0xFFFFFFFF;
    int   i;
    if( !init ) {
        init_table();
        init = 1;
    }
    for(i = 0; i < len;i++)
        ret = CRC32[((ret & 0xFF) ^ buf[i])] ^ (ret >> 8);

    return ~ret;
}

#define HASH_FUNC_NUM 8
#define BLOOM_SIZE 1000000
#define BITSIZE_PER_BLOOM  32
#define LIMIT   (BLOOM_SIZE * BITSIZE_PER_BLOOM)

pthread_mutex_t btLock = PTHREAD_MUTEX_INITIALIZER;

/* 
 * m=10n, k=8 when e=0.01 (m is bitsize, n is inputnum, k is hash_func num, e is error rate)
 * here m = BLOOM_SIZE*BITSIZE_PER_BLOOM = 32,000,000 (bits)
 * so n = m/10 = 3,200,000 (urls)
 * enough for crawling a website
 */
static int bloom_table[BLOOM_SIZE] = {0};

static unsigned int encrypt(char *key, unsigned int id)
{
    unsigned int val = 0;

    switch(id){
        case 0:
            val = times33(key); break;
        case 1:
            val = timesnum(key,31); break;
        case 2:
            val = aphash(key); break;
        case 3:
            val = hash16777619(key); break;
        case 4:
            val = mysqlhash(key); break;
        case 5:
            //basically multithreads supported
            val = crc32((unsigned char *)key, strlen(key));
            break;	
        case 6:
            val = timesnum(key,131); break;
            /*
               int i;
               unsigned char decrypt[16];
               MD5Init(&md5);
               MD5Update(&md5, (unsigned char *)key, strlen(key));
               MD5Final(&md5, decrypt);
               for(i = 0; i < 16; i++)
               val = (val << 5) + val + decrypt[i];
               break;
               */
        case 7:
            val = timesnum(key,1313); break;
            /*
               sha1_init(&sha);  
               sha1_write(&sha, (unsigned char *)key, strlen(key));
               sha1_final(&sha);
               for (i=0; i < 20; i++)  
               val = (val << 5) + val + sha.buf[i];
               break;
               */
        default:
            // should not be here
            abort();
    }
    return val;
}


int bloomfilter_search(char *url)
{
    unsigned int h, i, index, pos;
    int res = 0;

    pthread_mutex_lock(&btLock);

    for (i = 0; i < HASH_FUNC_NUM; i++) {
        h = encrypt(url, i);
        h %= LIMIT;
        index = h / BITSIZE_PER_BLOOM;
        pos = h % BITSIZE_PER_BLOOM;
        if (bloom_table[index] & (0x80000000 >> pos))
            res++;
        else
            bloom_table[index] |= (0x80000000 >> pos);
    } 

    pthread_mutex_unlock(&btLock);

    return (res == HASH_FUNC_NUM);
}

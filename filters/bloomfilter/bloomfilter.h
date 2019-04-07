#ifndef _BLOOMFILTER_H_
#define _BLOOMFILTER_H_

#include "bitmap.h"

typedef struct _bloomfilter* bloomfilter;

#define BF_MAY_IN 1
#define BF_NOT_IN 0

bloomfilter bloomfilter_create(unsigned long capacity, double err_rate);
int bloomfilter_destroy(bloomfilter bf);
int bloomfilter_add(bloomfilter bf, char* key, unsigned int len);
int bloomfilter_test(bloomfilter bf, char* key, unsigned int len);
void bloomfilter_show(bloomfilter bf);

#endif
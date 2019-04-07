/*
 *  @Created Time : 2018-10-06 15:55:52
 *  @Description  : 
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bitmap.h"

struct _bitmap{
   unsigned char* map;
   unsigned long arrlen;
   unsigned long capacity;
};

bitmap bitmap_create(unsigned long capacity)
{
   bitmap bm;
   unsigned char* p;
   unsigned long len;
  
  if (capacity <= 0)
       return NULL;
   len = ceil((float)capacity / 8);
   p = (unsigned char *)malloc(sizeof(unsigned char)*len);
   if (p == NULL)
       return NULL;
   bm = (bitmap)malloc(sizeof(struct _bitmap));
   if (bm == NULL)
   {
       free(p);
       return NULL;
   }
   bm->map = p;
   bm->capacity = capacity;
   bm->arrlen = len;
   return bm;
}

int bitmap_destroy(bitmap bm)
{
    if(bm == NULL)
        return -1;
    free(bm->map);
    free(bm);
    return 0;
}

int bitmap_set(bitmap bm, unsigned long pos)
{
    int value;
    long rem, quo;

    if(bm==NULL)
        return -1;
    if(pos<0||pos>=bm->capacity)
        return -2;
    quo = pos / 8;
    rem = pos % 8;
    value = (bm->map[quo] & 1<<(7-rem))?1:0;
    bm->map[quo] |= 1<<(7-rem);
    if(value)
        return BIT_ON;
    else
        return BIT_OFF;
}

int bitmap_setall(bitmap bm)
{
    if(bm==NULL)
        return -1;
    memset(bm->map,0xff,bm->arrlen);
    return 0;
}

int bitmap_clearall(bitmap bm)
{
    if(bm==NULL)
        return -1;
    memset(bm->map,0,bm->arrlen);
    return 0;
}

int bitmap_clear(bitmap bm, unsigned long pos)
{
    int value;
    long rem,quo;

    if(bm==NULL)
        return -1;
    if(pos<0||pos>=bm->capacity)
        return -2;
    quo = pos / 8;
    rem = pos % 8;
    value = (bm->map[quo] & 1<<(7-rem))?1:0;
    bm->map[quo] &= ~(1<<(7-rem));
    if(value)
        return BIT_ON;
    else
        return BIT_OFF;
    
}

int bitmap_test(bitmap bm, unsigned long pos)
{
    int value;
    long rem,quo;

    if(bm==NULL)
        return -1;
    if(pos<0||pos>=bm->capacity)
        return -2;
    quo = pos / 8;
    rem = pos % 8;
    value = (bm->map[quo] & 1<<(7-rem))?1:0;
    if(value)
        return BIT_ON;
    else
        return BIT_OFF;
}

int bitmap_capacity(bitmap bm)
{
    if(bm==NULL)
        return -1;
    return bm->capacity;
}
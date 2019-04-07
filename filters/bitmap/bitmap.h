#ifndef _BITMAP_H_
#define _BITMAP_H_

#define BIT_ON 1
#define BIT_OFF 0

typedef struct _bitmap* bitmap;

bitmap bitmap_create(unsigned long capacity);
int bitmap_destroy(bitmap bm);
int bitmap_set(bitmap bm, unsigned long pos);
int bitmap_clear(bitmap bm, unsigned long pos);
int bitmap_setall(bitmap bm);
int bitmap_clearall(bitmap bm);
int bitmap_test(bitmap bm, unsigned long pos);
int bitmap_capacity(bitmap bm);

#endif
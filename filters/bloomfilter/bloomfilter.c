#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "murmur3.h"
#include "bitmap.h"
#include "bloomfilter.h"

struct _bloomfilter
{
    unsigned long capacity;
    double err_rate;
    unsigned long total_bits;
    unsigned long func_numbers;
    unsigned long size;
    bitmap bm;
    int *hashes;
};

static void _do_hashes(bloomfilter bf, const void *key, int len);

bloomfilter bloomfilter_create(unsigned long capacity, double err_rate)
{
    bitmap bm;
    bloomfilter bf;
    int k, m;

    if (capacity <= 0 || err_rate <= 0 || err_rate >= 1)
        return NULL;
    k = (int)ceil(-1 * log(err_rate) / log(2));
    m = (int)ceil(-1 * log(err_rate) * capacity / pow(log(2), 2));
    bm = bitmap_create(m);
    if (bm == NULL)
        return NULL;
    bitmap_clearall(bm);
    bf = (bloomfilter)malloc(sizeof(struct _bloomfilter));
    if (bf == NULL)
        return NULL;
    bf->hashes = (int *)malloc(sizeof(int) * k);
    if (bf->hashes == NULL)
    {
        free(bf);
        return NULL;
    }
    bf->capacity = capacity;
    bf->err_rate = err_rate;
    bf->total_bits = m;
    bf->func_numbers = k;
    bf->size = 0;
    bf->bm = bm;
    return bf;
}

int bloomfilter_destroy(bloomfilter bf)
{
    if (bf == NULL)
        return -1;
    if (bf->bm)
        free(bf->bm);
    if (bf->hashes)
        free(bf->hashes);
    free(bf);
    return 0;
}

static void _do_hashes(bloomfilter bf, const void *key, int len)
{
    int i;
    uint32_t out[4];

    MurmurHash3_x64_128(key, len, 0, out);
    for (i = 0; i < bf->func_numbers; i++)
        bf->hashes[i] = (out[0] + i * out[1] + i * i) % bf->total_bits;
}

int bloomfilter_add(bloomfilter bf, char *key, unsigned int len)
{
    int i;

    if (bf->size >= bf->capacity)
        return -1;
    bf->size++;
    _do_hashes(bf, key, len);
    for (i = 0; i < bf->func_numbers; i++)
        bitmap_set(bf->bm, bf->hashes[i]);
    return 0;
}

int bloomfilter_test(bloomfilter bf, char *key, unsigned int len)
{
    int i, f = 1;

    _do_hashes(bf, key, len);
    for (i = 0; i < bf->func_numbers; i++)
    {
        if (bitmap_set(bf->bm, bf->hashes[i]) == BIT_OFF)
            f = 0;
    }

    if (f)
        return BF_MAY_IN;
    else
        return BF_NOT_IN;
}

void bloomfilter_show(bloomfilter bf)
{
    int i;
    printf("----------\n");
    printf("capacity: %ld\n", bf->capacity);
    printf("err_rate: %f\n", bf->err_rate);
    printf("total_bits: %ld\n", bf->total_bits);
    printf("func_numbers: %ld\n", bf->func_numbers);
    printf("size: %ld\n", bf->size);
    printf("hashes for last operate: ");
    for (i = 0; i < bf->func_numbers; i++)
        printf("%d ", bf->hashes[i]);
    printf("\n");
    printf("----------\n");
}
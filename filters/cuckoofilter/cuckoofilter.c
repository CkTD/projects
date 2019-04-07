#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "cuckoofilter.h"
#include "murmur3.h"
#include "bitmap.h"

static void _do_hashes(cuckoofilter cf, const void *key, int len);
static uint32_t _getFingerprint(cuckoofilter cf, uint32_t bucket_no, uint32_t entry_no);
static void _setFingerprint(cuckoofilter cf, uint32_t bucket_no, uint32_t entry_no, uint32_t val);
static int _findEmptyEntry(cuckoofilter cf, uint32_t bucket_no);

struct _cuckoofilter
{
    unsigned long fingerprint_length; // fingerprint length in bits
    unsigned long buckets_number;     //  total buckets number is 2^buckets_number
    unsigned long entries_per_bucket;
    unsigned long max_kicks; // max number of single insertion relocates (full condition)
    unsigned long size;      // number of items in filter
    unsigned long entries_capacity;

    uint32_t *filter;
    bitmap flags;
    uint32_t h1, h2, fingerprint;

    unsigned long statistic;   // total insert times
    unsigned long kicks_times; // total kicks times
};

cuckoofilter cf_create(unsigned long fingerprint_length, unsigned long buckets_number, unsigned long entries_per_bucket, unsigned long max_kicks)
{
    unsigned long u32_needed;
    cuckoofilter cf;

    cf = (cuckoofilter)malloc(sizeof(struct _cuckoofilter));
    if (cf == NULL)
        return NULL;
    if (fingerprint_length)
        cf->fingerprint_length = fingerprint_length;
    else
        cf->fingerprint_length = CF_FPL_DEF;
    if (buckets_number)
        cf->buckets_number = buckets_number;
    else
        cf->buckets_number = CF_BKTN_DEF;
    if (entries_per_bucket)
        cf->entries_per_bucket = entries_per_bucket;
    else
        cf->entries_per_bucket = CF_EPB_DEF;
    if (max_kicks)
        cf->max_kicks = max_kicks;
    else
        cf->max_kicks = CF_MAX_KICKS_DEF;

    cf->entries_capacity = (1 << cf->buckets_number) * cf->entries_per_bucket * cf->fingerprint_length;

    cf->flags = bitmap_create(cf->entries_capacity);
    if (cf->flags == NULL)
    {
        free(cf);
        return NULL;
    }
    bitmap_clearall(cf->flags);

    u32_needed = ceil((double)cf->entries_capacity / 32);
    cf->filter = (uint32_t *)malloc(sizeof(uint32_t) * u32_needed);
    if (cf->filter == NULL)
    {
        bitmap_destroy(cf->flags);
        free(cf);
        return NULL;
    }
    printf("%.2fMB allocated for the filter\n", (double)u32_needed * 4 / 1024 / 1024);
    memset(cf->filter, 0, u32_needed);
    cf->size = 0;
    cf->statistic = 0;
    cf->kicks_times = 0;
    return cf;
}

void cf_destroy(cuckoofilter cf)
{
    if (cf == NULL)
        return;
    if (cf->filter)
        free(cf->filter);
    if (cf->flags)
        bitmap_destroy(cf->flags);
    free(cf);
    return;
}

void cf_show(cuckoofilter cf)
{
    unsigned long total_bucket;
    unsigned long entries_capacity;

    total_bucket = pow(2, cf->buckets_number);
    entries_capacity = total_bucket * cf->entries_per_bucket;
    printf("------------\n");
    printf("fingerprint length: %ld\n", cf->fingerprint_length);
    printf("total bucket number: %ld\n", total_bucket);
    printf("entries per bucket: %ld\n", cf->entries_per_bucket);
    printf("entries capacity: %ld\n", entries_capacity);
    printf("max kicks: %ld\n", cf->max_kicks);
    printf("current entries: %ld\n", cf->size);
    printf("load factor: %f\n", (double)cf->size / entries_capacity);
    printf("average kick times %f\n", cf->statistic ? (double)cf->kicks_times / cf->statistic : 0);
    printf("------------\n");
}

static uint32_t _getFingerprint(cuckoofilter cf, uint32_t bucket_no, uint32_t entry_no)
{
    unsigned long begin_bit, rem, quo;
    uint64_t tmp;

    begin_bit = (cf->entries_per_bucket * bucket_no + entry_no) * cf->fingerprint_length;

    quo = begin_bit >> 5;
    rem = begin_bit & 0x1f;

    memcpy(((uint32_t *)&tmp) + 1, cf->filter + quo, 4);
    memcpy(&tmp, cf->filter + quo + 1, 4);
    tmp = tmp >> (64 - rem - cf->fingerprint_length);

    return (uint32_t)(tmp & ((uint64_t)0xffffffffffffffff >> (64 - cf->fingerprint_length)));
}

static void _setFingerprint(cuckoofilter cf, uint32_t bucket_no, uint32_t entry_no, uint32_t val)
{
    unsigned long begin_bit, rem, quo;
    uint64_t tmp, zeromask;

    begin_bit = (cf->entries_per_bucket * bucket_no + entry_no) * cf->fingerprint_length;

    quo = begin_bit >> 5;
    rem = begin_bit & 0x1f;

    // little endian
    memcpy(((uint32_t *)&tmp) + 1, cf->filter + quo, 4);
    memcpy(&tmp, cf->filter + quo + 1, 4);
    zeromask = (0xffffffffffffffff >> (64 - cf->fingerprint_length)) << (64 - rem - cf->fingerprint_length);
    zeromask = ~zeromask;
    tmp = tmp & zeromask;
    tmp = tmp | (uint64_t)val << (64 - rem - cf->fingerprint_length);
    memcpy(cf->filter + quo, ((uint32_t *)&tmp) + 1, 4);
    memcpy(cf->filter + quo + 1, &tmp, 4);
}

/*
 * find empty entry in a bucket
 * return -1 if full else the first empty entry_no
 */
#define CF_BKT_FULL -1
static int _findEmptyEntry(cuckoofilter cf, uint32_t bucket_no)
{
    unsigned long i, base;

    base = cf->entries_per_bucket * bucket_no;
    for (i = 0; i < cf->entries_per_bucket; i++)
        if (bitmap_test(cf->flags, base + i) == BIT_OFF)
            return i;
    return CF_BKT_FULL;
}

// get fingerprint(f), two candidate buckets(h1, h2) for the key
// h1 = hash(key)
// h2 = h1 xor hash(f)
static void _do_hashes(cuckoofilter cf, const void *key, int len)
{
    uint32_t out[4], hashf;

    MurmurHash3_x64_128(key, len, 0, out);
    cf->fingerprint = out[0] >> (32 - cf->fingerprint_length);
    cf->h1 = out[1] >> (32 - cf->buckets_number);

    MurmurHash3_x64_128(&cf->fingerprint, sizeof(uint32_t), 0, out);
    hashf = out[0] >> (32 - cf->buckets_number);
    cf->h2 = cf->h1 ^ hashf;
}

int cf_insert(cuckoofilter cf, const void *key, int len)
{
    uint32_t h, e, i, tmpf;
    uint32_t h1, h2, f, hashf, out[4];

    _do_hashes(cf, key, len);
    h1 = cf->h1;
    h2 = cf->h2;
    f = cf->fingerprint;

    e = _findEmptyEntry(cf, h1);
    if (e != CF_BKT_FULL)
    {
        cf->statistic++;
        cf->size++;
        _setFingerprint(cf, h1, e, f);
        bitmap_set(cf->flags, h1 * cf->entries_per_bucket + e);
        return CF_OK;
    }
    e = _findEmptyEntry(cf, h2);
    if (e != CF_BKT_FULL)
    {
        cf->statistic++;
        cf->size++;
        _setFingerprint(cf, h2, e, f);
        bitmap_set(cf->flags, h2 * cf->entries_per_bucket + e);
        return CF_OK;
    }

    // must relocate existing items
    // randomly pick h1 or h2
    h = (rand() % 2) ? h1 : h2;
    for (i = 0; i < cf->max_kicks; i++)
    {
        cf->kicks_times++;
        // randomly select an entry e from bucket[h]
        e = rand() % cf->entries_per_bucket;
        // swap f and the fingerprint stroed in entry e
        tmpf = _getFingerprint(cf, h, e);
        _setFingerprint(cf, h, e, f);
        f = tmpf;
        // h = h xor hash(f)
        MurmurHash3_x64_128(&f, sizeof(uint32_t), 0, out);
        hashf = out[0] >> (32 - cf->buckets_number);
        h = h ^ hashf;

        e = _findEmptyEntry(cf, h);
        if (e != CF_BKT_FULL)
        {
            cf->statistic++;
            cf->size++;
            _setFingerprint(cf, h, e, f);
            bitmap_set(cf->flags, h * cf->entries_per_bucket + e);
            return CF_OK;
        }
    }
    // Hashtable is considered full
    return CF_FULL;
}

int cf_lookup(cuckoofilter cf, const void *key, int len)
{
    uint32_t i;

    _do_hashes(cf, key, len);
    for (i = 0; i < cf->entries_per_bucket; i++)
    {
        if ((bitmap_test(cf->flags, cf->entries_per_bucket * cf->h1 + i) == BIT_ON) && (_getFingerprint(cf, cf->h1, i) == cf->fingerprint))
            return CF_MAY_IN;
        if ((bitmap_test(cf->flags, cf->entries_per_bucket * cf->h2 + i) == BIT_ON) && (_getFingerprint(cf, cf->h2, i) == cf->fingerprint))
            return CF_MAY_IN;
    }
    return CF_NOT_IN;
}

int cf_delete(cuckoofilter cf, const void *key, int len)
{
    uint32_t i;

    _do_hashes(cf, key, len);
    for (i = 0; i < cf->entries_per_bucket; i++)
    {
        if (_getFingerprint(cf, cf->h1, i) == cf->fingerprint)
        {
            bitmap_clear(cf->flags, cf->h1 * cf->entries_per_bucket + i);
            cf->size--;
            return CF_OK;
        }
        if (_getFingerprint(cf, cf->h2, i) == cf->fingerprint)
        {
            bitmap_clear(cf->flags, cf->h2 * cf->entries_per_bucket + i);
            cf->size--;
            return CF_OK;
        }
    }
    return CF_NOT_EXIST;
}
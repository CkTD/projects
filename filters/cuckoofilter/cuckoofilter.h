#ifndef _CUCKOOFILTER_H_
#define _CUCKOOFILTER_H_

// number of entries per buckets
#define CF_EPB_4 4
#define CF_EPB_8 8
#define CF_EPB_DEF CF_EPB_4

// fingerprint length in bits
#define CF_FPL_4 4
#define CF_FPL_6 6
#define CF_FPL_8 8
#define CF_FPL_DEF CF_FPL_6

// bucket numbers
#define CF_BKTN_DEF 20   //  (2^20)

// max kicks
#define CF_MAX_KICKS_DEF 500


// return value
#define CF_OK 0
#define CF_FULL 1
#define CF_MAY_IN 2
#define CF_NOT_IN 3
#define CF_NOT_EXIST 4


typedef struct _cuckoofilter* cuckoofilter;

// return NULL if something wrong
cuckoofilter cf_create(unsigned long fingerprint_length, unsigned long buckets_number, unsigned long entries_per_bucket, unsigned long max_kicks);

void cf_destroy(cuckoofilter cf);

// print status
void cf_show(cuckoofilter cf);

// return CF_FULL if single insertion relocations reach max_kicks else CF_OK
int cf_insert(cuckoofilter cf, const void *key, int len);

// CF_MAY_IN or CF_NOT_IN
int cf_lookup(cuckoofilter cf, const void *key, int len);

// CF_OK or CF_NOT_EXIST
int cf_delete(cuckoofilter cf, const void *key, int len);

#endif
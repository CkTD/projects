#include "cuckoofilter.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main()
{
    int retv;
    cuckoofilter cf;
    uint32_t buf;

    cf = cf_create(CF_FPL_6,20 , CF_EPB_8, 100000);
    if (cf != NULL)
        printf("Cuckoofilter created\n");
    else
    {
        printf("Can't create cuckoofilter\n");
        return -1;
    }

    cf_show(cf);

    printf("Insert 'Hello'...");
    retv = cf_insert(cf, "Hello", 5);
    if (retv == CF_OK)
        printf("OK\n");
    else
        printf("FULL\n");

    printf("Lookup 'Hello'...");
    retv = cf_lookup(cf, "Hello", 5);
    if (retv == CF_MAY_IN)
        printf("MAY IN\n");
    else
        printf("NOT IN\n");

    printf("Lookup 'byby'...");
    retv = cf_lookup(cf, "byby", 4);
    if (retv == CF_MAY_IN)
        printf("MAY IN\n");
    else
        printf("NOT IN\n");

    printf("Delete 'Hello'...");
    retv = cf_delete(cf, "Hello", 5);
    if (retv == CF_NOT_EXIST)
        printf("NOT EXIST!\n");
    else
        printf("OK\n");

    printf("Lookup 'Hello'...");
    retv = cf_lookup(cf, "Hello", 5);
    if (retv == CF_MAY_IN)
        printf("MAY IN\n");
    else
        printf("NOT IN\n");

    printf("Delete 'byby'...");
    retv = cf_delete(cf, "byby", 4);
    if (retv == CF_NOT_EXIST)
        printf("NOT EXIST!\n");
    else
        printf("OK\n");

    printf("Insert random entries unitl full...\n");
    for (retv = CF_OK; retv != CF_FULL;)
    {
        buf = rand();
        retv = cf_insert(cf, &buf, 4);
    }
    printf("DONE\n");
    cf_show(cf);

    cf_destroy(cf);

    return 0;
}
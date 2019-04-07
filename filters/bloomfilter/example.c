#include "bloomfilter.h"
#include <stdio.h>

int main()
{
    bloomfilter bf;

    bf = bloomfilter_create(10000, 0.0001);
    if (bf == NULL)
        return 1;

    bloomfilter_show(bf);
    printf("Add \"Hello\"\n");
    bloomfilter_add(bf, "Hello", 5);
    printf("Test \"Hello\"\n");
    if (bloomfilter_test(bf, "Hello", 5) == BF_MAY_IN)
        printf("MAY IN\n");
    else
        printf("NOT IN\n");
    printf("Test \"Hdello\"\n");
    if (bloomfilter_test(bf, "Hdello", 6) == BF_MAY_IN)
        printf("MAY IN\n");
    else
        printf("NOT IN\n");
    bloomfilter_show(bf);
    bloomfilter_destroy(bf);
    return 0;
}
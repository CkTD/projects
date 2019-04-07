#include <stdio.h>

static void memprint(const void* ptr, int c)
{
    char *p = (char*)ptr;
    int i;
    for (i = 0; i < c; i++)
    {
        printf("%d %p:  0x%2hhX\n",i,p+i , *(p+i));
    }
}

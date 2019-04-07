/*
 *  @Created Time : 2018-10-06 16:41:45
 *  @Description  : 
 */
#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"

int main()
{
    int i;
    bitmap ba;

    ba = bitmap_create(44);
    if(ba==NULL)
        return -1;
    printf("capacity: %d",bitmap_capacity(ba));

    printf("\nSet All\n");
    bitmap_setall(ba);
    for(i=0;i<44;i++)
        printf("%d:%d\t",i,bitmap_test(ba,i));
    
    printf("\nClear All");
    bitmap_clearall(ba);
    printf("\nSet %%3=0\n");
    for(i=0;i<44;i++)
        if(i%3==0)
            bitmap_set(ba,i);
    
    for(i=0;i<44;i++)
        printf("%d: %d\t",i,bitmap_test(ba,i));
    
    printf("\nClear %%6=0\n");
    for(i=0;i<44;i++)
        if(i%6==0)
            bitmap_clear(ba,i);
    
    for(i=0;i<44;i++)
    printf("%d: %d\t",i,bitmap_test(ba,i));

    bitmap_destroy(ba);

    return 0;
}

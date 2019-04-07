#include "list.h"


/* status */
#define RUN    0
#define READY  1
#define BLOCK  2

struct PCB {
    char pid[16];
    int priority;
    int status;

    struct list_head *parent;  /* my parent */
    struct list_head children; /* list of my children */
    struct list_head sibling;  /* linkage in my parent's children */
    
    struct list_head run_list; /* linkage in status list */

    int occupied_rs[4];        /* resource I occupied */ 
    int require_rs[4];         /* resource I am requiring */
};


struct RCB {
    char name[3];
    int  total;
    int  available;
    struct list_head *waiting;  
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "process.h"

void ps_create(char *name, int priority);
void ps_delete(char *name, int do_sche);
void timeout();
void scheduler();
void ps_tree(struct PCB *pcb);
void rs_request(int type, int count);
void rs_release(int type, int count);
void rs_check(int type);

/* three ready list */
struct list_head ready[3] = {
    LIST_HEAD_INIT(ready[0]),
    LIST_HEAD_INIT(ready[1]),
    LIST_HEAD_INIT(ready[2])};

/* four block list */
struct list_head block[4] = {
    LIST_HEAD_INIT(block[0]),
    LIST_HEAD_INIT(block[1]),
    LIST_HEAD_INIT(block[2]),
    LIST_HEAD_INIT(block[3]),
};

/* current running process */
struct PCB *current = NULL;

/* the init process */
struct PCB init = {
    .pid = "init",
    .priority = 0,
    .status = READY,

    .parent = NULL,
    .children = LIST_HEAD_INIT(init.children),
    // sibling is not used in init,
    // because init don't have a parent!

    .run_list = NULL,

    // not wait for any resource
    .occupied_rs = {0, 0, 0, 0},
    .require_rs = {0, 0, 0, 0}
};

/* 4 resources */
struct RCB rs[4] = {
    {.name = "R1",
     .total = 1,
     .available = 1,
     .waiting = &block[0]},
    {.name = "R2",
     .total = 2,
     .available = 2,
     .waiting = &block[1]},
    {.name = "R3",
     .total = 3,
     .available = 3,
     .waiting = &block[2]},
    {.name = "R4",
     .total = 4,
     .available = 4,
     .waiting = &block[3]}
};

void do_init()
{
    /* add init to ready0 list */
    list_add(&(init.run_list), &ready[0]);

    /* call the scheduler */
    scheduler();
}

int main()
{
    do_init();
    char cmd[128];
    char *p, *arg1, *arg2;
    int num, num2;

    printf(">>> ");
    fflush(stdout);

    while (fgets(cmd, 128, stdin) != NULL)
    {
        p = strtok(cmd, " \t\n");
        if (p == NULL)
            goto err;

        if ((arg1 = strtok(NULL, " \t\n")) == NULL)
            arg2 = NULL;
        else
            arg2 = strtok(NULL, " \t\n");

        if (strcmp(cmd, "cr") == 0) {

            if ((!arg1) || (!arg2) || (!(num = atoi(arg2))))
                goto err;
            ps_create(arg1, num);
        }
        else if (strcmp(cmd, "dl") == 0) {
            if (!arg1)
                goto err;
            ps_delete(arg1, 1);
        }
        else if (strcmp(cmd, "to") == 0) {
            timeout();
        }
        else if (strcmp(cmd, "req") == 0) {
            if ((!arg1) || (!arg2) || (!(num = atoi(arg1 + 1)))\
                        || (!(num2 = atoi(arg2))))
                goto err;
            /* R1 R2 R3 R4 having type 0 , type 1, type2, type 3 */
            rs_request(num - 1, num2);
        }
        else if (strcmp(cmd, "rel") == 0) {
            if ((!arg1) || (!arg2) || (!(num = atoi(arg1 + 1)))\
                        || (!(num2 = atoi(arg2))))
                goto err;
            /* R1 R2 R3 R4 having type 0 , type 1, type2, type 3 */
            rs_release(num - 1, num2);
        }
        else if (strcmp(cmd, "ps") == 0) {
            ps_tree(&init);
        }
        else if (strcmp(cmd, "quit") == 0)
            break;
        else
            goto err;
        printf(">>> ");
        fflush(stdout);
        continue;
err:
        fprintf(stderr, "Bad Command!\n");
        fprintf(stderr, "Useage:cr|dl|req|rel|to|ps|quit "
                        "[arg1] [arg2]\n");
        printf(">>> ");
        fflush(stdout);
    }
    return 0;
}

void ps_create(char *name, int priority)
{
    struct PCB *pcb;
    int i;

    /* get a new PCB */
    pcb = malloc(sizeof(struct PCB));

    /* set my pid */
    strcpy(pcb->pid, name);
    /* set my priority */
    pcb->priority = priority;
    /* I am ready to run */
    pcb->status = READY;

    /* set my parent to current*/
    pcb->parent = &(current->children);
    /* I have no chile now */
    INIT_LIST_HEAD(&(pcb->children));
    /* add me to my parent's children linkage */
    list_add(&(pcb->sibling), &(current->children));

    /* add me to the tail of ready list of my priority */
    list_add_tail(&(pcb->run_list), &ready[priority]);

    /* I am not waitting for any resources */
    for (i = 0; i < 4; i++) {
        pcb->occupied_rs[i] = 0;
        pcb->require_rs[i] = 0;
    }

    scheduler();
}

void ps_delete(char *name, int do_sche)
{
    struct PCB *pcb, *tp;
    struct list_head *head, *th;
    int i;

    /* try to find this process in ready list */
    for (i = 0; i < 4; i++)
        list_for_each_entry(pcb, &block[i], run_list) 
            if (strcmp(pcb->pid, name) == 0) goto find;
    /* try to find this process in block list */
    for (i = 1; i <= 2; i++)
        list_for_each_entry(pcb, &ready[i], run_list) 
            if (strcmp(pcb->pid, name) == 0) goto find;
    /* not find the process with pid=name */
    printf("Error! No process with pid [%s]", name);
    return;

find:
    printf("Process [%s] deleted\n", name);
    /* I am not the current process */
    if (current == pcb)
        current = NULL;
    /* I am not my parent's child */
    list_del(&(pcb->sibling));
    /* kill all my childs */
    for (head = (&(pcb->children))->next; \
            head != (&(pcb->children));) {
        th = head->next;
        tp = list_entry(head, struct PCB, sibling);
        ps_delete(tp->pid, 0);
        head = th;
    }
    /* realease all my resource */
    for (i = 0; i < 4; i++) {
        rs[i].available += pcb->occupied_rs[i];
        if (pcb->occupied_rs[i])
            rs_check(i);
    }
    /* remove me from my ready/block list */
    list_del(&(pcb->run_list));
    /* free mem */
    free(pcb);
    /* all done , call scheduler() */
    if (do_sche)
        scheduler();
}

void rs_request(int type, int count)
{
    if (!count)
        return;
    if (type < 0 || type > 3) {
        printf("Error, invalid resource type!\n");
        return;
    }
    if (current == &init) {
        printf("Error, init can't request any resource!\n");
        return;
    }

    if (current->occupied_rs[type] + count > rs[type].total) {
        printf("Error, process [%s] have already occupied %d" 
               "%s, Total number of %s is %d\n", current->pid,
               current->occupied_rs[type], rs[type].name,
               rs[type].name, rs[type].total);
        return;
    }

    if (rs[type].available >= count) {
        rs[type].available -= count;
        current->occupied_rs[type] += count;
        scheduler(); // do nothing but print current is still me
    }
    else {
        current->require_rs[type] += count;
        current->status = BLOCK;

        /* move me from ready list to block list*/
        list_move_tail(&(current->run_list), &(block[type]));

        printf("Resources not ready ,process %s blocked\n",
                    current->pid);

        /* do sched */
        scheduler();
    }
}

void rs_release(int type, int count)
{
    int flag;
    struct PCB *pcb;
    if (!count)
        return;
    if (type < 0 || type > 3) {
        printf("Error, invalid resource type!\n");
        return;
    }
    if (current == &init) {
        printf("Error, init can't request any resource!\n");
        return;
    }

    if (current->occupied_rs[type] < count) {
        printf("Error, you only occupy %d %s\n", 
            current->occupied_rs[type], rs[type].name);
    }
    else {
        /* release resource */
        current->occupied_rs[type] -= count;
        rs[type].available += count;

        /* check weather some process waiting for 
           this type of resource can get ready*/
        rs_check(type);
        scheduler();
    }
}

void rs_check(int type)
{
    struct PCB *pcb;
    int flag;

    do {
        flag = 0;
        list_for_each_entry(pcb, rs[type].waiting, run_list) {
            /* try to find one process can get resource */
            if (pcb->require_rs[type] <= rs[type].available) {
                flag = 1;

                pcb->occupied_rs[type] += pcb->require_rs[type];
                rs[type].available -= pcb->require_rs[type];
                pcb->require_rs[type] = 0;

                /* check if it is ready */
                if ((!pcb->require_rs[0]) && (!pcb->require_rs[1]) 
                    && (!pcb->require_rs[2]) && (!pcb->require_rs[3])) {
                    pcb->status = READY;
                    list_move_tail(&(pcb->run_list), &(ready[pcb->priority]));
                    printf("Process [%s], get resource [%s] and ready again\n",
                            pcb->pid, rs[type].name);
                }
                break;
            }
        }
    } while (flag);
}

void timeout()
{
    current->status = READY;
    list_move_tail(&(current->run_list), &ready[current->priority]);

    scheduler();
}

void scheduler()
{
    struct PCB *pcb;

    /* find a process to run */
    if (!list_empty(&ready[2]))
        pcb = list_entry(ready[2].next, struct PCB, run_list);
    else if (!list_empty(&ready[1]))
        pcb = list_entry(ready[1].next, struct PCB, run_list);
    else
        pcb = list_entry(ready[0].next, struct PCB, run_list);

    if (current == NULL || current->status != RUN)
    {
        current = pcb;
        pcb->status = RUN;
    }
    else if (pcb->priority > current->priority)
    {
        current->status = READY;
        pcb->status = RUN;
        current = pcb;
    }

    printf("scheduler:  current [%s]\n", current->pid);
}

void _ps_tree(struct PCB *pcb, int level)
{
    int i;
    struct PCB *child;
    char *status;

    switch (pcb->status) {
    case RUN:
        status = "RUNNINT";
        break;
    case BLOCK:
        status = "BLOCKED";
        break;
    case READY:
        status = "READY";
        break;
    }

    /* indent */
    for (i = 0; i < level; i++)
        printf("  ");
    /* show myself */
    printf("%s [%d|%s]  [%d,%d,%d,%d] [%d,%d,%d,%d]\n",
           pcb->pid, pcb->priority, status,
           pcb->occupied_rs[0], pcb->occupied_rs[1],
           pcb->occupied_rs[2], pcb->occupied_rs[3],
           pcb->require_rs[0], pcb->require_rs[1],
           pcb->require_rs[2], pcb->require_rs[3]);
    /* show all children */
    list_for_each_entry(child, &(pcb->children), sibling)
        _ps_tree(child, level + 1);
}

void ps_tree(struct PCB *pcb)
{
    printf("All Process:\n");
    _ps_tree(pcb, 0);
}

#ifndef __SERVER_H
#define __SERVER_H


#include <unistd.h>
#define MAX_WOKERS 32

#include "ae.h"
#include "anet.h"
#include "list.h"
#include "hiredis/hiredis.h"

typedef struct {
    int     pid;                     /* pid of current process */  
    int     master_pid;             /* server pid */
    int     port;                   /* listen port */
    int     lsfd;                   /* listen port fd */
    char*   daemon;                 /* daemon mode */
    int     log_level;              /* log level */
    char*   pid_file;               /* pid file for daemon mode */
    char*   log_file;               /* log file for daemon mode */

    int     wokers_n;               /* number of wokers */
    int     wokers_pid[MAX_WOKERS]; /* list of wokers pid */
    
    int     redis_pid;  
    int     redis_port;             /* redis-server listen port */
    char*   redis_log;              /* redis-server log file */


    struct list_head clients;
    int    save_per_logs;

    /* for master & wokers */
    int    total_logs_count;
    int    tmp_logs_count;
    redisContext *rc;               /* redis context */


    /* for every woker */
    int     maxclients;
    int     client_timeout;
    struct aeEventLoop* el;
    int     stat_numconnections;
    int     stat_rejected_conn;
    int     tcp_backlog;
    char*   neterr;                   /* err message */
    int     report_per_logs;

} server_t;


struct client
{
    int     fd;
    int     port;
    char    ip[16];
    int     last_active;
    struct list_head timeoutlist;
};


#ifndef __SERVER_C
extern server_t server;
#endif

#endif

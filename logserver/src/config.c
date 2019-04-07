#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "server.h"
#include "serverlog.h"

#define FILE_NAME_BUF_SIZE 64



static char pid_file[FILE_NAME_BUF_SIZE] = "/var/run/";
static char log_file[FILE_NAME_BUF_SIZE] = "/var/log/";
static char* redis_log = "/dev/null";


static const char *optString = "p:w:d:h?qv";
static const struct option longOpts[] =
    {
        {"pid-file",required_argument,NULL,0},
        {"log-file",required_argument,NULL,0},
        {"redis-port",required_argument,NULL,0},
        {"redis-log",required_argument,NULL,0},
        {"time-out",required_argument,NULL,0},
        {"save-per-logs",required_argument,NULL,0},
        {"max-clients",required_argument,NULL,0},
        {"help",no_argument,NULL,'h'},
        {NULL,0,NULL,0}
    };



void display_usage(char* name)
{
    printf("usage: %s [OPTION]... \n\n",name);
    printf("  -p PORT                   server port, default: 8888\n");
    printf("  -w WOKERS                 number of wakers\n");
    printf("  -d start/stop/restart     daemon mode\n");
    printf("\n");
    printf("  --pid-file PID_FILE       pid file for daemon mode\n");
    printf("  --log-file LOG_FILE       log file for daemon mode\n");
    printf("  --redis-log REDIS_LOG     log file for redis server, defaule: no log\n");
    printf("  --redis-port REDIS_PORT   redis server port, default: 6379\n");
    printf("  --time-out TIMEOUT        client time out\n");
    printf("  --max-clients NUMBER      number of max clients for per woker\n");
    printf("  --save-per-logs NUMBER    number of logs cached in redis\n");

    printf("\n");
    printf("  -h, --help                show this help and exit\n");
    printf("  -q, -qq                   quiet mode, only show  warnings/errors\n");
    printf("  -v                        verbose mode, more output for debug\n");
}


void display_config()
{
    printf("config:\n");
    printf("\tport: %d\n",server.port);
    printf("\twokers number: %d\n",server.wokers_n);
    printf("\tdaemon mode: %s\n",server.daemon);
    printf("\tredis port: %d\n",server.redis_port);
    printf("\tredis log: %s\n",server.redis_log);
    printf("\tlog file: %s\n",(server.daemon==NULL)?"not in daemon mode":server.log_file);
    printf("\tpid file: %s\n",(server.daemon==NULL)?"not in daemon mode":server.pid_file);
    printf("\tclient time out: %d\n",server.client_timeout);
    printf("\n");
}

int parse_config(int argc, char *argv[])
{
    strcpy(pid_file + 9, argv[0]);
    strcpy(log_file + 9, argv[0]);
    strcat(pid_file,".pid");
    strcat(log_file,".log");


    server.port = 8888;
    server.wokers_n = 1;
    memset(server.wokers_pid,0,sizeof(server.wokers_pid));  
    server.daemon = NULL;
    server.redis_port = 6379;
    server.redis_log = redis_log;
    server.pid_file = pid_file;
    server.log_file = log_file;
    server.log_level = LOG_INFO;
    server.client_timeout = 300;
    server.save_per_logs = 10000;
    server.maxclients = 1024;
    
    int opt;
    int longIndex;
    while((opt=getopt_long(argc,argv,optString,longOpts,&longIndex))!=-1)
    {
        switch(opt)
        {
            case 'p':
                server.port = atoi(optarg);
                break;
            case 'w':
                server.wokers_n = atoi(optarg);
                break;
            case 'd':
                server.daemon = optarg;
                break;
            case 'q':
                server.log_level-=1;
                break;
            case 'v':
                server.log_level+=1;
                break;
            case 'h':
            case '?':
                display_usage(argv[0]);
                exit(0);
                break;
            case 0:
                if (strcmp("time-out",longOpts[longIndex].name)==0)
                    server.client_timeout = atoi(optarg);                
                else if(strcmp("pid-file",longOpts[longIndex].name)==0)
                    strcpy(pid_file,optarg);
                else if (strcmp("log-file",longOpts[longIndex].name)==0)
                    strcpy(log_file,optarg);
                else if (strcmp("redis-port",longOpts[longIndex].name)==0)
                    server.redis_port = atoi(optarg);
                else if (strcmp("redis-log",longOpts[longIndex].name)==0)
                    server.redis_log = optarg;
                else if (strcmp("save-per-logs",longOpts[longIndex].name)==0)
                    server.save_per_logs = atoi(optarg);
                else if (strcmp("max-clients",longOpts[longIndex].name)==0)
                    server.maxclients = atoi(optarg);
                break;
            default:
                break;
        }
    }
    return 0;
}

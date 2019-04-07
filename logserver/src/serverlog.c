#define __SERVERLOG_C

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "serverlog.h"
#include "config.h"
#include "server.h"

static int old_stdout;


static const char* time_color = "\033[0;36m";
static const char* err_color = "  \033[1;31mERROR\033[0;0m\033[0;91m    ";
static const char* warning_color = "  \033[1;33mWARNING\033[0;0m\033[0;93m  ";
static const char* info_color = "  \033[1;34mINFO\033[0;0m\033[0;94m     ";
static const char* debug_color = "  \033[1;35mDEBUG\033[0;0m\033[0;95m    ";
static const char* end_color = "\033[0;0m";


int serverlog_init()
{
    /*
     * get old stdout before close
     * so we can print to screen when something wrong
    */
    old_stdout = dup(STDOUT_FILENO);
    
    int flags = fcntl(old_stdout,F_GETFD);
    if (-1 == flags)
        return-1;
    flags |= FD_CLOEXEC;
    if ( -1 == fcntl(old_stdout,F_SETFD,flags))
        return -1;
    
    if (server.daemon==NULL)
        return 0;

    close(STDIN_FILENO);
    close(STDERR_FILENO);

    
    int log_fd = open(server.log_file,O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (log_fd < 0)
    {
        return -1;
    }
    dup2(log_fd, STDOUT_FILENO);
    close(log_fd);
    return 0;
}


void serverlog_error()
{
    dup2(old_stdout,STDOUT_FILENO);
}

void serverlog_done()
{
    if(server.daemon!=NULL)
        close(old_stdout);
}

void serverlog(int priority, const char *format, ...)
{
    if(priority > server.log_level)
        return;

    time_t tm;
    time(&tm);
    char *timeStr = ctime(&tm);
    timeStr[strlen(timeStr)-1] = 0;
    printf("%s%s%s",time_color,timeStr,end_color);


    switch(priority)
    {
        case LOG_ERROR:
            printf("%s", err_color);
            break;
        case LOG_WARNING:
            printf("%s", warning_color);
            break;
        case LOG_INFO:
            printf("%s", info_color);
            break;
        case LOG_DEBUG:
            printf("%s", debug_color);
            break;


    }
    va_list args;
    va_start(args,format);
    vprintf(format,args);
    va_end(args);

    printf("%s\n", end_color);
}

#include <stdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "config.h"
#include "serverlog.h"
#include "server.h"

#define PID_BUF_SIZE 64

static void err_exit(char* err_msg);
static int daemon_start();
static int daemon_stop();
static int write_pid_file(pid_t pid);


void daemon_exec()
{
    if(server.daemon==NULL)
        return;
    else if(strcmp(server.daemon, "start")==0)
    {
        if(daemon_start()!=0)
            err_exit("\tdaemon_start error\n");
    }
    else if(strcmp(server.daemon, "stop")==0)
    {
        if(daemon_stop()!=0)
            err_exit("\tdaemon_stop error\n");
        exit(0);
    }
    else if(strcmp(server.daemon, "restart")==0)
    {
        if(daemon_stop()!=0)
            err_exit("\tdaemon_stop error\n");
        if(daemon_start()!=0)
            err_exit("\tdaemon_start error\n");
    }
    else
        err_exit("damon mode should be start|stop|restart\n");
}


static void err_exit(char* err_msg)
{
    printf("%s",err_msg);
    exit(1);
}

static int write_pid_file(pid_t pid)
{
    char buf[PID_BUF_SIZE];
    int fd = open(server.pid_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (-1 == fd)
    {
        printf("%s\n",strerror(errno));
        printf("can't open!\n");
        return -1;
    }

    int flags = fcntl(fd,F_GETFD);
    if (-1 == flags)
        return-1;
    flags |= FD_CLOEXEC;
    if( -1 == fcntl(fd,F_SETFD,flags))
        return -1;

    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if ( -1 == fcntl(fd, F_SETLK, &fl))
    {
        int n = read(fd,buf,PID_BUF_SIZE-1);
        if (n>0)
        {
            buf[n] = 0;
            printf("already started at pid %ld\n",atol(buf));
        }
        else
            printf("already started\n");
        return -1;
    }

    if (-1 == ftruncate(fd,0))
        return -1;

    snprintf(buf,PID_BUF_SIZE,"%ld\n",(long)getpid());
    if ( write(fd,buf,strlen(buf))!=strlen(buf))
    {
        printf("write error!\n");
        return -1;
    }
    return 0;
}


static int daemon_start()
{
    pid_t               pid;

    /*
     * Clear file creation mask.
     */
    umask(0);


    /*
     * Become a session leader to lose controlling TTY.
     */
    if((pid=fork())<0)
        err_exit("can't fork");
    else if(pid!=0)
        exit(0);
    setsid();

    /*
     * Change the current working directory to the root so
     * we won't prevent file systems from being unmounted.
     */
    if(chdir("/")<0)
    {
        printf("can't change working directory to /");
        return -1;
    }

    pid = getpid();
    if(write_pid_file(pid)!=0)
    {
        printf("can't write pid file!\n");
        return -1;
    }

    printf("daemon started\n");


    return 0;
}

static int daemon_stop()
{
    char buf[PID_BUF_SIZE];
    int i, stopped;
    int fd;
    pid_t pid;  

    fd = open(server.pid_file, O_RDONLY);
    if(fd < 0)
    {
        printf("not running\n");
        return 0;
    }

    read(fd, buf,PID_BUF_SIZE);
    close(fd);
    

    pid = (pid_t)atol(buf);

    stopped = 0;
    if (pid > 0)
    {
        for(i=0;i<200;i++)
        {
            if(-1 == kill(pid,SIGTERM))
            {
                if(errno == ESRCH)
                {
                    stopped=1;
                    break;
                }
            }
            usleep(50000);
        }
        if(!stopped)
        {
            printf("time out when stopping pid %d",pid);
            return -1;
        }
        printf("stopped.\n");

        if(0!=unlink(server.pid_file))
        {
            printf("unlink error!");
            return -1;
        }
    }
    else
    {
        printf("pid is not positive: %ld",(long)pid);
        return -1;
    }   
    
    return 0;
}

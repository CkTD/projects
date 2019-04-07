#define __SERVER_C

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "config.h"
#include "daemon.h"
#include "server.h"
#include "serverlog.h"
#include "woker.h"

#include "ae.h"
#include "anet.h"
#include "db.h"

void acceptClientHandler(struct aeEventLoop *eventLoop, int fd);
void timeoutHandler(struct aeEventLoop *eventLoop);
void clientLogHandler(struct aeEventLoop *eventLoop, int fd);
static void nice_exit(int signo);


server_t server;



void do_save()
{
    serverlog(LOG_INFO, "Master[%d]: save db to disk...", server.pid);
    save_db_and_flush();
    system("mkdir -p logs ; mv ./dump.rdb ./logs/`date +%Y%m%d_%H%M%S.rdb`");
}


static void woker_signal_handler()
{
    server.tmp_logs_count += server.report_per_logs;
    serverlog(LOG_INFO, "Master[%d]: receive woker signal, receive %d logs since start.",server.pid, server.total_logs_count + server.tmp_logs_count);
    if (server.tmp_logs_count >= server.save_per_logs)
    {
        server.total_logs_count += server.tmp_logs_count;
        server.tmp_logs_count = 0;
        do_save();
    }
}


static void master_exit_handler(int signo)
{
    char *sigStr;
    switch (signo)
    {
    case SIGTERM:
        sigStr = "SIGTERM";
        break;
    case SIGINT:
        sigStr = "SIGINT";
        break;
    case SIGQUIT:
        sigStr = "SIGQUIT";
        break;
    }
    serverlog(LOG_INFO, "Maste[%d]r received signal %s to stop",server.pid, sigStr);
    nice_exit(signo);
}

static void nice_exit(int signo)
{
    do_save();
    for (int n = 0; n < server.wokers_n; n++)
    {
        if (server.wokers_pid[n] == 0)
            continue;

        if ((kill(server.wokers_pid[n], signo)) < 0)
            continue;

        waitpid(server.wokers_pid[n], NULL, 0);
        serverlog(LOG_INFO, "woker stoped [pid: %d]", server.wokers_pid[n]);
    }

    if (server.redis_pid != 0)
    {
        kill(server.redis_pid, signo);
        waitpid(server.redis_pid, NULL, 0);
        serverlog(LOG_INFO, "redis stoped [pid: %d]", server.redis_pid);
    }
    serverlog(LOG_INFO, "logserver exit");
    exit(0);
}

int main(int argc, char *argv[])
{
    int n;
    pid_t pid;

    /*  init config  */
    parse_config(argc, argv);
    display_config();
    
    server.tcp_backlog = 32;
    server.pid = getpid();
    server.master_pid = server.pid;
    server.rc = NULL;
    server.total_logs_count = 0;
    server.tmp_logs_count = 0;
    server.report_per_logs = 1000;
    server.rc = NULL;
    INIT_LIST_HEAD(&server.clients);

    /* daemonize */
    daemon_exec();

    /* inti serverlog */
    if (serverlog_init() != 0)
    {
        serverlog(LOG_ERROR, "error! can't init log");
        exit(1);
    }

    /* run redis */
    pid = fork();
    if (pid == 0)
    {
        // child
        int fd = open(server.redis_log, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        char port[8];
        sprintf(port, "%d", server.redis_port);
        execlp("redis-server", "redis-server", "./redis.conf", "--port", port, NULL);

        // execlp failed
        serverlog_error();
        serverlog(LOG_ERROR, "can't start redis  %s",
                  (errno == ENOENT) ? "redis-server not installed" : strerror(errno));
        exit(1);
    }
    else if (pid > 0)
    {
        sleep(1);
        if (waitpid(pid, NULL, WNOHANG) == pid)
        {
            serverlog_error();
            serverlog(LOG_ERROR, "redis error, server will exit. check previous message or redis log file");
            exit(1);
        }
        serverlog(LOG_INFO, "redis started [pid: %d]", pid);
        server.redis_pid = pid;
    }

    /* listen to port */
    server.lsfd = anetTcpServer(server.neterr, server.port, NULL, server.tcp_backlog);
    anetNonBlock(NULL, server.lsfd);
    if (server.lsfd == ANET_ERR)
    {
        serverlog(LOG_ERROR, "can't listen to port. %s", server.neterr);
        exit(1);
    }


    /* create workers */
    for (n = 0; n < server.wokers_n; n++)
    {
        pid = fork();
        if (pid == 0)
        {
            server.pid = getpid();
            server.stat_numconnections = 0;
            server.stat_rejected_conn = 0;
            run_server(); // never return
            exit(1);
        }
        else if (pid > 0)
        {
            server.wokers_pid[n] = pid;
            serverlog(LOG_INFO, "woker started [pid: %d]", pid);
        }
    }

    serverlog_done();

    serverlog(LOG_INFO, "logserver started pid=%d  daemon mode=%s",
              server.pid, (server.daemon == NULL ? "FALSE" : "TRUE"));

    signal(SIGTERM, master_exit_handler);
    signal(SIGINT, master_exit_handler);
    signal(SIGQUIT, master_exit_handler);
    signal(SIGUSR1, woker_signal_handler);

    /* connect to redis server */
    init_redis();

    for (;;)
    {
        pid = wait(NULL);
        if (pid == server.redis_pid)
        {
            server.redis_pid = 0;
            serverlog(LOG_ERROR, "redis exit unexpectedly [pid :%d]", pid);
            serverlog(LOG_ERROR, "server will exit.");
            nice_exit(SIGTERM);
        }
        for (n = 0; n < server.wokers_n; n++)
        {
            if (pid == server.wokers_pid[n])
            {
                server.wokers_pid[n] = 0;
                serverlog(LOG_WARNING, "woker exit unexpectedly [pid: %d]", pid);
                break;
            }
        }
    }
}

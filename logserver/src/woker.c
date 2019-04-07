#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

#include "server.h"
#include "serverlog.h"

#include "anet.h"
#include "ae.h"
#include "db.h"

#include "hiredis/hiredis.h"

void periodicHandler(struct aeEventLoop *eventLoop);
void clientLogHandler(struct aeEventLoop *eventLoop, int fd, void *client);
void acceptClientHandler(struct aeEventLoop *eventLoop, int fd, void *data);
struct client* new_client(int cfd, int cport, char* cip);
void debug_show_clients();
void destory_client(struct aeEventLoop *eventLoop, struct client* client);


void run_server()
{
    /* set up event loop */   
    server.el = aeCreateEventLoop(server.maxclients, 5, periodicHandler);
    if (server.el == NULL)
    {
        serverlog(LOG_ERROR, "Woker[%d]: Can't create eventloop", server.pid);
        exit(1);
    }

    if (aeCreateFileEvent(server.el, server.lsfd, AE_READABLE, acceptClientHandler, NULL) == AE_ERR)
    {
        serverlog(LOG_ERROR, "Woker[%d]: Can't add listen socket to epoll!", server.pid);
        exit(1);
    }

    /* connect to redis server */
    init_redis();

    /* run mainloop */
    aeMain(server.el);
    exit(0);
}



#define MAX_ACCEPTS_PER_CALL 32
void acceptClientHandler(struct aeEventLoop *eventLoop, int fd, void* data)
{
    int max = MAX_ACCEPTS_PER_CALL;
    int cfd;
    int cport;
    char cip[16];

    while (max--)
    {
        cfd = anetTcpAccept(server.neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR)
        {
            if (errno == EMFILE)
            {
                serverlog(LOG_WARNING, "Woker[%d] Accept: %s, stop accept for %ds",strerror(errno), eventLoop->interval);
                aeDeleteFileEvent(eventLoop, server.lsfd, AE_READABLE);
            }
            else if (errno != EWOULDBLOCK)
                serverlog(LOG_WARNING, "Woker[%d] Accepting client connections: %s",server.pid, strerror(errno));
            else
                return;
        }

        if (cfd >= server.maxclients)
        {
            close(cfd);

            serverlog(LOG_WARNING, "Woker rejected client, max number of clients reached");
            server.stat_rejected_conn += 1;
            return;
        }
        serverlog(LOG_INFO, "\tWoker[%d] Accepted %s:%d fd=%d",server.pid, cip, cport, cfd);
        anetNonBlock(NULL, cfd);

        struct client* client = new_client(cfd,cport,cip);
        if (aeCreateFileEvent(eventLoop, cfd, AE_READABLE, clientLogHandler, client)==AE_ERR)
        {
            serverlog(LOG_WARNING, "Woker can't creat event for client %s:%d", cip, cport);
            close(cfd);
        }
        server.stat_numconnections += 1;
    }
    return;
}

struct client* new_client(int cfd, int cport, char* cip)
{
    struct client* client = malloc(sizeof(struct client));
    client->fd = cfd;
    client->port = cport;
    strcpy(client->ip,cip);
    client->last_active = time(NULL);
    list_add_tail(&client->timeoutlist, &server.clients);
    return client;
}

void del_client(struct client* client)
{
    free((struct client*)client);
}

void destory_client(struct aeEventLoop *eventLoop, struct client* client)
{
    server.stat_numconnections -= 1;
    close(client->fd);
    aeDeleteFileEvent(eventLoop, client->fd, AE_READABLE);
    list_del(&(((struct client*)client)->timeoutlist));
    del_client(client);
}


void clientLogHandler(struct aeEventLoop *eventLoop, int fd, void *client)
{
    char buf[4096];
    int n = read(fd,buf,4096);
    if (n < 0)
    {
        serverlog(LOG_WARNING,"\tWoker[%d] read error %s", server.pid, strerror(errno));
        destory_client(eventLoop, client);
    }
    else if (n == 0)
    {
        serverlog(LOG_INFO,"\tWoker[%d] client %s:%d  fd=%d close the connection", server.pid,((struct client*)client)->ip, ((struct client*)client)->port, fd);
        destory_client(eventLoop, client);
    }
    else
    {
        server.tmp_logs_count += 1;
        buf[n] = '\0';
        ((struct client*)client)->last_active = time(NULL);
        list_move_tail(&(((struct client*)client)->timeoutlist), &server.clients);
        write_to_redis(buf, n);
        serverlog(LOG_DEBUG,"\tWoker[%d] read from client address=%s:%d fd=%d datalen=%d",server.pid, ((struct client*)client)->ip, ((struct client*)client)->port, fd ,n);
    }
}



void periodicHandler(struct aeEventLoop *eventLoop)
{
    struct client* client;

    serverlog(LOG_DEBUG, "\tperiodicHandler called!");
    
    /* start accept again if [Too many fd] cause it stop */
    if(!(aeGetFileEvents(eventLoop, server.lsfd) && AE_READABLE))
    {
        aeCreateFileEvent(eventLoop, server.lsfd, AE_READABLE, acceptClientHandler, NULL);
        serverlog(LOG_INFO, "\tWoker[%d] start accept again...",server.pid);
    }

    /* show all client */   
    debug_show_clients();

    /* checkout timeout clients */
    int out = 0;
    struct list_head *tmp, *head = server.clients.next;
    while(head != &server.clients)
    {
        client = list_entry(head,struct client,timeoutlist);
        if (time(NULL) - client->last_active >= server.client_timeout)
        {
            tmp = head->next;
            serverlog(LOG_WARNING, "\tWoker[%d] client timeout address=%s:%d fd=%d", server.pid, client->ip, client->port , client->fd);
            destory_client(eventLoop, client);
            head = tmp;
            out += 1;
        }
        else
            break;
    }
    if (out)
        debug_show_clients();
        

    /* send signal to server when receive server.report_per_logs logs */

    if (server.tmp_logs_count >= server.report_per_logs)
    {
        server.total_logs_count += server.tmp_logs_count;
        server.tmp_logs_count -= server.report_per_logs;
        serverlog(LOG_DEBUG, "\tWoker[%d]: report_per_logs reach, rport to server.  Total recv: %d", server.pid, server.total_logs_count + server.tmp_logs_count);
        kill(server.master_pid, SIGUSR1);
    }

    serverlog(LOG_DEBUG, "\tperiodicHandler return!");
}

void debug_show_clients()
{
    struct client* client;
    serverlog(LOG_DEBUG, "\tall client:");
    list_for_each_entry(client, &(server.clients), timeoutlist)
    {
        serverlog(LOG_DEBUG, "\t\t address=%s:%d fd=%d",client->ip, client->port, client->fd);
    }
}
#include <stdlib.h>

#include "hiredis/hiredis.h"
#include "server.h"
#include "serverlog.h"


void init_redis()
{
    redisReply *reply;
    server.rc = redisConnect("localhost", server.redis_port);
    if (server.rc == NULL || server.rc->err) {
        if (server.rc) {
            serverlog(LOG_INFO, "PID: [%d]: Connection redis error: %s",server.pid, server.rc->errstr);
            redisFree(server.rc);
        } else {
            serverlog(LOG_INFO, "PID: [%d]: Connection redis error: can't allocate redis context", server.pid);
        }
        exit(1);
    }
    serverlog(LOG_INFO, "PID: [%d] Connect to redis success.", server.pid);
    /* PING server */
    reply = redisCommand(server.rc,"PING");
    serverlog(LOG_INFO, "PID: [%d] PING to redis: %s", server.pid ,reply->str);
    freeReplyObject(reply);
}


void write_to_redis(char* buf, int n)
{
    redisReply *reply;
    reply = redisCommand(server.rc,"rpush logs %s", buf);
    freeReplyObject(reply);
}

void save_db_and_flush()
{
    redisReply *reply;
    reply = redisCommand(server.rc,"save");
    reply = redisCommand(server.rc,"flushdb");
    freeReplyObject(reply); 
}
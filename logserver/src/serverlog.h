#ifndef __SERVERLOG_H
#define __SERVERLOG_H

#define LOG_ERROR 0
#define LOG_WARNING 1
#define LOG_INFO 2
#define LOG_DEBUG 3

#ifndef __SERVERLOG_C
extern void serverlog(int priority, const char *format, ...);
extern int serverlog_init();
extern void serverlog_done();
extern void serverlog_error();

#endif

#endif

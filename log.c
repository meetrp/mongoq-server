/*
 *  log.c
 *
 *  Customized logging related functions and definitions  
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

/* system includes */
#include <stdarg.h>             /* va_*() */
#include <stdio.h>              /* fopen(), FILE, .. */
#include <time.h>               /* time(), ctime(), .. */
#include <string.h>             /* strlen() */
#include <unistd.h>             /* getpid() */
#include <pthread.h>            /* pthread_self() */

/* our includes */
#include "common.h"

/**
 * mq_log()
 *
 * Logs the msg into the file defined, in 'common.h', by LOG_FILE.
 *
 *  log_level  - ERR, DBG, INF, etc... all of 3 chars only
 *  fname      - file name for this invocation. So each invocation can be 
 *               targetted to different files like err.log, dbg.log, etc...
 *  func       - name of the function where this was invoked
 *  line_no    - line number of the function where this was invoked
 *  fmt        - printf format specifier
 *
 **/
void mq_log(const char *log_level, const char *fname, const char *func,
            int line_no, const char *fmt, ...)
{
    va_list args;
    char msg[LOG_MAX_LEN];
    time_t timer;
    char *cur_time_str = NULL;

    FILE *fstream = fopen(LOG_FILE, "a");
    if (NULL == fstream)
        fstream = stdout;

    timer = time(NULL);
    cur_time_str = ctime(&timer);
    cur_time_str[strlen(cur_time_str) - 1] = 0;     /* '/0' terminate */

    va_start(args, fmt);
    vsnprintf(msg, LOG_MAX_LEN, fmt, args);
    va_end(args);

    fprintf(fstream,
            "%-24.24s [%-6d:%16ld] %-16.16s %-16.16s %4d : [%.3s] %s\n",
            cur_time_str, getpid(), pthread_self(), fname, func, line_no,
            log_level, msg);

    fclose(fstream);
}

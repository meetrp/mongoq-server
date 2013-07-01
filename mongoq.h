/*
 *  mongoq.h
 *
 *  The main file's header file with all the declarations, structs & typedefs
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

#ifndef _MONGOQ_H_
#define _MONGOQ_H_

#include <mongo.h>              /* mongodb related */
#include <evhttp.h>             /* evhttp.* */

/**
 * Event handler function pointer that will be passed while creating
 * a thread.
 **/
typedef void (*ev_hdlr)(struct evhttp_request *req, void *arg);

/* db related functions */
mq_err_t db_init(void);
void db_deinit(void);
mq_err_t db_push(mongo*, const char*, const char*);


mq_err_t thread_init(int, ev_hdlr, struct event_base*);

#endif /* _MONGOQ_H_ */

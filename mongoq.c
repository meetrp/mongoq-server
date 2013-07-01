/*
 *  mongoq.c
 *
 *  The main file. 
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

/* system includes */
#include <event.h>              /* libevent.* */
//#include <evhttp.h>             /* evhttp.* */
#include <signal.h>             /* SIGTERM, SIGQUIT, SIGINT */

//#include <mongo.h>              /* mongodb related */

/* our includes */
#include "common.h"
#include "config.h"
#include "mongoq.h"


/* static variables */
static struct event_base *main_base = NULL;
bool daemon_quit = false;

/**
 * event_handler()
 *
 * Called when an http event happens on the port
 *
 *  req        - http event request structure
 *  arg        - arg that was passed while setting up the event
 *
 **/
void
event_handler(struct evhttp_request *req, void *arg)
{
    struct evbuffer *buf = evbuffer_new();
    if (NULL == buf) {
        mqerr("unable to create event buffer");
        evhttp_send_reply(req, HTTP_SERVUNAVAIL, "Service unavailable", NULL);
        return;
    }
    mqdbg("event handler: %p", buf);

    evbuffer_add_printf(buf, "Requested: %s\n", evhttp_request_uri(req));
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
}


/**
 * sig_handler()
 *
 * For safe exit, the termination signals are caught and handled.
 *
 *  sig_no     - signal #
 *
 **/
static void
sig_handler(int sig_no)
{
    mqdbg("Caught signal %d", sig_no);
    if (sig_no != SIGTERM && sig_no != SIGQUIT && sig_no != SIGINT) {
        mqlog("Received an unsupported signal: %d", sig_no);
        return;
    }

    if (true == daemon_quit) {
        mqlog("Daemon is already shutting down\nYet signal{%d} is issued",
                sig_no);
        return;
    }

    daemon_quit = true;
    mqlog("Signal(%d) caught. Trying to exit gracefully...", sig_no);

    /* exit event loop first */
    mqlog("exitting event base...");
    if (0 == event_base_loopexit(main_base, 0))
        mqlog("done!");
    else
        mqerr("error!!");
}


/**
 * main()
 *
 * The main function where everything is initialized
 *
 *  argc       - # of CLI args
 *  argv       - CLI args in an array format
 *
 **/
int
main(int argc, char **argv)
{
    mq_err_t ret_code = MQ_ERR;

    /* Initialize an event base */
    //main_base = event_init();
    main_base = event_base_new();
    if (NULL == main_base) {
        mqerr("unable to initialize an event");
        ret_code = MQ_EV_INIT_FAILED;
        goto end;
    }
    mqdbg("event base: %p", main_base);

    /* register for singal callback */
    if (SIG_ERR == signal(SIGTERM, sig_handler))
        fprintf(stderr, "Cannot catch SIGTERM");
    if (SIG_ERR == signal(SIGQUIT, sig_handler))
        fprintf(stderr, "Cannot catch SIGQUIT");
    if (SIG_ERR == signal(SIGINT, sig_handler))
        fprintf(stderr, "Cannot catch SIGINT");

    /* connect to db */
    ret_code = db_init();
    if (MQ_OK != ret_code) {
        mqerr("DB init has failed: %s", MQ_ERR_STR(ret_code));
        goto db_init_failed;
    }
    mqdbg("connected to db: %d", ret_code);

    /* create, initialize 'NTHREADS' threads */
    ret_code = thread_init(MQ_NTHREADS, &event_handler, main_base);
    if (MQ_OK != ret_code) {
        mqerr("thread_init has failed: %s", MQ_ERR_STR(ret_code));
        goto thread_init_failed;
    }
    mqdbg("created thread: %d", ret_code);

db_init_failed:
end:
    mqlog("exiting with ret_code: %d", ret_code);
    return ret_code;

thread_init_failed:
    mqdbg("cleaning up db init changes");
    event_base_free(main_base);
    goto end;
}

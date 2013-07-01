/*
 *  thread.c
 *
 *  If the server is run as a multi-threaded application then all the required
 *  definitions are available here.
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

#include <pthread.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

/* our includes */
#include "common.h"
#include "config.h"
#include "mongoq.h"

/*
 * # of threads that have finished setting themselves up.
 */
//static int init_count = 0;
//static pthread_mutex_t init_lock;
//static pthread_cont_t init_cond;

typedef struct _ev_thread_t {
    pthread_t evt_pthread;
    struct evhttp *evt_httpd;
} ev_thread_t;


static mq_err_t
setnonblock(int fd)
{
    mq_err_t ret_code = MQ_ERR;
    int flags = 0;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        mqerr("get flags failed!");
        ret_code = MQ_SOCK_GET_FLAGS_FAILED;
        goto end;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        mqerr("set flags failed!");
        ret_code = MQ_SOCK_SET_FLAGS_FAILED;
        goto end;
    }

    ret_code = MQ_OK;
end:
    return ret_code;
}

static mq_err_t
create_and_bind_socket(int port, int* fd)
{
    mq_err_t ret_code = MQ_ERR;
    struct sockaddr_in server_addr;

    /* create a internet stream socket */
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        mqerr("unable to create a socket");
        ret_code = MQ_SOCK_CREATE_FAILED;
        goto end;
    }

    /* Initialize the socket address with the port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* bind the socket address with the socket stream fd */
    if (bind(listenfd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        mqerr("binding the socket to server failed");
        ret_code = MQ_SOCK_BINDING_FAILED;
        goto bind_failed;
    }

    /* listen for data on that socket */
    if (listen(listenfd, MQ_CONN_BACKLOG) < 0) {
        mqerr("failed to listen");
        ret_code = MQ_SOCK_FAILED_TO_LISTEN;
        goto listen_failed;
    }

    /* set the socket to non-blocking, this is essential in event based
     * programming with libevent
     */
    ret_code = setnonblock(listenfd);
    if (ret_code != MQ_OK) {
        mqerr("failed to set the socket non-blocking");
        goto nonblock_failed;
    }

    *fd = listenfd;
    ret_code = MQ_OK;

end:
    return ret_code;

nonblock_failed:
listen_failed:
bind_failed:
    close(listenfd);
    goto end;
}


static void*
ev_dispatcher(void *arg)
{
    event_base_dispatch((struct event_base *) arg);
    mqdbg("Dispatched an event: %p", arg);
    return NULL;
}


mq_err_t
thread_init(int nthreads, ev_hdlr handler_fn, struct event_base *ev_base)
{
    mq_err_t ret_code = MQ_ERR;
    int sock_fd = -1;
    int i = 0, j = 0, created = 0;
    ev_thread_t evt[nthreads];

    if (NULL == ev_base) {
        mqerr("a NULL event");
        ret_code = MQ_EV_INIT_FAILED;       /* re-using the libevent err */
        goto end;
    }

    mqdbg("nthreads: %d", nthreads);

    /* create & bind a scoket to a given port */
    ret_code = create_and_bind_socket(MQ_SERVER_PORT, &sock_fd);
    if (MQ_OK != ret_code) {
        mqerr("bind_socked functin failed!");
        goto socket_bind_failed;
    }
    mqdbg("created a socket @ %d - %d", MQ_SERVER_PORT, ret_code);

    for (; i < nthreads; i++) {
        /* create a new http event */
        evt[i].evt_httpd = evhttp_new(ev_base);
        if (NULL == evt[i].evt_httpd) {
            mqerr("unable to create httpd server #%d", i);
            if (i)
                mqerr("cleanup begins");
            ret_code = MQ_EV_CREATE_HTTP_SERVER_FAILED;
            goto create_http_server_failed;
        }
        mqdbg("new httpd event created: %p", evt[i].evt_httpd);

        /* bind the socket with httpd server */
        if (evhttp_accept_socket(evt[i].evt_httpd, sock_fd) != 0) {
            mqerr("unable to bind the socket with httpd server");
            mqerr("cleanup begins");
            ret_code = MQ_EV_HTTP_SOCKET_BIND_FAILED;
            evhttp_free(evt[i].evt_httpd);
            goto bind_http_with_socket_failed;
        }
        mqdbg("bound the socket with the httpd server");

        /* set a callback for the httpd server */
        evhttp_set_gencb(evt[i].evt_httpd, handler_fn, NULL);

        if (0 != pthread_create(&(evt[i].evt_pthread), NULL, &ev_dispatcher,
                                    (void *)ev_base)) {
            mqerr("unable to create thread #%d", i);
            evhttp_free(evt[i].evt_httpd);
            continue;       // continue if a thread is unable to be created.
        }

        created++;
    }

    if (created != nthreads)
        mqerr("Only %d threads created", created);
    if (created == 0)
        ret_code = MQ_THR_CREATE_FAILED;

    mqdbg("waiting for #%d threads created to close", created);
    for (i=0; i<created; i++) {
        mqdbg("waiting to close #%d", i);
        pthread_join(evt[i].evt_pthread, NULL);
    }

socket_bind_failed:
end:
    return ret_code;

bind_http_with_socket_failed:
create_http_server_failed:
    for (; j < i; j++)
        evhttp_free(evt[j].evt_httpd);
    goto end;
}

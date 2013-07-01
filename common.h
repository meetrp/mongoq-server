/*
 *  common.h
 *
 *  All the common typedefs, declarations, includes clubbed under the same
 *  header file
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/* the missing piece of definition in C */
typedef enum _bool_t {
    false = 0,
    true = 1
} bool;


/* Custom error types */
typedef enum _mq_err_t {
    MQ_ERR = -1,                /* generic error type */
    MQ_OK = 0,                  /* all okay */
    MQ_MALLOC_FAILED,           /* malloc failed */

    MQ_DB_CONNECT_FAILED,       /* mongodb connection failed */
    MQ_DB_NO_SOCKET,            /* could not create socket */
    MQ_DB_ADDR_ERROR,           /* error while calling getaddrinfo() */
    MQ_DB_NOT_MASTER,           /* Non-master node */
    MQ_DB_QNAME_TOO_LONG,       /* queue name is too long */
    MQ_DB_NAME_SPACE_INVALID,   /* name space is not valid */
    MQ_DB_INSERT_FAILED,        /* insert into mongo db failed */
    MQ_DB_IO_ERROR,             /* IO error */
    MQ_DB_RUN_COMMAND_FAILED,   /* mongo db run command failed */
    MQ_DB_SOCKET_ERROR,         /* other socket error */
    MQ_DB_READ_SIZE_ERROR,      /* response is not the expected len */
    MQ_DB_WRITE_ERROR,          /* write error */
    MQ_DB_BSON_INVALID,         /* BSON invalid for the given op */
    MQ_DB_BSON_NOT_FINISHED,    /* BSON obj has not been finished */
    MQ_DB_BSON_TOO_LARGE,       /* BSON obj exceeds max BSON size */

    MQ_EV_INIT_FAILED,                  /* event initialization failed */
    MQ_EV_CREATE_HTTP_SERVER_FAILED,    /* creation of httpd server failed */
    MQ_EV_HTTP_SOCKET_BIND_FAILED,      /* bind socket with server failed */

    MQ_SOCK_CREATE_FAILED,      /* create a new socket failed */
    MQ_SOCK_BINDING_FAILED,     /* binding of a socket failed */
    MQ_SOCK_FAILED_TO_LISTEN,   /* listen failed */
    MQ_SOCK_GET_FLAGS_FAILED,   /* get flags of socket failed */
    MQ_SOCK_SET_FLAGS_FAILED,   /* set flags to socket failed */

    MQ_THR_CREATE_FAILED        /* creating of thread failed */
    /* Remember to update the _mq_err_str defined below  */
} mq_err_t;

const char** _mq_err_str;
#define MQ_ERR_STR(err)     _mq_err_str[err+1]


/* Logging */
#define LOG_MAX_LEN             1024
//#define LOG_FILE                "/var/log/mongoq.log"
#define LOG_FILE                "/tmp/mongoq.log"

void mq_log(const char *log_level, const char *fname, const char *func,
            int line_no, const char *fmt, ...);

#define mqlog(...)              mq_log("INF", __FILE__, __FUNCTION__,   \
                                       __LINE__, __VA_ARGS__)
#define mqerr(...)              mq_log("ERR", __FILE__, __FUNCTION__,   \
                                       __LINE__, __VA_ARGS__)
#define mqwarn(...)             mq_log("WRN", __FILE__, __FUNCTION__,   \
                                       __LINE__, __VA_ARGS__)

#define MQ_DEBUG_ENABLED
#ifdef MQ_DEBUG_ENABLED
#define mqdbg(...)              mq_log("DBG", __FILE__, __FUNCTION__,   \
                                       __LINE__, __VA_ARGS__)
#else
#define mqdbg(...)
#endif

#endif /* _COMMON_H_ */

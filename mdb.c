/*
 *  mdb.c    
 *
 *  Mongo DB related functions & definitions
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

/* system includes */
#include <time.h>               /* time */
#include <string.h>             /* strlen */
#include "mongo.h"

/* our includes */
#include "common.h"
#include "config.h"


/* locally used */
#define NAME_SPC_MAX_LEN    64
#define MAX_DATA_LEN        1024


/**
 * mongo_to_mq()
 *
 * mongo err to mq error
 *
 *  mongo_err  - mongo db error number
 *
 **/
static mq_err_t
mongo_to_mq(int mongo_err)
{
    switch(mongo_err) {
        case MONGO_CONN_NO_SOCKET: return MQ_DB_NO_SOCKET;
        case MONGO_CONN_FAIL: return MQ_DB_CONNECT_FAILED;
        case MONGO_CONN_ADDR_FAIL: return MQ_DB_ADDR_ERROR;
        case MONGO_CONN_NOT_MASTER: return MQ_DB_NOT_MASTER;

        case MONGO_IO_ERROR: return MQ_DB_IO_ERROR;
        case MONGO_NS_INVALID: return MQ_DB_NAME_SPACE_INVALID;
        case MONGO_COMMAND_FAILED: return MQ_DB_RUN_COMMAND_FAILED;
        case MONGO_SOCKET_ERROR: return MQ_DB_SOCKET_ERROR;
        case MONGO_READ_SIZE_ERROR: return MQ_DB_READ_SIZE_ERROR;
        case MONGO_WRITE_ERROR: return MQ_DB_WRITE_ERROR;
        case MONGO_BSON_INVALID: return MQ_DB_BSON_INVALID;
        case MONGO_BSON_NOT_FINISHED: return MQ_DB_BSON_NOT_FINISHED;
        case MONGO_BSON_TOO_LARGE: return MQ_DB_BSON_TOO_LARGE;

        default: return MQ_ERR;
    }
}



/**
 * db_init()
 *
 * Initialize the DB, i.e., mongodb
 *
 **/
mq_err_t
db_init(void)
{
    mq_err_t ret_code = MQ_ERR;

    mongo *conn = (mongo *)malloc(sizeof(mongo));
    if (NULL == conn) {
        mqerr("malloc failed for %d bytes", sizeof(mongo));
        ret_code = MQ_MALLOC_FAILED;
        goto end;
    }

    mqdbg("About to connect to %s:%d", MONGO_SERVER_ADDR, MONGO_SERVER_PORT);
    if (MONGO_OK !=
            mongo_connect(conn, MONGO_SERVER_ADDR, MONGO_SERVER_PORT)) {
        ret_code = mongo_to_mq(conn->err);
        if (MQ_DB_NOT_MASTER == ret_code)
            /* connected to non-master (read-only) node */
            mongo_destroy(conn);

        mqerr("unable to connect to %s:%d. Error code: %s",
               MONGO_SERVER_ADDR, MONGO_SERVER_PORT, MQ_ERR_STR(ret_code));
        goto failed_to_connect;
    }

    /* verify connection */
    if (MONGO_OK != mongo_check_connection(conn)) {
        mqerr("No connection!!! **DANGER**");
        ret_code = mongo_to_mq(conn->err);
        mongo_destroy(conn);
        goto failed_to_connect;
    }

    ret_code = MQ_OK;

end:
    return ret_code;

failed_to_connect:
    mqdbg("freeing up the connnection");
    free(conn);
    goto end;
}


/**
 * db_deinit()
 *
 * De-initialize the DB, i.e., drop the connection
 *
 **/
void
db_deinit(void *args)
{
    mongo *conn = (mongo *) args;
    mongo_destroy(conn);
    free(conn);
}


/**
 * db_push()
 *
 * Push the 'val' into the queue 'qname'
 *
 *  conn       - mongo db connection object
 *  qname      - name of the queue into which data is queued
 *  val        - string formatted data to be pushed
 *
 **/
mq_err_t
db_push(mongo *conn, const char *qname, const char *val)
{
    bson b;
    mq_err_t ret_code = MQ_ERR;
    
    /* avoided malloc for perfomrance */
    char name_spc[NAME_SPC_MAX_LEN];

    /* limiting the qname with the db name for ease of programming */
    int name_spc_len = (strlen(MONGO_DB_NAME) + strlen(qname) +
                        strlen(".")) + 1;
    if (NAME_SPC_MAX_LEN <= name_spc_len) {
        mqerr("qname is too long: %s", qname);
        ret_code = MQ_DB_QNAME_TOO_LONG;
        goto end;
    }
    snprintf(name_spc, name_spc_len, "%s.%s", MONGO_DB_NAME, qname);
    name_spc[name_spc_len + 1] = 0;         /* null termination */


    /* validate the name space */
    if (MONGO_OK != mongo_validate_ns(conn, name_spc)) {
        mqerr("name space validation failed: %s", name_spc);
        ret_code = mongo_to_mq(conn->err);
        goto end;
    }
    mqdbg("Name space{%s} is valid!", name_spc);


    /* initialize the bson object with val for insertion */
    bson_init(&b);
    bson_append_int(&b, "ts", time(NULL));
    bson_append_string(&b, "val", val);
    bson_finish(&b);

    ret_code = MQ_OK;
    mqdbg("about to insert(%s) into queue(%s)", val, qname);
    if (MONGO_OK != mongo_insert(conn, name_spc, &b, NULL)) {
        mqerr("failed to insert: %s", val);
        ret_code = mongo_to_mq(conn->err);
    }

    bson_destroy(&b);

end:
    return ret_code;
}

/**
 * db_pop()
 *
 * Pop from the queue 'qname' into 'val'
 *
 *  conn       - mongo db connection object
 *  qname      - name of the queue from where data is poped.
 *  val        - string formatted data that is returned. If no data is
 *               found then '\0' is returned.
 *
 **/
mq_err_t
db_pop(mongo *conn, const char *qname, char *val)
{
    int result = -1;
    mq_err_t ret_code = MQ_ERR;
    bson cmd, out;
    bson_iterator it;


    /*
     * push the following command into a bson object:
     *   <db>.<q>.findAndModify({remove: {$pop: {$val: -1}}})
     */
    bson_init(&cmd);
    bson_append_string(&cmd, "findAndModify", qname);
        bson_append_start_object(&cmd, "remove");
            bson_append_start_object(&cmd, "$pop");
                bson_append_int(&cmd, "$val", -1);
            bson_append_finish_object(&cmd);
        bson_append_finish_object(&cmd);
    bson_finish(&cmd);

    mqdbg("about to execute the command");
    result = mongo_run_command(conn, MONGO_DB_NAME, &cmd, &out);
    if (MONGO_OK != result) {
        mqerr("run command failed.");
        ret_code = mongo_to_mq(conn->err);
        goto end;
    }

    mqdbg("mongo run command successful");
    if (bson_find(&it, &out, "value")) {
        bson_iterator it_sub;
        bson_iterator_from_buffer(&it_sub, bson_iterator_value(&it));
        while (bson_iterator_next(&it_sub)) {
            if (0 == strcmp("val", bson_iterator_key(&it_sub))) {
                const char *data = bson_iterator_string(&it_sub);
                snprintf(val, strlen(data) + 1, "%s", data);
                mqdbg("qname: %s   val: %s", qname, val);
                ret_code = MQ_OK;
                break;
            }
        }
    }
    bson_destroy(&out);

end:
    bson_destroy(&cmd);
    return ret_code;
}

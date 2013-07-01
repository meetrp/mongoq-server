/*
 *  config.h
 *
 *  Configuration params in the header file
 *
 *  Author: rp <rp@meetrp.com>
 *
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

/* MongoDB related information */
#define MONGO_SERVER_ADDR       "127.0.0.1"
#define MONGO_SERVER_PORT       27017
#define MONGO_DB_NAME           "donot-delete-mq"

/* Queue server related information */
//#define MQ_NTHREADS             8
#define MQ_NTHREADS             1
#define MQ_SERVER_PORT          5454
#define MQ_CONN_BACKLOG         64      // Max pending connections

#endif /* _CONFIG_H_ */

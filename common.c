/*
 *  common.h
 *
 *  All the common typedefs, declarations, includes clubbed under the same
 *  header file
 *
 *  Author: rp <rp@meetrp.com>
 *
 */

const char* _mq_err_str[] = {
    "Generic error",
    "Success",
    "Malloc failed",

    "Mongo DB connect failed",
    "Mongo DB socket creation failed",
    "Mongo DB address error",
    "Mongo DB server is not master",
    "Mongo DB queue name provided is too long",
    "Mongo DB name space validation failed",
    "Mongo DB insert failed",
    "Mongo DB IO error while reading or writting on a socket"
    "Mongo DB run command failed",
    "Mongo DB general socket error",
    "Mongo DB response is not the expected len",
    "Mongo DB write error",
    "Mongo DB BSON invalid for the given op",
    "Mongo DB BSON not finished",
    "Mongo DB BSON too large & exceeds max BSON size",

    "Libevent base initialization failed",
    "Libevent creation of httpd server failed",
    "Libevent bind socket with httpd server failed",

    "Socket create failed",
    "Socket binding the socket stream to server failed",
    "Socket listen on a socket stream failed",
    "Socket retrieve flags failed",
    "Socket set flags failed",

    "Thread unable to create a phthread"
};


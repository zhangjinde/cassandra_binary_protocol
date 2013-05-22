/* cql_client.h */

#ifndef _CQL_CLIENT_H_INCLUDED_
#define _CQL_CLIENT_H_INCLUDED_

/*
 * the CQL client class
 */
#define CQL_HOSTNAME_LEN    256

struct cql_client
{
    /* the hostname of the server */
    char hostname[CQL_HOSTNAME_LEN];

    /* the server port */
    int port;

    /* the socket connecting to the server */
    int s;

    /* parser for incoming data */
    struct cql_result_parser parser;
};

int cql_client_create(struct cql_client **client_out);
int cql_client_init(struct cql_client *client);
int cql_client_free(struct cql_client *client);

int cql_client_connect(struct cql_client *client, char *ip, int port);

int cql_startup(struct cql_client *client);
int cql_query(struct cql_client *client, char * query, int consistency);
int cql_recv_msg(struct cql_client *client);

#endif

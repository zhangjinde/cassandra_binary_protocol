#include <stdio.h>
#include <stdlib.h>

#include "cql.h"

int main(int argc, char **argv)
{
    struct cql_client *client = NULL;
    int result = 1;
    int i;

    if (cql_client_create(&client) != CQL_OK)
    {
        printf("Error creating CQL client\n");
        return 1;
    }

    if (cql_client_connect(client, "127.0.0.1", 9042) == CQL_OK)
    {
        printf("Connected\n");

        cql_startup(client);
        cql_recv_msg(client);

        cql_query(client,"CREATE KEYSPACE keyspace1 WITH REPLICATION = { 'class' : 'SimpleStrategy', 'replication_factor' : 1 }",CQL_CONSISTENCY_ANY);

        cql_query(client,"USE keyspace1",CQL_CONSISTENCY_ANY);

        /* Creating a column family with a single validated column */
        cql_query(client,"CREATE COLUMNFAMILY users (id varchar PRIMARY KEY, email varchar)",CQL_CONSISTENCY_ANY);

        /* Create an index on the name */
        cql_query(client,"CREATE INDEX users_email_idx ON users (email)",CQL_CONSISTENCY_ANY);

        /*
         * Insert into a Column Family
         */

        for (i = 0; i < 1000; i++)
        {
            char buf[256];

            sprintf(buf,"INSERT INTO users (id, email) VALUES ('kreynolds%d', 'kelley@insidesystems.net')",i);
            cql_query(client,buf,CQL_CONSISTENCY_ANY);

            sprintf(buf,"INSERT INTO users (id, email) VALUES ('kway%d', 'kevin@insidesystems.net')",i);
            cql_query(client,buf,CQL_CONSISTENCY_ANY);
        }

        cql_query(client,"UPDATE users SET email='kreynolds@insidesystems.net' WHERE id='kreynolds2'",CQL_CONSISTENCY_ANY);

        cql_query(client,"SELECT * FROM users",CQL_CONSISTENCY_QUORUM);
        cql_query(client,"SELECT * FROM users WHERE id='kreynolds'",CQL_CONSISTENCY_QUORUM);
        cql_query(client,"SELECT * FROM users where email='kreynolds@insidesystems.net",CQL_CONSISTENCY_QUORUM);

        cql_query(client,"USE system",CQL_CONSISTENCY_ANY);
        cql_query(client,"DROP KEYSPACE keyspace1",CQL_CONSISTENCY_ANY);

        result = 0;
    }

    cql_client_free(client);

    return result;
}

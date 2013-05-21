#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "cql.h"
#include "cql_parser.h"
#include "hexdump.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

static int cql_make_query_body(char * buf, int buflen, char * query, int consistency);
static int cql_send(int s, const char* buf, int len);
static int cql_make_startup_string_map(char * buf);

int cql_client_create(struct cql_client **client_out)
{
    struct cql_client * client;
    int err;

    if (client_out == NULL)
        return CQL_ERROR_INVALID_PARAMETERS;

    client = malloc(sizeof(struct cql_client));
    if (!client)
        return CQL_ERROR_MEMORY;

    err = cql_client_init(client);
    if (err == CQL_OK)
    {
        *client_out = client;
    }
    else
    {
        free(client);
        *client_out = NULL;
    }

    return err;
}

int cql_client_init(struct cql_client *client)
{
    if (client == NULL)
        return CQL_ERROR_INVALID_PARAMETERS;

    memset(client, 0, sizeof(struct cql_client));
    client->s = -1;

    return CQL_OK;
}

int cql_client_free(struct cql_client *client)
{
    if (!client)
        return CQL_ERROR_INVALID_PARAMETERS;
    if (client->s != -1)
    {
        shutdown(client->s,SHUT_RDWR);
        close(client->s);
        client->s = -1;
    }
    free(client);
    return CQL_OK;
}

const char *cql_opcode(int code)
{
    switch(code)
    {
    case CQL_OPCODE_ERROR:           return "CQL_OPCODE_ERROR";
    case CQL_OPCODE_STARTUP:         return "CQL_OPCODE_STARTUP";
    case CQL_OPCODE_READY:           return "CQL_OPCODE_READY";
    case CQL_OPCODE_AUTHENTICATE:    return "CQL_OPCODE_AUTHENTICATE";
    case CQL_OPCODE_CREDENTIALS:     return "CQL_OPCODE_CREDENTIALS";
    case CQL_OPCODE_OPTIONS:         return "CQL_OPCODE_OPTIONS";
    case CQL_OPCODE_SUPPORTED:       return "CQL_OPCODE_SUPPORTED";
    case CQL_OPCODE_QUERY:           return "CQL_OPCODE_QUERY";
    case CQL_OPCODE_RESULT:          return "CQL_OPCODE_RESULT";
    case CQL_OPCODE_PREPARE:         return "CQL_OPCODE_PREPARE";
    case CQL_OPCODE_EXECUTE:         return "CQL_OPCODE_EXECUTE";
    case CQL_OPCODE_REGISTER:        return "CQL_OPCODE_REGISTER";
    case CQL_OPCODE_EVENT:           return "CQL_OPCODE_EVENT";
    default:                         return "<unknown>";
    }
}

int cql_client_connect(struct cql_client *client, char *hostname, int port)
{
    struct sockaddr_in addr;

    if (client == NULL || client->s != -1)
        return CQL_ERROR_INVALID_PARAMETERS;

    if (strlen(hostname) + 1 > CQL_HOSTNAME_LEN)
        return CQL_ERROR_INVALID_PARAMETERS;

    strcpy(client->hostname,hostname);
    client->port = port;

    memset(&addr, 0, sizeof(addr));

    /* TODO: perform proper gethostbyname lookup */
    inet_pton(AF_INET, client->hostname, &addr.sin_addr.s_addr);
    addr.sin_port = htons(client->port);

    client->s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client->s == -1)
    {
        printf("error creating socket(%d)\n",errno);
        return CQL_ERROR_SOCKET;
    }

    if (connect(client->s, (struct sockaddr *) &addr, sizeof(addr)) != 0)
    {
        printf("connect to %s:%d failed (%d)\n", client->hostname, client->port, errno);
        return CQL_ERROR_CONNECT;
    }

    return CQL_OK;
}

static int cql_send(int s, const char* buf, int len)
{
    int n;

    printf("Sending cql message: v=%02x fl=%02x st=%02x op=%02x (%s) len=%d\n",
        buf[0], buf[1], buf[2], buf[3], cql_opcode(buf[3]), ntohl(*(int *)(buf+4)));
    n = send(s,buf,len,0);
    if (n != len)
    {
        return -1;
    }

    return 0;
}

static int cql_make_startup_string_map(char * buf)
{
    char * p = buf;
    int n;

    *(unsigned short *)p = htons(1);
    p += 2;

    *(unsigned short *)p = htons(n = strlen(CQL_VERSION_KEY));
    p += 2;
    memcpy(p,CQL_VERSION_KEY,n);
    p += n;

    *(unsigned short *)p = htons(n = strlen(CQL_VERSION_SUPPORTED));
    p += 2;
    memcpy(p,CQL_VERSION_SUPPORTED,n);
    p += n;

    return p - buf;
}

int cql_startup(struct cql_client *client)
{
    char buf[64];
    struct cql_header *hdr;
    int n;
    int s;

    if (client == NULL)
        return CQL_ERROR_INVALID_PARAMETERS;

    s = client->s;

    hdr = (struct cql_header *)buf;
    n = cql_make_startup_string_map(buf + sizeof(struct cql_header));
    assert(n + sizeof(struct cql_header) <= sizeof(buf));

    hdr->cql_version = CQL_VERSION | CQL_REQUEST;
    hdr->cql_flags = CQL_FLAG_NONE;
    hdr->cql_stream = 1;
    hdr->cql_opcode = CQL_OPCODE_STARTUP;
    hdr->cql_body_length = htonl(n);

    n = cql_send(s, buf, sizeof(struct cql_header) + n);
    if (n == -1)
    {
        printf("Failed to send startup message (%d)\n",errno);
        return CQL_ERROR;
    }

    return CQL_OK;
}

static int cql_make_query_body(char * buf, int buflen, char * query, int consistency)
{
    char * p = buf;
    int n;

    /* check that the provided buffer is large enough for the data we are going to put into it */
    if (buflen < 4 + strlen(query) + 2)
        return 0;

    *(unsigned long *)p = htonl(n = strlen(query));
    p += 4;

    memcpy(p,query,n);
    p += n;

    *(unsigned short *)p = htons(consistency);
    p += 2;

    return p - buf;
}

int cql_query(struct cql_client *client, char * query, int consistency)
{
    char buf[512];
    struct cql_header *hdr;
    int n;
    int s;

    if (client == NULL)
        return CQL_ERROR_INVALID_PARAMETERS;

    s = client->s;

    hdr = (struct cql_header *)buf;
    n = cql_make_query_body(buf + sizeof(struct cql_header), sizeof(buf) - sizeof(struct cql_header), query, consistency);
    assert(n + sizeof(struct cql_header) <= sizeof(buf));

    hdr->cql_version = CQL_VERSION | CQL_REQUEST;
    hdr->cql_flags = CQL_FLAG_NONE;
    hdr->cql_stream = 1;
    hdr->cql_opcode = CQL_OPCODE_QUERY;
    hdr->cql_body_length = htonl(n);

    n = cql_send(s, buf, sizeof(struct cql_header) + n);
    if (n == -1)
    {
        printf("Failed to send query message (%d)\n",errno);
        return CQL_ERROR;
    }

    return cql_recv_msg(client);
}

int cql_recv_msg(struct cql_client *client)
{
    unsigned char buf[10*1024], *p, *e;
    struct cql_header *hdr;
    int n, msglen = 0, msgread = 0;
    int c, s;
    int retcode = CQL_ERROR;

    if (client == NULL)
        return CQL_ERROR_INVALID_PARAMETERS;

    s = client->s;
    cql_header_parser_init(&client->hdr_parse_state);

    for (;;)
    {
        printf("reading data (current msglen=%d, msgread=%d)\n",msglen,msgread);

        n = recv(s, buf, sizeof(buf), 0);
        if (n == -1)
        {
            printf("Error receiving data from CQL server (%d)\n", errno);
            break;
        }

        printf("Received %d bytes from CQL server\n", n);
        p = buf;

        /* see if there is more data needed for the header */
        if (!cql_header_parser_complete(&client->hdr_parse_state))
        {
            cql_header_parser_process_data(&client->hdr_parse_state,p,n,&e);
            msgread += (e-p);
            n -= (e-p);
            p = e;
        }

        /* if the header is complete, extract the message length from the header block */
        if (cql_header_parser_complete(&client->hdr_parse_state))
        {
            if (msglen == 0)
            {
                hdr = cql_header_parser_getvalue(&client->hdr_parse_state);

                printf("recived CQL header v=%02x fl=%02x st=%02x op=%02x (%s) len=%d\n",
                    hdr->cql_version, hdr->cql_flags, hdr->cql_stream,
                    hdr->cql_opcode, cql_opcode(hdr->cql_opcode),
                    hdr->cql_body_length);

                msglen = sizeof(struct cql_header) + hdr->cql_body_length;
            }
        }

        /* msgread is the total number of bytes we have read (and processed) */
        /* msglen is the total byte length of the whole message */
        /* n is the number of bytes at *p which are yet to be processed */

        if (n > 0 && msgread < msglen)
        {
            c = MIN(n, msglen - msgread);
            /* todo: send the bytes to the response parser instead of just to hexdump */
            hexdump(NULL, p, c);
            msgread += c;
        }

        /* check if the whole message has been read */
        if (msglen > 0 && msgread == msglen)
        {
            /* the whole message has been processed */
            retcode = CQL_OK;
            break;
        }
    }

    return retcode;
}

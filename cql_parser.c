#include <stdlib.h>
#include <string.h>

#include "cql.h"


static int cql_parser_process_data(struct cql_parser_base *p, int (*fn)(struct cql_parser_base *,unsigned char), unsigned char *s, int len, unsigned char **e )
{
    while (len > 0 && p->bytes_remaining != 0)
    {
        (*fn)(p,*s);
        s++;
        len--;
    }
    *e = s;
    return p->bytes_remaining == 0;
}

static int cql_parser_complete(struct cql_parser_base* p)
{
    return p->bytes_remaining == 0;
}


/*
 * Integer parsing
 */

void cql_int_parser_init_32(struct cql_int_parser_state* p)
{
    p->b.bytes_remaining = 4;
    p->value = 0;
}

void cql_int_parser_init_16(struct cql_int_parser_state* p)
{
    p->b.bytes_remaining = 2;
    p->value = 0;
}

int cql_int_parser_complete(struct cql_int_parser_state* p)
{
    return cql_parser_complete(&p->b);
}

int cql_int_parser_getvalue(struct cql_int_parser_state* p)
{
    return p->value;
}

int cql_int_parser_process_byte(struct cql_int_parser_state* p, unsigned char b)
{
    p->value = (p->value << 8) | b;
    if (p->b.bytes_remaining > 0)
        p->b.bytes_remaining--;
    return p->b.bytes_remaining == 0;
}

static int cql_int_parser_process_byte_fn(struct cql_parser_base * p, unsigned char b)
{
    return cql_int_parser_process_byte((struct cql_int_parser_state *)p,b);
}

int cql_int_parser_process_data(struct cql_int_parser_state* p, unsigned char *s, int len, unsigned char **e )
{
    return cql_parser_process_data(&p->b,cql_int_parser_process_byte_fn,s,len,e);
}


/*
 * String parsing
 */

int cql_string_parser_init(struct cql_string_parser_state* p, int len)
{
    p->b.bytes_remaining = len;
    if (len < sizeof(p->staticbuf))
    {
        p->buf = p->staticbuf;
    }
    else
    {
        p->buf = malloc(len+1);
        if (p->buf == NULL)
            return CQL_ERROR_MEMORY;
    }
    p->p = p->buf;
    return CQL_OK;
}


int cql_string_parser_cleanup(struct cql_string_parser_state* p)
{
    if (p->buf != NULL && p->buf != p->staticbuf)
    {
        free(p->buf);
        p->buf = NULL;
    }
    return CQL_OK;
}


int cql_string_parser_complete(struct cql_string_parser_state* p)
{
    return cql_parser_complete(&p->b);
}


char *cql_string_parser_getvalue(struct cql_string_parser_state* p)
{
    return p->buf;
}

int cql_string_parser_process_byte(struct cql_string_parser_state* p, unsigned char b)
{
    if (p->b.bytes_remaining > 0)
    {
        *p->p++ = b;
        p->b.bytes_remaining--;
        if (p->b.bytes_remaining == 0)
            *p->p = '\0';
    }
    return p->b.bytes_remaining == 0;
}

static int cql_string_parser_process_byte_fn(struct cql_parser_base * p, unsigned char b)
{
    return cql_string_parser_process_byte((struct cql_string_parser_state *)p,b);
}

int cql_string_parser_process_data(struct cql_string_parser_state* p, unsigned char *s, int len, unsigned char **e )
{
    return cql_parser_process_data(&p->b,cql_string_parser_process_byte_fn,s,len,e);
}



/*
 * Header parsing
 */

void cql_header_parser_init(struct cql_header_parser_state* p)
{
    p->b.bytes_remaining = sizeof(struct cql_header);
    memset(&p->hdr,0,sizeof(struct cql_header));
    p->p = (char *)&p->hdr;
}

int cql_header_parser_complete(struct cql_header_parser_state* p)
{
    return cql_parser_complete(&p->b);
}

struct cql_header *cql_header_parser_getvalue(struct cql_header_parser_state* p)
{
    return &p->hdr;
}

int cql_header_parser_process_byte(struct cql_header_parser_state* p, unsigned char b)
{
    if (p->b.bytes_remaining > 0)
    {
        *p->p++ = b;
        p->b.bytes_remaining--;
        if (p->b.bytes_remaining == 0)
        {
            /* fix byte ordering */
            p->hdr.cql_body_length = ntohl(p->hdr.cql_body_length);
        }
    }
    return p->b.bytes_remaining == 0;
}

static int cql_header_parser_process_byte_fn(struct cql_parser_base * p, unsigned char b)
{
    return cql_header_parser_process_byte((struct cql_header_parser_state *)p,b);
}

int cql_header_parser_process_data(struct cql_header_parser_state* p, unsigned char *s, int len, unsigned char **e )
{
    return cql_parser_process_data(&p->b,cql_header_parser_process_byte_fn,s,len,e);
}


/*
 * Result parsing
 */


int cql_parse_result_message(struct cql_result_parser_state *state, char * buf, int buflen)
{
    return 0;
}

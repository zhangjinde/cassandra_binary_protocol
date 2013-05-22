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
    if (e)
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

void cql_header_parser_init(struct cql_header_parser* p)
{
    p->b.bytes_remaining = sizeof(struct cql_header);
    memset(&p->hdr,0,sizeof(struct cql_header));
    p->p = (char *)&p->hdr;
}

int cql_header_parser_complete(struct cql_header_parser* p)
{
    return cql_parser_complete(&p->b);
}

struct cql_header *cql_header_parser_getvalue(struct cql_header_parser* p)
{
    return &p->hdr;
}

int cql_header_parser_process_byte(struct cql_header_parser* p, unsigned char b)
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
    return cql_header_parser_process_byte((struct cql_header_parser *)p,b);
}

int cql_header_parser_process_data(struct cql_header_parser* p, unsigned char *s, int len, unsigned char **e )
{
    return cql_parser_process_data(&p->b,cql_header_parser_process_byte_fn,s,len,e);
}


/*
 * Result parsing
 */

void cql_result_parser_init(struct cql_result_parser* p, void* callback_context)
{
    p->state = CQL_RESULT_IN_HEADER;
    p->callback_context = callback_context;
    cql_header_parser_init(&p->header_parser);
}

void cql_result_parser_set_callbacks(struct cql_result_parser* p, CQL_HEADER_CALLBACK_FN* header_callback)
{
    p->header_callback = header_callback;
}


int cql_result_parser_complete(struct cql_result_parser* p)
{
    return p->state == CQL_RESULT_DONE;
}

int cql_result_parser_process_byte(struct cql_result_parser* p, unsigned char b)
{
    struct cql_header *hdr;

    switch (p->state)
    {
    case CQL_RESULT_IN_HEADER:
        if (cql_header_parser_process_byte(&p->header_parser,b))
        {
            /* the header is now complete. get the body length and reset the bodyread counter */
            hdr = cql_header_parser_getvalue(&p->header_parser);
            /* TODO: perform some validation on the header. enter an error state if it does not appear to be valid */
            if (p->header_callback != NULL)
            {
                (p->header_callback)(hdr,p->callback_context);
            }
            p->bodylen = hdr->cql_body_length;
            p->bodyread = 0;
            if (p->bodylen == 0)
                p->state = CQL_RESULT_DONE;
            else
                p->state = CQL_RESULT_IN_BODY;
        }
        break;
    case CQL_RESULT_IN_BODY:
        /* TODO: pass the byte into a parser appropriate for the response type we are reading */
        p->bodyread++;
        if (p->bodyread == p->bodylen)
            p->state = CQL_RESULT_DONE;
        break;
    }
    return p->state == CQL_RESULT_DONE;
}

int cql_result_parser_process_data(struct cql_result_parser* p, unsigned char *s, int len, unsigned char **e )
{
    while (len > 0 && !cql_result_parser_complete(p))
    {
        cql_result_parser_process_byte(p,*s);
        s++;
        len--;
    }
    if (e)
        *e = s;
    return cql_result_parser_complete(p);
}


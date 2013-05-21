#ifndef _CQL_PARSER_H_INCLUDED_
#define _CQL_PARSER_H_INCLUDED_

struct cql_parser_base
{
    int magic;
    int bytes_remaining;
};

struct cql_int_parser_state
{
    struct cql_parser_base b;
    unsigned int value;
};

void cql_int_parser_init_32(struct cql_int_parser_state* p);
void cql_int_parser_init_16(struct cql_int_parser_state* p);
int cql_int_parser_complete(struct cql_int_parser_state* p);
int cql_int_parser_getvalue(struct cql_int_parser_state* p);
int cql_int_parser_process_byte(struct cql_int_parser_state* p, unsigned char b);
int cql_int_parser_process_data(struct cql_int_parser_state* p, unsigned char *s, int len, unsigned char **e );


#define CQL_STRING_PARSER_STATICBUF_LEN 128

struct cql_string_parser_state
{
    struct cql_parser_base b;
    char staticbuf[CQL_STRING_PARSER_STATICBUF_LEN];
    char *buf;
    char *p;
};

int cql_string_parser_init(struct cql_string_parser_state* p, int len);
int cql_string_parser_cleanup(struct cql_string_parser_state* p);
int cql_string_parser_complete(struct cql_string_parser_state* p);
char *cql_string_parser_getvalue(struct cql_string_parser_state* p);
int cql_string_parser_process_byte(struct cql_string_parser_state* p, unsigned char b);
int cql_string_parser_process_data(struct cql_string_parser_state* p, unsigned char *s, int len, unsigned char **e );



struct cql_header_parser
{
    struct cql_parser_base b;
    struct cql_header hdr;
    char *p;
};

void cql_header_parser_init(struct cql_header_parser* p);
int cql_header_parser_complete(struct cql_header_parser* p);
struct cql_header *cql_header_parser_getvalue(struct cql_header_parser* p);
int cql_header_parser_process_byte(struct cql_header_parser* p, unsigned char b);
int cql_header_parser_process_data(struct cql_header_parser* p, unsigned char *s, int len, unsigned char **e );




/* Result parsing */

#define CQL_RESULT_IN_HEADER        0
#define CQL_RESULT_IN_BODY          1
#define CQL_RESULT_DONE             2

struct cql_result_parser
{
    int state;
    struct cql_header_parser header_parser;
    int bodylen;
    int bodyread;
};

void cql_result_parser_init(struct cql_result_parser* p);
int cql_result_parser_complete(struct cql_result_parser* p);
int cql_result_parser_process_byte(struct cql_result_parser* p, unsigned char b);
int cql_result_parser_process_data(struct cql_result_parser* p, unsigned char *s, int len, unsigned char **e );

#endif

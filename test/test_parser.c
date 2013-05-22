#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cql.h"

int success = 0,
    fails = 0;

void test_section(char *name)
{
    int i, len = strlen(name);
    printf("\n%s\n",name);
    for (i = 0; i < len; i ++)
    {
        printf("-");
    }
    printf("\n");
}

void test_assert(char *file, int line, char *test, int cond)
{
    printf("%-40s%5s\n", test, cond ? "OK" : "FAIL");
    if (cond)
    {
        success++;
    }
    else
    {
        printf("    %s:%d\n",file,line);
        fails++;
    }
}

#define TEST(test,cond)  test_assert(__FILE__,__LINE__,test,cond)

void test_int_parser()
{
    struct cql_int_parser_state s;
    unsigned char buf[16], *p;

    /* test init */
    test_section("test init");
    cql_int_parser_init_32(&s);
    TEST("initial state", s.b.bytes_remaining == 4);
    TEST("initial value", s.value == 0);

    /* test parse one int byte */
    test_section("test int one byte");
    cql_int_parser_init_32(&s);
    cql_int_parser_process_byte(&s,80);
    TEST("final state", s.b.bytes_remaining == 3);
    TEST("final value", cql_int_parser_getvalue(&s) == 80);
    TEST("not complete", cql_int_parser_complete(&s) == 0);

    /* test parse 2 int bytes */
    test_section("test int two bytes");
    cql_int_parser_init_32(&s);
    cql_int_parser_process_byte(&s,0x13);
    cql_int_parser_process_byte(&s,0xf2);
    TEST("final state", s.b.bytes_remaining == 2);
    TEST("final value", cql_int_parser_getvalue(&s) == 0x13f2);
    TEST("not complete", cql_int_parser_complete(&s) == 0);

    /* test parse 4 int bytes */
    test_section("test int four bytes");
    cql_int_parser_init_32(&s);
    cql_int_parser_process_byte(&s,0xfd);
    cql_int_parser_process_byte(&s,0xcc);
    cql_int_parser_process_byte(&s,0x12);
    cql_int_parser_process_byte(&s,0xf9);
    TEST("final state", s.b.bytes_remaining == 0);
    TEST("final value", cql_int_parser_getvalue(&s) == 0xfdcc12f9);
    TEST("complete", cql_int_parser_complete(&s) == 1);


    /* test parse int buffer */
    test_section("test int buffer");
    *(unsigned long *)buf = htonl(0x89716f34);
    cql_int_parser_init_32(&s);
    cql_int_parser_process_data(&s,buf,4,&p);
    TEST("final state", s.b.bytes_remaining == 0);
    TEST("final value", cql_int_parser_getvalue(&s) == 0x89716f34);
    TEST("complete", cql_int_parser_complete(&s) == 1);
}

void test_short_parser()
{
    struct cql_int_parser_state s;

    /* test parse 2 short bytes */
    test_section("test int two bytes");
    cql_int_parser_init_16(&s);
    cql_int_parser_process_byte(&s,0x13);
    cql_int_parser_process_byte(&s,0xf2);
    TEST("final state", s.b.bytes_remaining == 0);
    TEST("final value", cql_int_parser_getvalue(&s) == 0x13f2);
    TEST("complete", cql_int_parser_complete(&s) == 1);
}

void test_string_parser()
{
    struct cql_string_parser_state s;
    unsigned char data[160], *ptr;
    int i;

    /* test a short string */
    test_section("four byte string");
    cql_string_parser_init(&s,4);
    TEST("using static buffer",s.buf == s.staticbuf);
    TEST("initial len",s.b.bytes_remaining == 4);
    cql_string_parser_process_byte(&s,'b');
    cql_string_parser_process_byte(&s,'a');
    TEST("partial len",s.b.bytes_remaining == 2);
    cql_string_parser_process_byte(&s,'z');
    cql_string_parser_process_byte(&s,'z');
    TEST("output value", strcmp(cql_string_parser_getvalue(&s),"bazz") == 0);
    TEST("complete", cql_string_parser_complete(&s) == 1);
    cql_string_parser_cleanup(&s);
    TEST("cleanup",s.buf == s.staticbuf);

    /* test a long string */
    test_section("long string");
    cql_string_parser_init(&s,150);
    TEST("using dynamic buffer",s.buf != s.staticbuf && s.buf != NULL);
    TEST("initial pointer",s.p == s.buf);
    TEST("initial len",s.b.bytes_remaining == 150);
    for (i = 0; i < 15; i++)
    {
        strcpy((char *)data,"abcdefghij");
        cql_string_parser_process_data(&s,data,10,&ptr);
        TEST("partial len",s.b.bytes_remaining == 150 - 10 * (i+1));
        TEST("not complete", cql_string_parser_complete(&s) == ((i+1)==15 ? 1 : 0));
    }
    *data = 0;
    for (i = 0; i < 15; i++)
    {
        strcat((char *)data,"abcdefghij");
    }
    TEST("output value", strcmp(cql_string_parser_getvalue(&s),(char *)data) == 0);
    TEST("complete", cql_string_parser_complete(&s) == 1);
    cql_string_parser_cleanup(&s);
    TEST("cleanup",s.buf == NULL);
}

void test_combined_parsers()
{
    unsigned char buf[14];
    unsigned char *p = buf;
    struct cql_int_parser_state ip;
    struct cql_string_parser_state sp;

    test_section("combined parsers");

    /* create a buffer of data with a number followed by a string */
    *(short *)buf = htons(12);
    memcpy(buf+2,"hello world!",12);

    cql_int_parser_init_16(&ip);
    cql_int_parser_process_data(&ip,p,14,&p);

    TEST("int value",cql_int_parser_getvalue(&ip) == 12);
    TEST("pointer position", p == buf+2);
    cql_string_parser_init(&sp,12);
    cql_string_parser_process_data(&sp,p,12,&p);
    TEST("string value",strcmp(cql_string_parser_getvalue(&sp),"hello world!") == 0);
    TEST("poiner position", p == buf+14);
}

void test_header_parser()
{
    struct cql_header hdr, *parsed_hdr;
    struct cql_header_parser p;
    unsigned char *d;

    d = (unsigned char *)&hdr;

    test_section("header parsers");

    hdr.cql_version = CQL_VERSION | CQL_REQUEST;
    hdr.cql_flags = CQL_FLAG_NONE;
    hdr.cql_stream = 1;
    hdr.cql_opcode = CQL_OPCODE_QUERY;
    hdr.cql_body_length = htonl(1000);

    cql_header_parser_init(&p);

    TEST("init", p.b.bytes_remaining == sizeof(struct cql_header));

    cql_header_parser_process_data(&p,d,5,&d);

    TEST("remaining", p.b.bytes_remaining == 3);

    cql_header_parser_process_data(&p,d,3,&d);

    parsed_hdr = cql_header_parser_getvalue(&p);

    TEST("value not null", parsed_hdr != NULL);
    TEST("header cql_version", parsed_hdr->cql_version == (CQL_VERSION | CQL_REQUEST));
    TEST("header cql_flags", parsed_hdr->cql_flags == CQL_FLAG_NONE);
    TEST("header cql_stream", parsed_hdr->cql_stream == 1);
    TEST("header cql_opcode", parsed_hdr->cql_opcode == CQL_OPCODE_QUERY);
    TEST("header cql_cql_body_length", parsed_hdr->cql_body_length == 1000);
}

void test_header_callback_fn(struct cql_header * hdr, void *ctx)
{
    TEST("callback header not null", hdr != NULL);
    TEST("callback context not null", ctx != NULL);
    TEST("callback header cql_version", hdr->cql_version == (CQL_VERSION | CQL_REQUEST));
    *(int *)ctx = 1;
}

void test_result_parser()
{
    struct cql_header *hdr, *parsed_hdr;
    unsigned char buf[sizeof(struct cql_header) + 8], *d = buf;
    struct cql_result_parser p;
    int ctx = 0;

    hdr = (struct cql_header *)buf;

    test_section("result parsers");

    hdr->cql_version = CQL_VERSION | CQL_REQUEST;
    hdr->cql_flags = CQL_FLAG_NONE;
    hdr->cql_stream = 1;
    hdr->cql_opcode = CQL_OPCODE_QUERY;
    hdr->cql_body_length = htonl(4);
    *(int*)(buf + sizeof(struct cql_header)) = 0xdeadf00d;

    cql_result_parser_init(&p,&ctx);
    cql_result_parser_set_callbacks(&p,test_header_callback_fn);

    TEST("init state", p.state == CQL_RESULT_IN_HEADER);
    TEST("init", p.header_parser.b.bytes_remaining == sizeof(struct cql_header));

    cql_result_parser_process_data(&p,d,5,&d);
    TEST("pointer", d == buf + 5);
    TEST("state", p.state == CQL_RESULT_IN_HEADER);
    TEST("remaining", p.header_parser.b.bytes_remaining == 3);

    cql_result_parser_process_data(&p,d,4,&d);
    TEST("pointer", d == buf + 9);
    TEST("state", p.state == CQL_RESULT_IN_BODY);

    parsed_hdr = cql_header_parser_getvalue(&p.header_parser);

    TEST("value not null", parsed_hdr != NULL);
    TEST("header cql_version", parsed_hdr->cql_version == (CQL_VERSION | CQL_REQUEST));
    TEST("header cql_flags", parsed_hdr->cql_flags == CQL_FLAG_NONE);
    TEST("header cql_stream", parsed_hdr->cql_stream == 1);
    TEST("header cql_opcode", parsed_hdr->cql_opcode == CQL_OPCODE_QUERY);
    TEST("header cql_cql_body_length", parsed_hdr->cql_body_length == 4);

    cql_result_parser_process_data(&p,d,7,&d);
    TEST("pointer", d == buf + 12);
    TEST("state", p.state == CQL_RESULT_DONE);
}

int main()
{
    test_int_parser();
    test_short_parser();
    test_string_parser();
    test_combined_parsers();
    test_header_parser();
    test_result_parser();

    printf("\nsuccess: %d, fail: %d\n",success,fails);

    return fails;
}

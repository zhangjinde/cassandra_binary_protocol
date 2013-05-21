/* cql_header.h */

#ifndef _CQL_HEADER_H_INCLUDED_
#define _CQL_HEADER_H_INCLUDED_

/*
 * See https://git-wip-us.apache.org/repos/asf?p=cassandra.git;a=blob_plain;f=doc/native_protocol.spec;hb=refs/heads/cassandra-1.2
 * referred to below as [CBP]
 */

/* Values for the cql_header.cql_version field */

#define CQL_VERSION 0x01

#define CQL_REQUEST  0x00
#define CQL_RESPONSE 0x80


/* Values for the cql_header.cql_flags field */

#define CQL_FLAG_NONE               0x00
#define CQL_FLAG_COMPRESSION        0x01
#define CQL_FLAG_TRACING            0x02


/* Values for the cql_header.cql_opcode field [CBP.2.4] */

#define CQL_OPCODE_ERROR           0x00
#define CQL_OPCODE_STARTUP         0x01
#define CQL_OPCODE_READY           0x02
#define CQL_OPCODE_AUTHENTICATE    0x03
#define CQL_OPCODE_CREDENTIALS     0x04
#define CQL_OPCODE_OPTIONS         0x05
#define CQL_OPCODE_SUPPORTED       0x06
#define CQL_OPCODE_QUERY           0x07
#define CQL_OPCODE_RESULT          0x08
#define CQL_OPCODE_PREPARE         0x09
#define CQL_OPCODE_EXECUTE         0x0A
#define CQL_OPCODE_REGISTER        0x0B
#define CQL_OPCODE_EVENT           0x0C


/* Values for the Consistency value in the Query [CBP.3] */

#define CQL_CONSISTENCY_ANY            0x0000
#define CQL_CONSISTENCY_ONE            0x0001
#define CQL_CONSISTENCY_TWO            0x0002
#define CQL_CONSISTENCY_THREE          0x0003
#define CQL_CONSISTENCY_QUORUM         0x0004
#define CQL_CONSISTENCY_ALL            0x0005
#define CQL_CONSISTENCY_LOCAL_QUORUM   0x0006
#define CQL_CONSISTENCY_EACH_QUORUM    0x0007


#define CQL_VERSION_KEY "CQL_VERSION"
#define CQL_VERSION_SUPPORTED "3.0.0"


const char *cql_opcode(int code);

/*
 * Defines the structure of a CQL packet header.
 */
struct cql_header
{
    /* See [CBP.2.1] version contains both protocol version in the low bits and a request/response flag in the high bit */
    uint8_t cql_version;

    /* See [CBP.2.2] Specifies flags for compression and tracing */
    uint8_t cql_flags;

    /* See [CBP.2.3] Identifies the STREAM ID. */
    uint8_t cql_stream;

    /* See [CBP.2.4] Identifis the operation code (opcode) */
    uint8_t cql_opcode;

    /* See [CBP.1] Length of body (in network byte order) following the header */
    uint32_t cql_body_length;

};

#endif

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <stdint.h>

#include "net/ipv4/addr.h"

#include <sancus/sm_support.h>

#include "enclave_utils.h"

typedef struct
{
    conn_index    conn_id;
    sm_id         to_sm;
    uint16_t      to_port;
    ipv4_addr_t   to_address;
    uint8_t       local;
} Connection;

// Copies connection so may be stack allocated.
int connections_add(Connection* connection);

// We keep ownership of the returned Connection. May return NULL.
Connection* connections_get(uint16_t conn_id);

// Replaces an existing connection
int connections_replace(Connection* connection);


#endif

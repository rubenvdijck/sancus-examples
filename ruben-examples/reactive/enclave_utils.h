#ifndef __ENCLAVE_UTILS_H__
#define __ENCLAVE_UTILS_H__

#include <stdint.h>

#include <sancus_support/tools.h>
#include <sancus/sm_support.h>

#include "networking.h"

typedef uint16_t io_index;
typedef uint16_t conn_index;

ResultMessage load_enclave(CommandMessage m);

void reactive_handle_output(conn_index conn_id, void* data, size_t len);
void reactive_handle_input(sm_id sm, conn_index conn_id, void* data, size_t len);

ResultMessage handle_set_key(sm_id id, ParseState *state);
ResultMessage handle_attest(sm_id id, ParseState *state);
ResultMessage handle_disable(sm_id id, ParseState *state);
ResultMessage handle_user_entrypoint(sm_id id, uint16_t index, ParseState *state);

#endif

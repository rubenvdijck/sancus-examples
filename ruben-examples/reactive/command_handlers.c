#include "command_handlers.h"

#include <sancus_support/sm_control.h>
#include <sancus_support/tools.h>

#include "log.h"
#include "net/ipv4/addr.h"

#include "enclave_utils.h"
#include "connection.h"
#include "byteorder.h"
#include "utils.h"

#if USE_PERIODIC_EVENTS
  #include "periodic_event.h"
#endif

ResultMessage handler_add_connection(CommandMessage m) {
  LOG_DEBUG("[EM] Received add connection\n");
  Connection connection;
  ipv4_addr_t* parsed_addr;

  ParseState *state = create_parse_state(m->message->payload,
                           m->message->size);

   // The payload format is [conn_id to_sm port address]
  // which is basically this same thing as a Connection.

  if (!parse_int(state, &connection.conn_id))
     return RESULT(ResultCode_IllegalPayload);
  if (!parse_int(state, &connection.to_sm))
     return RESULT(ResultCode_IllegalPayload);
 if (!parse_byte(state, &connection.local))
    return RESULT(ResultCode_IllegalPayload);
  if (!parse_int(state, &connection.to_port))
    return RESULT(ResultCode_IllegalPayload);

  if (!parse_raw_data(state, sizeof(ipv4_addr_t), (uint8_t**)&parsed_addr))
    return RESULT(ResultCode_IllegalPayload);
  connection.to_address = *parsed_addr;

  free_parse_state(state);
  destroy_command_message(m);

  if (!connections_replace(&connection) && !connections_add(&connection))
     return RESULT(ResultCode_InternalError);

  return RESULT(ResultCode_Ok);
}

ResultMessage handler_call_entrypoint(CommandMessage m) {
  LOG_DEBUG("[EM] Received entrypoint call\n");
  ParseState *state = create_parse_state(m->message->payload,
                           m->message->size);

  // The payload format is [sm_id, index, args]
  sm_id id;
  if (!parse_int(state, &id))
     return RESULT(ResultCode_IllegalPayload);

  uint16_t index;
  if (!parse_int(state, &index))
     return RESULT(ResultCode_IllegalPayload);

  ResultMessage res;

  switch(index) {
    case Entrypoint_SetKey:
      res = handle_set_key(id, state);
      break;
    case Entrypoint_Attest:
      res = handle_attest(id, state);
      break;
    case Entrypoint_Disable:
      res = handle_disable(id, state);
    default:
      res = handle_user_entrypoint(id, index, state);
  }

  free_parse_state(state);
  destroy_command_message(m);

  return res;
}

ResultMessage handler_remote_output(CommandMessage m) {
  LOG_DEBUG("[EM] Received remote output\n");
  ParseState *state = create_parse_state(m->message->payload,
                           m->message->size);

  // The packet format is [sm_id conn_id data]
  sm_id sm;
  if (!parse_int(state, &sm))
      return RESULT(ResultCode_IllegalPayload);

  uint16_t conn_id;
  if (!parse_int(state, &conn_id))
      return RESULT(ResultCode_IllegalPayload);

  uint8_t* payload;
  size_t payload_len;
  if (!parse_all_raw_data(state, &payload, &payload_len))
      return RESULT(ResultCode_IllegalPayload);

  reactive_handle_input(sm, conn_id, payload, payload_len);

  free_parse_state(state);
  destroy_command_message(m);

  return RESULT(ResultCode_Ok);
}

ResultMessage handler_load_sm(CommandMessage m) {
  LOG_DEBUG("[EM] Received new module\n");
  ResultMessage res = load_enclave(m);
  // m is destroyed inside load_enclave
  return res;
}

ResultMessage handler_ping(CommandMessage m) {
  LOG_DEBUG("[EM] Received ping\n");
  destroy_command_message(m);
  return RESULT(ResultCode_Ok);
}

ResultMessage handler_register_entrypoint(CommandMessage m) {
  LOG_DEBUG("[EM] Received entrypoint registration\n");

#if USE_PERIODIC_EVENTS
  PeriodicEvent event;
  ParseState *state = create_parse_state(m->message->payload,
                           m->message->size);

   // The payload format is [module entry frequency]
  if (!parse_int(state, &event.module))
     return RESULT(ResultCode_IllegalPayload);
  if (!parse_int(state, &event.entry))
     return RESULT(ResultCode_IllegalPayload);

  uint32_t *freq;
  if (!parse_raw_data(state, sizeof(uint32_t), (uint8_t **)&freq))
    return RESULT(ResultCode_IllegalPayload);

  event.frequency = REVERSE_INT32(*freq);
  event.counter = 0;

  free_parse_state(state);
  destroy_command_message(m);

  periodic_event_add(&event);
#else
  LOG_WARNING("Periodic events disabled\n");
  destroy_command_message(m);
#endif

  return RESULT(ResultCode_Ok);
}

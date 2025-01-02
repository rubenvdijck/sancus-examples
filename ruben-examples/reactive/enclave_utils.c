#include "enclave_utils.h"

#include <sancus_support/sm_control.h>
#include <sancus_support/global_symtab.h>
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "byteorder.h"
#include "net/ipv4/addr.h"

#include "command_handlers.h"
#include "utils.h"
#include "connection.h"

#if USE_MINTIMER && TIMING_TESTS
  #include "mintimer.h"
  static uint32_t start_time = 0;
#endif

#define LOAD_SM_RESPONSE_BUFFER 512
#define RAM_TO_FILL 8192

/* store the conn_idx ids associated to a connection ID. conn_idx is an index
 * used internally by each SM, which is returned by set_key.It has to be passed
 * as input to the handle_input entry point of the SM
 *
 * Note: adjust MAX_CONNECTIONS accordingly. This value should ALWAYS be >= than
 *       the total number of connections in the Authentic Execution deployment
 *
 * TODO: this array should be dynamic in a real scenario.
 */
#define MAX_CONNECTIONS 32
uint16_t connection_idxs[MAX_CONNECTIONS] = { -1 };

ResultCode load_sm_from_buffer(ParseState *state, sm_id *id) {
  // The buf format is [NAME \0 VID ELF_FILE]
  char* name;
  if (!parse_string(state, &name))
  {
      LOG_ERROR("Err: name\n");
      return ResultCode_IllegalPayload;
  }

  vendor_id vid;
  if (!parse_int(state, &vid))
  {
      LOG_ERROR("Err: VID\n");
      return ResultCode_IllegalPayload;
  }

  uint8_t* sm_file;
  size_t sm_file_size;
  if(!parse_all_raw_data(state, &sm_file, &sm_file_size)) {
    LOG_ERROR("SM parse fail\n");
    return ResultCode_IllegalPayload;
  }

  LOG_DEBUG("loading SM with name %s and VID %u\n", name, vid);
  *id = sm_load(sm_file, name, vid);

  if (*id == 0) {
      LOG_ERROR("sm_load failed\n");
      return ResultCode_InternalError;
  }

  LOG_INFO("Loaded SM %u\n", *id);
  return ResultCode_Ok;
}

void copy_to_buf(unsigned char **buf, void* data, size_t size) {
  memcpy(*buf, data, size);
  *buf += size;
}

ResultCode read_global_symbols(unsigned char **buf, size_t num_symbols) {
  size_t current_symbol;
  char symbol_buf[128];

  for (current_symbol = 0; current_symbol < num_symbols; current_symbol++) {
    Symbol symbol;
    int is_section;
    ElfModule* module;

    if (!symtab_get_symbol(current_symbol,
                           &symbol, &is_section, &module)) {
        LOG_ERROR("symtab error\n");
        return ResultCode_InternalError;
    }

    if (!is_section && module == NULL) {
        size_t len = snprintf(symbol_buf,
                              sizeof(symbol_buf),
                              "%s = %p;\n", symbol.name, symbol.value);

        if (len + 1 > sizeof(symbol_buf)) { // len+1 to include \0
            LOG_ERROR("symtab buf too small\n");
            return ResultCode_InternalError;
        }

        copy_to_buf(buf, symbol_buf, len);
    }
  }

  return ResultCode_Ok;
}

ResultCode read_module_sections(unsigned char **buf, sm_id id, size_t num_symbols) {
  ElfModule *elf = sm_get_elf_by_id(id);
  size_t current_symbol;
  char symbol_buf[128];

  char *sectname = "SECTIONS\n{\n";
  copy_to_buf(buf, sectname, strlen(sectname));

  for (current_symbol = 0; current_symbol < num_symbols; current_symbol++) {
      Symbol symbol;
      int is_section;
      ElfModule* module;

      if (!symtab_get_symbol(current_symbol, &symbol, &is_section, &module)) {
          LOG_ERROR("symtab error\n");
          return ResultCode_InternalError;
      }

      if (is_section && module == elf) {
          // fix symbol name
          char *name = strdup(symbol.name);
          unsigned int sym_len = strlen(name);
          for(; sym_len > 0 && symbol.name[sym_len - 1] < 33; sym_len--);
          if(sym_len > 0) name[sym_len] = '\0';

          size_t len = snprintf(symbol_buf,
                                sizeof(symbol_buf),
                                "%s %p : {}\n", name, symbol.value);

          free(name);

          if (len > sizeof(symbol_buf)) {
              LOG_ERROR("symtab buffer too small\n");
              return ResultCode_InternalError;
          }

          copy_to_buf(buf, symbol_buf, len);
      }
  }

  char *endsect = "}\n";
  copy_to_buf(buf, endsect, strlen(endsect));

  return ResultCode_Ok;
}

ResultMessage load_enclave(CommandMessage m) {
  sm_id id;
  ResultCode res;
  unsigned char* buf = m->message->payload;
  uint16_t size = m->message->size;

  LOG_DEBUG("Buffer size: %d\n", size);

  // Here i use a trick to avoid memory issues after the module is loaded
  // During the loading process, some memory will be allocated using "malloc"
  // and never released, as a result subsequent mallocs of big size (e.g. new
  // modules) could fail, because no contiguous memory could be found
  // The graphical example is the following:
  // Suppose that we have 10K memory and buf is 5K
  // [BBBBB-----]
  // Then, for the new module, 1K is allocated
  // [BBBBBM----]
  // After that, the buffer is released
  // [-----M----]
  // Now, if a new module arrives and has size 6K, it cannot be loaded because
  // there isn't enough contiguous memory and malloc() will fail!
  //
  // Fix:
  // If we can, we allocate memory to reach a minimum size of 8K, in order to
  // keep enough space for the next modules.
  // So, we temporarily allocate memory to reach 8K if the module is smaller
  // From the previous example, we would have:
  // [BBBBBAAA--]
  // This way, we force the load_sm to allocate memory in the last portion of
  // the RAM.
  // [BBBBBAAAM-]
  // After the memory is freed, we would have this situation:
  // [--------M-]
  // As you can see, this time we would have enough memory to load the second module!
  //
  // We didn't consider other permanent allocations in this example
  // e.g. connections, periodic events. However, these allocations will fill the
  // start of the RAM, leaving the "middle" part (the largest) empty.
  void *dummy = NULL;
  if(size < RAM_TO_FILL) {
    dummy = malloc_aligned(RAM_TO_FILL - size);
    LOG_DEBUG("Allocating additional %d bytes at %x\n", RAM_TO_FILL - size, (unsigned int)dummy);
  }


  ParseState* state = create_parse_state(buf, size);
  res = load_sm_from_buffer(state, &id);
  free_parse_state(state);
  destroy_command_message(m);

  if(dummy != NULL) {
    free(dummy);
  }

  if(res != ResultCode_Ok) {
    // error in load_sm_from_buffer
    return RESULT(res);
  }

  // prepare response
  unsigned char *response = malloc_aligned(LOAD_SM_RESPONSE_BUFFER);
  if(response == NULL) {
    LOG_ERROR("OOM\n");
    return RESULT(ResultCode_InternalError);
  }
  unsigned char *res_ptr = response;

  // add sm ID to buf
  uint16_t id_net = htons(id);
  copy_to_buf(&res_ptr, &id_net, 2);

  size_t num_symbols = symtab_get_num_symbols();

  res = read_global_symbols(&res_ptr, num_symbols);
  if(res != ResultCode_Ok) {
    free(response);
    return RESULT(res);
  }

  res = read_module_sections(&res_ptr, id, num_symbols);
  if(res != ResultCode_Ok) {
    free(response);
    return RESULT(res);
  }

  // everything went good
  size_t response_size = res_ptr - response;
  LOG_DEBUG("Response size: %d\n", response_size);
  return RESULT_DATA(ResultCode_Ok, response_size, response);
}


static int is_local_connection(Connection* connection) {
  return (int) connection->local;
}

static void handle_local_connection(Connection* connection,
                      void* data, size_t len) {
    reactive_handle_input(connection->to_sm, connection->conn_id, data, len);
    free(data);
}

static void handle_remote_connection(Connection* connection,
                                     void* data, size_t len) {

    unsigned char *payload = malloc_aligned(len + 4);
    if(payload == NULL) {
      LOG_WARNING("OOM\n");
      return;
    }

    uint16_t sm_id = htons(connection->to_sm);
    uint16_t conn_id = htons(connection->conn_id);

    memcpy(payload, &sm_id, 2);
    memcpy(payload + 2, &conn_id, 2);
    memcpy(payload + 4, data, len);

    CommandMessage m = create_command_message(
            CommandCode_RemoteOutput,
            create_message(len + 4, payload));

    write_command_message(m, (unsigned char *) connection->to_address.u8, connection->to_port);

    destroy_command_message(m);
    free(data);
}


void reactive_handle_output(uint16_t conn_id, void* data, size_t len) {
  #if USE_MINTIMER && TIMING_TESTS
    start_time = mintimer_now_usec();
  #endif

  Connection* connection = connections_get(conn_id);

  if (connection == NULL) {
      LOG_DEBUG("no connection for id %u\n", conn_id);
      return;
  }

  LOG_DEBUG("accepted output %u to be delivered at %u.%u.%u.%u:%u %u\n",
      connection->conn_id,
      connection->to_address.u8[0], connection->to_address.u8[1],
      connection->to_address.u8[2], connection->to_address.u8[3],
      connection->to_port,
      connection->to_sm);

  if (is_local_connection(connection))
      handle_local_connection(connection, data, len);
  else
      handle_remote_connection(connection, data, len);
}


void reactive_handle_input(sm_id sm, conn_index conn_id, void* data, size_t len) {
    LOG_DEBUG("Calling handle_input of sm %d\n", sm);

    // we assume that the conn_idx has already been stored on map
    // even if this is not the case, handle_input would simply fail if the ID is
    // not correct (either because out of bounds, or due to a decryption error)
    uint16_t conn_idx = connection_idxs[conn_id];
    uint16_t args[] = {conn_idx, (uint16_t)data, (uint16_t)len};
    uint16_t retval = 0;

    if (!sm_call_id(sm, Entrypoint_HandleInput, args,
            sizeof(args) / sizeof(args[0]), &retval) || retval != 0) {
        LOG_ERROR("handle_input failed: %d\n", retval);
    }

    #if USE_MINTIMER && TIMING_TESTS
      LOG_DEBUG("Elapsed time: %lu us\n", mintimer_now_usec() - start_time);
    #endif
}


ResultMessage handle_set_key(sm_id id, ParseState *state) {
  uint8_t* ad;
  const size_t AD_LEN = sizeof(conn_index) + sizeof(io_index) + 2;
  if (!parse_raw_data(state, AD_LEN, &ad))
      return RESULT(ResultCode_IllegalPayload);

  // check if we can store the conn_idx of this connection ID on connection_idxs
  uint16_t conn_id = (ad[0] << 8) | ad[1];
  if(conn_id >= MAX_CONNECTIONS) {
    LOG_ERROR("connection_idxs too small\n");
    return RESULT(ResultCode_InternalError);
  }

  uint8_t* cipher;
  if (!parse_raw_data(state, SANCUS_KEY_SIZE, &cipher))
      return RESULT(ResultCode_IllegalPayload);

  uint8_t* tag;
  if (!parse_raw_data(state, SANCUS_TAG_SIZE, &tag))
      return RESULT(ResultCode_IllegalPayload);

  uint16_t args[] = {(uint16_t)ad, (uint16_t)cipher, (uint16_t)tag, (uint16_t) &connection_idxs[conn_id]};

  LOG_DEBUG("Calling set_key on module %d\n", id);
  uint16_t retval = 0;

  if (!sm_call_id(id, Entrypoint_SetKey, args, sizeof(args) / sizeof(args[0]),
                  &retval) || retval != 0) {
      LOG_ERROR("set_key failed: %d\n", retval);
      return RESULT(ResultCode_InternalError);
  }

  return RESULT(ResultCode_Ok);
}


ResultMessage handle_attest(sm_id id, ParseState *state) {
  uint16_t challenge_len;
  if (!parse_int(state, &challenge_len))
      return RESULT(ResultCode_IllegalPayload);

  uint8_t* challenge;
  if (!parse_raw_data(state, challenge_len, &challenge))
      return RESULT(ResultCode_IllegalPayload);

  // The result format is [tag] where the tag is the challenge response
  const size_t RESULT_PAYLOAD_SIZE = SANCUS_TAG_SIZE;
  void* result_payload = malloc_aligned(RESULT_PAYLOAD_SIZE);

  if (result_payload == NULL)
      return RESULT(ResultCode_InternalError);

  uint16_t args[] = {(uint16_t)challenge, challenge_len, (uint16_t)result_payload};

  LOG_DEBUG("Calling attest on module %d\n", id);
  if (!sm_call_id(id, Entrypoint_Attest, args, sizeof(args) / sizeof(args[0]),
                  /*retval=*/NULL)) {
      LOG_ERROR("attest failed\n");
      free(result_payload);
      return RESULT(ResultCode_InternalError);
  }

  return RESULT_DATA(ResultCode_Ok, RESULT_PAYLOAD_SIZE, result_payload);
}

ResultMessage handle_disable(sm_id id, ParseState *state) {
  uint8_t* ad;
  const size_t AD_LEN = 2;
  if (!parse_raw_data(state, AD_LEN, &ad))
      return RESULT(ResultCode_IllegalPayload);

  uint8_t* cipher;
  if (!parse_raw_data(state, 2, &cipher))
      return RESULT(ResultCode_IllegalPayload);

  uint8_t* tag;
  if (!parse_raw_data(state, SANCUS_TAG_SIZE, &tag))
      return RESULT(ResultCode_IllegalPayload);

  uint16_t args[] = {(uint16_t)ad, (uint16_t)cipher, (uint16_t)tag};

  LOG_DEBUG("Calling disable on module %d\n", id);
  uint16_t retval = 0;

  if (!sm_call_id(id, Entrypoint_Disable, args, sizeof(args) / sizeof(args[0]),
                  &retval) || retval != 0) {
      LOG_ERROR("disable failed: %d\n", retval);
      return RESULT(ResultCode_InternalError);
  }

  return RESULT(ResultCode_Ok);
}

ResultMessage handle_user_entrypoint(sm_id id, uint16_t index, ParseState *state) {
  uint8_t* payload;
  size_t payload_len;
  if(!parse_all_raw_data(state, &payload, &payload_len)) {
    return RESULT(ResultCode_IllegalPayload);
  }

  uint16_t args_buf[] = {(uint16_t)payload, payload_len};
  uint16_t* args = NULL;
  size_t nargs = 0;

  if (payload_len > 0) {
     args = args_buf;
     nargs = 2;
  }

  if (!sm_call_id(id, index, args, nargs, NULL))
     return RESULT(ResultCode_InternalError);

  return RESULT(ResultCode_Ok);
}

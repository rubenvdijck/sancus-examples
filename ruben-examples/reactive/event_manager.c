#include "event_manager.h"

#include <stdio.h>

#include "msg.h"
#include "log.h"

#include "networking.h"
#include "command_handlers.h"

ResultMessage process_message(CommandMessage m) {
  switch (m->code) {
    case CommandCode_AddConnection:
      return handler_add_connection(m);

    case CommandCode_CallEntrypoint:
      return handler_call_entrypoint(m);

    case CommandCode_RemoteOutput:
      handler_remote_output(m);
      return NULL; // no response by default, to avoid races with the UART

    case CommandCode_LoadSM:
      return handler_load_sm(m);

    case CommandCode_Ping:
      return handler_ping(m);

    case CommandCode_RegisterEntrypoint:
      return handler_register_entrypoint(m);

    default: // CommandCode_Invalid
      LOG_WARNING("[EM] wrong cmd id\n");
      return NULL;
  }
}


void event_manager_run(kernel_pid_t uart_pid) {
  msg_t msg;

  LOG_DEBUG("[EM] Running\n");

  while(1) {
    // wait for messages
    msg_receive(&msg);

    // The CommandMessage will be freed by its handler. This because we want
    // to release memory as soon as we don't need it anymore, in order to keep
    // the lowest possible addresses for future allocations.
    CommandMessage m = (CommandMessage) msg.content.ptr;

    LOG_DEBUG("[EM] received command\n");

    ResultMessage res = process_message(m);

    if(res != NULL) {
      if(msg.sender_pid == uart_pid) {
        write_result_message(res);
      }
      destroy_result_message(res);
    }
  }
}

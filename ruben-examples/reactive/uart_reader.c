#include "uart_reader.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "log.h"

#include "networking.h"
#include "utils.h"

UartReaderData create_uart_reader_data(kernel_pid_t pid) {
  UartReaderData data = malloc_aligned(sizeof(*data));
  data->pid = pid;

  return data;
}


void destroy_uart_reader_data(UartReaderData data) {
  free(data);
}


void uart_reader_run(void *arg) {
  UartReaderData data = (UartReaderData) arg;

  // free data
  kernel_pid_t pid = data->pid;
  destroy_uart_reader_data(data);

  LOG_DEBUG("[UR] Running\n");

  while(1) {
    // handshake: wait for a new transmission
    handshake();

    // read a command
    CommandMessage m = read_command_message();

    LOG_DEBUG("[UR] Read command\n");

    // send to queue
    msg_t msg;
    msg.content.ptr = m;
    if(msg_send(&msg, pid) != 1) {
      LOG_WARNING("[UR] send msg fail\n");
      destroy_command_message(m);
    }
  }
}

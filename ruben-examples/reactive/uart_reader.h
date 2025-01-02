#ifndef __UART_READER_H__
#define __UART_READER_H__

#include "mutex.h"
#include "thread.h"

typedef struct reader {
  kernel_pid_t pid;
} *UartReaderData;

UartReaderData create_uart_reader_data(kernel_pid_t pid);
void destroy_uart_reader_data(UartReaderData data);

void uart_reader_run(void *arg);


#endif

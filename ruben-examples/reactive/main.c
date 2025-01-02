#include <stdio.h>
#include <stdlib.h>

#include "thread.h"
#include "log.h"

#include "networking.h"
#include "uart_reader.h"
#include "event_manager.h"

#include <sancus_support/sm_control.h>

#if USE_MMIO_LED
  #include "pmodled.h"
  #include "led_driver.h"
#endif

#if USE_MMIO_BTN
  #include "pmodbtn.h"
  #include "button_driver.h"
#endif

#define QUEUE_SIZE 16

char uart_thread_stack[THREAD_STACKSIZE_MEDIUM];

#if USE_PERIODIC_EVENTS
  #include "event_generator.h"
  char event_generator_thread_stack[THREAD_STACKSIZE_MEDIUM];
#endif

msg_t queue[QUEUE_SIZE];

int main(void)
{
    LOG_DEBUG("Using periodic events: %d\n", USE_PERIODIC_EVENTS);
    LOG_DEBUG("Using mintimer module: %d\n", USE_MINTIMER);

    #if USE_MMIO_LED
      if(!sancus_enable(&led_driver)) {
        LOG_ERROR("led_driver fail 1\n");
        return -1;
      }
      if(!sm_register_existing(&led_driver)) {
        LOG_ERROR("led_driver fail 2\n");
        return -1;
      }

      if(!sancus_enable(&pmodled)) {
        LOG_ERROR("led_driver fail 3\n");
        return -1;
      }

      LOG_DEBUG("LED initialized\n");
    #endif

    #if USE_MMIO_BTN
      if(!sancus_enable(&button_driver)) {
        LOG_ERROR("button_driver fail 1\n");
        return -1;
      }
      if(!sm_register_existing(&button_driver)) {
        LOG_ERROR("button_driver fail 2\n");
        return -1;
      }

      if(!sancus_enable(&pmodbtn)) {
        LOG_ERROR("button_driver fail 3\n");
        return -1;
      }

      LOG_DEBUG("Button initialized\n");
    #endif

    // initialize queue
    msg_init_queue(queue, QUEUE_SIZE);

    kernel_pid_t my_pid = thread_getpid();

    // create data for UART thread
    UartReaderData uart_data = create_uart_reader_data(my_pid);

    // set priority
    #if USE_MINTIMER
      uint8_t uart_priority = THREAD_PRIORITY_MAIN - 1;
    #else
      uint8_t uart_priority = THREAD_PRIORITY_MAIN;
    #endif

    // start UART thread
    // TODO if we use xtimer module we should assign an higher priority to uart_reader,
    // otherwise we could lose some bytes during UART transmission
    kernel_pid_t uart_pid = thread_create(uart_thread_stack, sizeof(uart_thread_stack),
                            uart_priority, 0,
                            (void *) uart_reader_run, uart_data, "uartReader");

    // If we use this other thread, we start to get weird runtime bugs
    //
    // 1. Randomly the deployment of a module fails (maybe because this thread
    //    interferes with the UART thread, and some bytes are lost)
    // 2. sm_call_id() sometimes provokes a weird bug that eventually freezes
    //    the execution and there is nothing we can do but reset the board.
    //    I don't have a good explanation for this one. If we comment sm_call_id
    //    the execution continues normally. Anyway, I encountered issues
    //    with other Sancus functions as well, e.g. if somewhere in the code
    //    malloc() returns an odd pointer, the crypto functions don't work
    //    properly anymore. Weird.
#if USE_PERIODIC_EVENTS
    // create data for Event Generator thread
    EventGeneratorData eg_data = create_event_generator_data(my_pid);

    // set priority
    #if USE_MINTIMER
      uint8_t event_gen_priority = THREAD_PRIORITY_MAIN + 1;
    #else
      uint8_t event_gen_priority = THREAD_PRIORITY_MAIN;
    #endif

    // start Event Generator thread
    thread_create(event_generator_thread_stack, sizeof(event_generator_thread_stack),
                            event_gen_priority, 0,
                            (void *) event_generator_run, eg_data, "eventGenerator");
#endif

    // start event manager
    event_manager_run(uart_pid);

    return 0;
}

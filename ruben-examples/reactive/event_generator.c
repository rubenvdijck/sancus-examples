#include "event_generator.h"

#include <stdlib.h>
#include <string.h>

#include "msg.h"
#include "byteorder.h"

#include "periodic_event.h"
#include "utils.h"
#include "log.h"
#include "networking.h"

#if USE_MINTIMER
  #include "mintimer.h"
#else
  #include "thread.h"
#endif

EventGeneratorData create_event_generator_data(kernel_pid_t pid) {
  EventGeneratorData data = malloc_aligned(sizeof(*data));
  data->pid = pid;

  return data;
}


void destroy_event_generator_data(EventGeneratorData data) {
  free(data);
}


void event_generator_run(void *arg) {
  EventGeneratorData data = (EventGeneratorData) arg;
  PeriodicEventLink *l;

  // free data
  kernel_pid_t pid = data->pid;
  destroy_event_generator_data(data);

  LOG_DEBUG("[EG] Running\n");

  while(1) {
    // phase 1: get callable tasks
    PeriodicEventLink *tasks = get_callable_tasks();

    // phase 2: for each of them, send a message to the event manager
    for(l = tasks; l != NULL; l = l->next) {
      msg_t msg;
      unsigned char *payload = malloc_aligned(4);
      if(payload == NULL) {
        LOG_WARNING("OOM\n");
        continue;
      }

      uint16_t sm_id = htons(l->event->module);
      uint16_t entry = htons(l->event->entry);

      memcpy(payload, &sm_id, 2);
      memcpy(payload + 2, &entry, 2);

      CommandMessage m = create_command_message(
              CommandCode_CallEntrypoint,
              create_message(4, payload));

      msg.content.ptr = m;
      if(msg_send(&msg, pid) != 1) {
        LOG_WARNING("[EG] sending fail\n");
        destroy_command_message(m);
      }
    }

    // phase 3: dispose tasks and sleep
    dispose_callable_tasks(tasks);
    #if USE_MINTIMER
      mintimer_usleep(BASE_FREQUENCY_US);
    #else
      thread_yield();
    #endif
  }
}

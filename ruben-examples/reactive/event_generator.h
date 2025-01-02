#ifndef __EVENT_GENERATOR_H__
#define __EVENT_GENERATOR_H__

#include "thread.h"

typedef struct generator {
  kernel_pid_t pid;
} *EventGeneratorData;

EventGeneratorData create_event_generator_data(kernel_pid_t pid);
void destroy_event_generator_data(EventGeneratorData data);

void event_generator_run(void *arg);

#endif

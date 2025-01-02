#include "periodic_event.h"

#include <stdlib.h>

#include "mutex.h"
#include "log.h"

#include "utils.h"

static mutex_t mutex = MUTEX_INIT;

typedef struct PeriodicEventNode
{
    PeriodicEvent event;
    struct PeriodicEventNode* next;
} PeriodicEventNode;

static PeriodicEventNode* events_head = NULL;

int increment_counter_event(PeriodicEvent *event) {
  event->counter += BASE_FREQUENCY;

  if (event->counter >= event->frequency) {
      event->counter = 0;
      return 1;
  }
  else {
      return 0;
  }
}

int periodic_event_add(PeriodicEvent* event) {
  PeriodicEventNode* node = malloc_aligned(sizeof(PeriodicEventNode));

  if (node == NULL) {
    return 0;
  }

  node->event = *event;

  mutex_lock(&mutex);
  node->next = events_head;
  events_head = node;
  mutex_unlock(&mutex);
  return 1;
}

PeriodicEventLink* get_callable_tasks(void) {
  PeriodicEventLink *callable_tasks = NULL;
  PeriodicEventNode *l;

  mutex_lock(&mutex);
  for(l = events_head; l != NULL; l = l->next) {
      int is_callable = increment_counter_event(&l->event);

      if(is_callable) {
        // add to local list
        PeriodicEventLink* link = malloc_aligned(sizeof(PeriodicEventLink));

        if (link == NULL) {
          LOG_ERROR("OOM");
          dispose_callable_tasks(callable_tasks);
          return NULL;
        }

        link->event = &l->event;
        link->next = callable_tasks;
        callable_tasks = link;
      }
  }
  mutex_unlock(&mutex);

  return callable_tasks;
}

void dispose_callable_tasks(PeriodicEventLink *head) {
  while(head != NULL) {
    PeriodicEventLink *tmp = head;
    head = head->next;
    free(tmp);
  }
}

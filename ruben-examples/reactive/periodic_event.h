#ifndef __PERIODIC_EVENT_H__
#define __PERIODIC_EVENT_H__

#include <stdint.h>

#include <sancus/sm_support.h>

#define BASE_FREQUENCY 100 // ms
#define BASE_FREQUENCY_US 1e5 // us

typedef struct
{
    sm_id         module;
    uint16_t      entry;
    uint32_t      frequency;
    uint32_t      counter;
} PeriodicEvent;

typedef struct PeriodicEventLink {
  PeriodicEvent *event;
  struct PeriodicEventLink *next;
} PeriodicEventLink;

// Copies event so may be stack allocated.
int periodic_event_add(PeriodicEvent* event);

// creates a sublist of all the periodic events, containing only pointers to
// those events that are ready to be called
PeriodicEventLink* get_callable_tasks(void);

// release memory allocated for the temporary list of callable tasks
void dispose_callable_tasks(PeriodicEventLink *head);


#endif

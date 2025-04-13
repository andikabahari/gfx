#include "event.h"

#include "memory.h"
#include "array.h"
#include "log.h"

typedef struct {
    void          *listener;
    Event_Handler  callback;
} Event;

// TODO: should we change it?
// 
// Raylib is also using 16384 for max events that can be handled. It seems that
// people use 16384 because it's a power of two (2^14) and align with binary memory
// allocation stuff. But I'm still wondering why people specifically use the number!?
#define MAX_EVENT_LIST 16384

static struct {
    Event *events;
} list[MAX_EVENT_LIST];

static bool initialized = false;

void event_init()
{
    if (!initialized) {
        list->events = NULL;
        memory_zero(list, sizeof(list));
        initialized = true;
    } else {
        LOG_WARNING("Event list is already initialized\n");
    }
}

void event_destroy()
{
    if (!initialized) {
        LOG_WARNING("Event list is not initialized yet\n");
        return;
    }

    for (size_t i = 0; i < MAX_EVENT_LIST; ++i) {
        if (list[i].events != NULL) {
            array_destroy(list[i].events);
            list[i].events = NULL;
        }
    }
}

bool event_register(int type, void *listener, Event_Handler callback)
{
    if (!initialized) {
        LOG_WARNING("Event list is not initialized yet\n");
        return false;
    }

    if (list[type].events == NULL) {
        list[type].events = array_create(Event);
    }

    size_t n = array_length(list[type].events);
    for (size_t i = 0; i < n; ++i) {
        Event e = list[type].events[i];
        if (e.listener == listener) return false;
    }

    Event event = {
        .listener = listener,
        .callback = callback,
    };
    array_push(list[type].events, event);

    return true;
}

bool event_unregister(Event_Type type, void *listener, Event_Handler callback)
{
    if (!initialized) {
        LOG_WARNING("Event list is not initialized yet\n");
        return false;
    }

    if (list[type].events == NULL) return false;

    size_t n = array_length(list[type].events);
    for (size_t i = 0; i < n; ++i) {
        Event e = list[type].events[i];
        if (e.listener == listener && e.callback == callback) {
            Event unreg;
            array_pop_at(list[type].events, i, &unreg);
            return true;
        }
    }

    return false;
}

bool event_dispatch(Event_Type type, Event_Context ctx)
{
    if (!initialized) {
        LOG_WARNING("Event list is not initialized yet\n");
        return false;
    }

    if (list[type].events == NULL) return false;

    size_t n = array_length(list[type].events);
    for (size_t i = 0; i < n; ++i) {
        Event e = list[type].events[i];
        if (e.callback(type, e.listener, ctx)) return true;
    }

    return false;
}
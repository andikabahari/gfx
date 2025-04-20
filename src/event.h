#ifndef EVENT_H
#define EVENT_H

#include "common.h"

typedef struct {
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;
} Event_Context;

typedef enum {
    EVENT_EXIT = 0,

    // Keyboard
    EVENT_KEY_PRESSED,
    EVENT_KEY_RELEASED,

    // Mouse
    EVENT_MOUSE_BUTTON_PRESSED,
    EVENT_MOUSE_BUTTON_RELEASED,

    MAX_EVENT_TYPES
} Event_Type;

typedef bool (*Event_Handler)(int event_type, void *listener, Event_Context ctx);

void event_init();
void event_destroy();

bool event_register(Event_Type type, void *listener, Event_Handler callback);
bool event_unregister(Event_Type type, void *listener, Event_Handler callback);

bool event_dispatch(Event_Type type, Event_Context ctx);

#endif
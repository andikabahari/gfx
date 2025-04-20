#include "input.h"

#include "common.h"
#include "memory.h"
#include "event.h"
#include "log.h"

typedef struct {
    bool keys[MAX_INPUT_KEYS];
    bool mouse_buttons[MAX_INPUT_MOUSE_BUTTONS];
} Input;

static union {
    Input prev;
    Input current;
} state;

static bool initialized = false;

void input_init()
{
    if (!initialized) {
        memory_zero(&state, sizeof(state));
        initialized = true;
    } else {
        LOG_WARNING("Input is already initialized\n");
    }
}

void input_destroy()
{
    initialized = false;
}

void input_update()
{
    if (!initialized) return;

    memory_copy(&state.prev.keys, &state.current.keys, sizeof(state.prev.keys));
    memory_copy(&state.prev.mouse_buttons, &state.current.mouse_buttons, sizeof(state.prev.mouse_buttons));
}

static bool valid_key(Input_Key key)
{
    return key > 0 && key < MAX_INPUT_KEYS;
}

void input_process_key(Input_Key key, bool pressed)
{
    if (!initialized || !valid_key(key)) return;

    if (state.current.keys[key] != pressed) {
        state.current.keys[key] = pressed;

        Event_Context ctx = {0};
        ctx.data.u16[0] = key;
        event_dispatch(pressed ? EVENT_KEY_PRESSED : EVENT_KEY_RELEASED, ctx);
    }
}

bool input_is_key_down(Input_Key key)
{
    bool ret = false;
    
    if (initialized && valid_key(key)) {
        if (state.current.keys[key] == true) ret = true;
    }

    return ret;
}

bool input_is_key_up(Input_Key key)
{
    bool ret = false;
    
    if (initialized && valid_key(key)) {
        if (state.current.keys[key] == false) ret = true;
    }

    return ret;
}

static bool valid_mouse_button(Input_Mouse_Button mb)
{
    return mb > 0 && mb < MAX_INPUT_MOUSE_BUTTONS;
}

void input_process_mouse_button(Input_Mouse_Button mb, bool pressed)
{
    if (!initialized || !valid_mouse_button(mb)) return;

    if (state.current.mouse_buttons[mb] != pressed) {
        state.current.mouse_buttons[mb] = pressed;

        Event_Context ctx = {0};
        ctx.data.u16[0] = mb;
        event_dispatch(pressed ? EVENT_MOUSE_BUTTON_PRESSED : EVENT_MOUSE_BUTTON_RELEASED, ctx);
    }
}

bool input_is_mouse_button_down(Input_Mouse_Button mb)
{
    bool ret = false;
    
    if (initialized && valid_mouse_button(mb)) {
        if (state.current.mouse_buttons[mb] == true) ret = true;
    }

    return ret;
}

bool input_is_mouse_button_up(Input_Mouse_Button mb)
{
    bool ret = false;
    
    if (initialized && valid_mouse_button(mb)) {
        if (state.current.mouse_buttons[mb] == false) ret = true;
    }

    return ret;
}

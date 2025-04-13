#ifndef INPUT_H
#define INPUT_H

#include "types.h"

void input_init();
void input_destroy();
void input_update();

// TODO: add more keys!
typedef enum {
    KEY_NONE = 0,

    KEY_ESC = 27,
    KEY_SPACE = 32,

    // Numbers (Top row)
    KEY_0 = 48,
    KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    // Letters
    KEY_A = 65,
    KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
    KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
    KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S,
    KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
} Input_Key;

#define MAX_INPUT_KEYS 512

void input_process_key(Input_Key key, bool pressed);
bool input_is_key_down(Input_Key key);
bool input_is_key_up(Input_Key key);

// TODO: add more buttons, maybe?
typedef enum {
    INPUT_MOUSE_BUTTON_NONE = 0,

    INPUT_MOUSE_BUTTON_LEFT,
    INPUT_MOUSE_BUTTON_MIDDLE,
    INPUT_MOUSE_BUTTON_RIGHT,

    MAX_INPUT_MOUSE_BUTTONS
} Input_Mouse_Button;

void input_process_mouse_button(Input_Mouse_Button mb, bool pressed);
bool input_is_mouse_button_down(Input_Mouse_Button mb);
bool input_is_mouse_button_up(Input_Mouse_Button mb);

#endif
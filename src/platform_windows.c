#include "platform.h"

#if PLATFORM_WINDOWS

#include <windows.h>
#include <windowsx.h>

#include "common.h"
#include "log.h"
#include "input.h"

typedef struct {
    HINSTANCE h_instance;
    HWND      hwnd;
} Window_Handle;

LRESULT CALLBACK _platform_window_handle_message(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
        case WM_CLOSE:
            // TODO: quit app implementation
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
            input_process_key((int)w_param, msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            input_process_mouse_button(INPUT_MOUSE_BUTTON_LEFT, msg == WM_LBUTTONDOWN);
            break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            input_process_mouse_button(INPUT_MOUSE_BUTTON_MIDDLE, msg == WM_MBUTTONDOWN);
            break;
        
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            input_process_mouse_button(INPUT_MOUSE_BUTTON_RIGHT, msg == WM_RBUTTONDOWN);
            break;

        default:
            return DefWindowProcA(hwnd, msg, w_param, l_param);
    }

    return 0;
}

void platform_window_handle_message(Platform_Window *window)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void platform_window_init(Platform_Window *window, const char *title, int x, int y, int width, int height)
{
    window->handle = malloc(sizeof(Window_Handle));

    Window_Handle *handle = (Window_Handle *)window->handle;
    handle->h_instance = GetModuleHandleA(NULL);

    WNDCLASSA wc = {0};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = _platform_window_handle_message;
    wc.hInstance = handle->h_instance;
    wc.lpszClassName = "BasicWindowClass";
    wc.hIcon = LoadIcon(handle->h_instance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassA(&wc)) {
        LOG_FATAL("Window registration failed\n");
    }

    RECT rect = {0};
    DWORD window_style = 0;
    window_style |= WS_OVERLAPPED;
    window_style |= WS_SYSMENU;
    window_style |= WS_CAPTION;
    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;
    DWORD window_ex_style = WS_EX_APPWINDOW;
    AdjustWindowRectEx(&rect, window_style, FALSE, window_ex_style);

    x += rect.left;
    y += rect.top;
    width += rect.right - rect.left;
    height += rect.bottom - rect.top;

    HWND hwnd = CreateWindowExA(
        window_ex_style, "BasicWindowClass", title, window_style,
        x, y, width, height,
        NULL, NULL, handle->h_instance, NULL);

    if (hwnd == NULL) {
        LOG_FATAL("Window creation failed\n");
    }

    handle->hwnd = hwnd;

    // This is how we want to show the window (e.g. normal, minimized, maximized, etc.)
    ShowWindow(handle->hwnd, SW_SHOWNORMAL);
}

void platform_window_destroy(Platform_Window *window)
{
    Window_Handle *handle = (Window_Handle *)window->handle;
    
    if (handle->hwnd) {
        DestroyWindow(handle->hwnd);
        handle->hwnd = NULL;
    }

    free(handle);
}

void platform_log_output(Log_Level level, const char *msg)
{
    HANDLE handle = level < LOG_LEVEL_WARNING
        ? GetStdHandle(STD_OUTPUT_HANDLE)
        : GetStdHandle(STD_ERROR_HANDLE);

    // Color order: info, debug, warning, error, fatal
    WORD colors[5] = {
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        FOREGROUND_BLUE,
        FOREGROUND_RED | FOREGROUND_GREEN,
        FOREGROUND_RED,
        FOREGROUND_RED | FOREGROUND_INTENSITY,
    };
    SetConsoleTextAttribute(handle, colors[level]);
    LPDWORD chars_written = 0;
    WriteConsoleA(handle, msg, (DWORD)strlen(msg), chars_written, 0);

    OutputDebugStringA(msg);
}

#endif
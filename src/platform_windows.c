#include "platform.h"

#if PLATFORM_WINDOWS

#include <windows.h>
#include <windowsx.h>

typedef struct {
    HINSTANCE h_instance;
    HWND hwnd;
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
        default:
            return DefWindowProcA(hwnd, msg, w_param, l_param);
    }

    return 0;
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
        LOG_ERROR("Window registration failed");
        exit(1);
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
        LOG_ERROR("Window creation failed");
        exit(1);
    }

    handle->hwnd = hwnd;

    LOG_INFO("Showing application window\n");
    // This is how we want to show the window (e.g. normal, minimized, maximized, etc.)
    ShowWindow(handle->hwnd, SW_SHOWNORMAL);

    for (;;) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
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

    // Color order: info, warning, error
    WORD colors[3] = {
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,      // White
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // Yellow
        FOREGROUND_RED | FOREGROUND_INTENSITY,                    // Red
    };
    SetConsoleTextAttribute(handle, colors[level]);
    LPDWORD chars_written = 0;
    WriteConsoleA(handle, msg, (DWORD)strlen(msg), chars_written, 0);

    OutputDebugStringA(msg);
}

#endif
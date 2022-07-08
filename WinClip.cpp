#include <Windows.h>
#include <iostream>
#include "RWClip.h"
bool g_AddedListener = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)   {

    // Register the window class.
    LPSTR CLASS_NAME = "Window Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                           // Optional window styles.
        CLASS_NAME,                  // Window class
        "", // Window text
        WS_OVERLAPPEDWINDOW,         // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,      // Parent window
        NULL,      // Menu
        hInstance, // Instance handle
        NULL       // Additional application data
    );

    if (hwnd == NULL)   {
        return 0;
    }


    // Run the message loop.
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)   {
         case WM_CREATE:
            //g_hwndNext = ::SetClipboardViewer(hwnd);
            g_AddedListener = ::AddClipboardFormatListener(hwnd);
            return g_AddedListener ? 0 : -1;

        case WM_DESTROY:
            
            if (g_AddedListener)    {
                RemoveClipboardFormatListener(hwnd);
                g_AddedListener = false;
            }
            return 0;

        case WM_CLIPBOARDUPDATE:
            ReadIMGClip();
            ReadUTFClip();    
            return 0;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

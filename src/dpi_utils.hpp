#pragma once
#include <windows.h>

void EnablePerMonitorDpiAwareness();
unsigned int GetSystemDpi();
unsigned int GetWindowDpi(HWND hwnd);
int ScaleForDpi(int value, unsigned int dpi);
SIZE GetWindowSizeForClientDpi(int clientWidth, int clientHeight, DWORD style, DWORD exStyle, unsigned int dpi);
void ResizeWindowForClientDpi(HWND hwnd, int clientWidth, int clientHeight, unsigned int dpi);

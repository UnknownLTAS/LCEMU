#pragma once
#include <windows.h>
#include <dwmapi.h>
#define GUI_NO_CLOSE_WINDOW CS_HREDRAW | CS_VREDRAW | CS_CLASSDC | CS_NOCLOSE

HWND util_CreateWindow(const TCHAR* win_class, const TCHAR* title, const WNDPROC& proc, const COLORREF& Background, const UINT style);
void messageLoop(HWND& hwnd);

bool fixWindowCornor(HWND hwnd);

#pragma once
#include <windows.h>
#define GUI_NO_CLOSE_WINDOW CS_HREDRAW | CS_VREDRAW | CS_CLASSDC | CS_NOCLOSE

HWND UtilCreateWindow(const TCHAR* win_class, const TCHAR* title, const WNDPROC& proc, const COLORREF& Background, const UINT style);
void MessageLoop(const HWND& hwnd);

bool FixWindowCorner(const HWND& hwnd);
std::pair<int, int> CalcWindowSize(const HWND& hwnd, const int& client_w, const int& client_h);

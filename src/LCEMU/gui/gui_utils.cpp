#include "gui_utils.h"
#pragma comment(lib, "Dwmapi.lib")

HWND util_CreateWindow(const TCHAR* win_class, const TCHAR* title, const WNDPROC& proc, const COLORREF& Background, const UINT style)
{
	WNDCLASS wc;
	HWND hwnd;

	wc.style = style;
	wc.lpfnWndProc = proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = (HINSTANCE)GetModuleHandle(0);
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(Background);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = win_class;

	if (!RegisterClass(&wc))
		return NULL;

	hwnd = CreateWindow(win_class, title, WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
						NULL, NULL, wc.hInstance, NULL);

	return hwnd;
}

void messageLoop(HWND& hwnd) {
	MSG msg;
	while (GetMessage(&msg, hwnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

bool fixWindowCornor(HWND hwnd)
{
	const DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DONOTROUND;
	return S_OK == DwmSetWindowAttribute(
					   hwnd,
					   33, // DWMWA_WINDOW_CORNER_PREFERENCE,
					   &corner,
					   sizeof(DWM_WINDOW_CORNER_PREFERENCE));
}
#include "key_sequence.h"
// todo: improve config
using namespace std;

static const int WINDOW_W = 200;
static const int WINDOW_H = 220;
static const int FONT_SIZE = 25;
static const int AHEAD = 5;
static const int X_STEP = 14;
static const int Y_STEP = 22;
static const int Y_START = 11;
static const int X_START = 11;

static const COLORREF COL_NO_KEY = RGB(0x34, 0x34, 0x34);
static const COLORREF COL_KEY = RGB(0xff, 0xff, 0xff);
static const COLORREF COL_FRAME = RGB(0xff, 0xff, 0xff);
static const COLORREF COL_FIN_FRAME = RGB(0xff, 0xff, 0xff);
static const COLORREF COL_NOW_BACK = RGB(0x00, 0x00, 0x33);
static const COLORREF COL_TSTART_BACK = RGB(0x3e, 0x00, 0);
static HWND hwnd;

static int now_pos = -1;
static int now_frame = 0;
static vector<string> now_data;
static const char * all_show;
static string no_show;
static bool is_finished = false;
static int now_tstart = 0;
static bool is_waiting_event = false;

static string get_data(int pos) {
	if (pos < 0)
		return no_show;
	if (pos >= now_data.size())
		return no_show;
	return now_data[pos];
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_PAINT:
	{
		{
			hdc = BeginPaint(hwnd, &ps);
			HFONT hf = CreateFontW(FONT_SIZE, 0, 0, 0, FW_BOLD, 0, 0, 0,
								   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
								   FIXED_PITCH | FF_SWISS, NULL);
			auto fnttmp = SelectObject(hdc, hf);

			int nowy = Y_START;
			auto colback = GetTextColor(hdc);
			auto bkback = GetBkColor(hdc);

			SetBkColor(hdc, 0);
			// inputs
			{
				for (int i = 0; i <=  AHEAD; i++)
				{
					int tar = now_pos + AHEAD - i;
					auto s = get_data(tar);
					if (!now_data.empty()  && (tar == now_tstart || tar + 1 == now_data.size()))
					{
						SetBkColor(hdc, COL_TSTART_BACK);
					}
					else if (AHEAD == i)
					{
						SetBkColor(hdc, COL_NOW_BACK);
					}
					else
					{
						SetBkColor(hdc, 0);
					}
					const char* ap = all_show;
					for (int j = 0, nowx = X_START; *ap; j++, nowx += X_STEP,ap++)
					{
						if (s[j] != ' ')
						{
							SetTextColor(hdc, COL_KEY);
							TextOutA(hdc, nowx, nowy, ap, 1);
						}
						else
						{
							SetTextColor(hdc, COL_NO_KEY);
							TextOutA(hdc, nowx, nowy, ap, 1);
						}
					}
					nowy += Y_STEP;
				}
			}

			if (is_waiting_event)
			{
				SetTextColor(hdc, COL_KEY);
				TextOutA(hdc, X_START, nowy - Y_STEP, "[WAITING...]", lstrlenA("[WAITING...]"));
			}
		
			SetBkColor(hdc, 0);
			if(is_finished) // bitrate reduce
			{ // frame
				nowy += FONT_SIZE - Y_STEP;
				auto st = to_string(now_frame) + "f";
				auto cst = st.c_str();
				nowy += 3;
				if (is_finished)
				{
					SetTextColor(hdc, COL_FIN_FRAME);
					TextOutA(hdc, X_START, nowy, cst, lstrlenA(cst));
				}
				//else
				//{
				//	SetTextColor(hdc, COL_FRAME);
				//	TextOutA(hdc, X_START, nowy, cst, lstrlenA(cst));
				//}
				nowy += FONT_SIZE;
			}
			SetTextColor(hdc, colback);
			SetBkColor(hdc, bkback);
			DeleteObject(SelectObject(hdc, fnttmp));
			EndPaint(hwnd, &ps);
		}
	}
	break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static void window_main()
{
	hwnd = util_CreateWindow(TEXT("Input viewer"), TEXT("Input viewer"), WndProc, 0, GUI_NO_CLOSE_WINDOW);
	if (!hwnd)
		return;
	ShowWindow(hwnd, SW_SHOW);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 110, WINDOW_W, WINDOW_H, SWP_SHOWWINDOW);
	fixWindowCornor(hwnd);
	UpdateWindow(hwnd);
	messageLoop(hwnd);

}

void keyseq_set(std::vector<std::string>& d, int tstart) {
	now_data = d;
	now_tstart = tstart;
	keyseq_reset();
}

void keyseq_reset()
{
	now_pos = -1;
	now_frame = 0;
	is_finished = false;
	InvalidateRect(hwnd, NULL, TRUE);
}

void keyseq_on_frame(const MacroManager& mgr)
{
	now_frame = mgr.get_now_frame_count();
	now_pos = mgr.get_last_input_index();
	is_finished = !mgr.is_running();
	is_waiting_event = mgr.is_waiting_event();
	InvalidateRect(hwnd, NULL, FALSE);
}


void keyseq_init(string&all)
{
	all_show = all.c_str();
	no_show.resize(all.size(), ' ');
	std::thread _t(window_main);
	_t.detach();
}

void keyseq_set_window_pos(int x, int y) {
	SetWindowPos(hwnd, HWND_TOPMOST, x, y, WINDOW_W, WINDOW_H, SWP_SHOWWINDOW);
}
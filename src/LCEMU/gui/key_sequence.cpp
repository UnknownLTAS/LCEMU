#include "key_sequence.h"
#include "gui_utils.h"
#include <string>
#include <vector>
#include <thread>

// todo: improve config
using namespace std;

static constexpr int WINDOW_W = 200;
static int window_height = 220; // dummy
static constexpr int FONT_SIZE = 25;
static constexpr int X_STEP = 14;
static constexpr int Y_STEP = 22;
static constexpr int DPAD_BOX_SIZE = X_STEP;
static constexpr int Y_START = 11;
static constexpr int X_START = 11;
static constexpr int Y_END_PADDING = 2;
static constexpr int PAD_LINE_HEIGHT = 1;

static constexpr COLORREF COL_NO_KEY = RGB(0x34, 0x34, 0x34);
static constexpr COLORREF COL_KEY = RGB(0xff, 0xff, 0xff);
static constexpr COLORREF COL_FRAME = RGB(0xcc, 0xff, 0xff);
static constexpr COLORREF COL_FIN_FRAME = RGB(0xff, 0xff, 0xff);
static constexpr COLORREF COL_NOW_BACK = RGB(0x00, 0x00, 0x33);
static constexpr COLORREF COL_START_END_BACK = RGB(0x3e, 0x00, 0);

static constexpr COLORREF COL_PAD_LINE = RGB(0x33, 0x33, 0xbe);

// static constexpr COLORREF COL_PAD_ON = RGB(0xff, 0, 0);
static constexpr COLORREF COL_PAD_ON = COL_KEY;
static constexpr COLORREF COL_PAD_OFF = COL_NO_KEY;

static HWND hwnd;

static MacroManager* mgrp;

static constexpr int SHOW_FRAME_MODE_NONE = 0;
static constexpr int SHOW_FRAME_MODE_PART = 1;
static constexpr int SHOW_FRAME_MODE_FULL = 2;

static constexpr int SHOW_MODE_LINE = 0;
static constexpr int SHOW_MODE_PAD = 1;
// default cfgs
static int cfg_show_next = 3;
static int cfg_show_mode = SHOW_MODE_PAD;
static int cfg_show_frames = SHOW_FRAME_MODE_NONE;

/*
L..........R
..^......X..
.<.>.-+.Y.A.
..v......B..
*/
// row, column, ID, char
static const tuple<int, int, VPAD_IDS, const char*> PAD_DATA[] = {
	{0, 0, VPAD_L, "L"},
	{0, 11, VPAD_R, "R"},

	{1, 9, VPAD_X, "X"},


	{2, 5, VPAD_MINUS, "-"},
	{2, 6, VPAD_PLUS, "+"},
	{2, 8, VPAD_Y, "Y"},
	{2, 10, VPAD_A, "A"},

	{3, 9, VPAD_B, "B"}
};

// row, column, ID
static const tuple<int, int, VPAD_IDS> DPAD_DATA[]{
	{1, 2, VPAD_UP},
	{2, 1, VPAD_LEFT},
	{2, 3, VPAD_RIGHT},
	{3, 2, VPAD_DOWN},
};

static POINT points_buffer[3];

bool keyseq_config_validate(const int& nextf, const int& show_mode, const int& show_frame_mode){
	if (nextf < 0 || nextf > 256)
		return false;
	if (show_frame_mode < 0 || show_frame_mode > SHOW_FRAME_MODE_FULL)
		return false;
	if (show_mode < 0 || show_mode > SHOW_MODE_PAD)
		return false;
	return true;
}

static bool resize_request = true;

inline static const string& get_data(const int& pos) {
	if (pos < 0)
		return mgrp->get_mapping().empty_show;
	if (pos >= mgrp->get_all_inputs().size())
		return mgrp->get_mapping().empty_show;
	return mgrp->get_all_inputs()[pos];
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
			SetGraphicsMode(hdc, GM_ADVANCED);
			const HFONT hf = CreateFontW(FONT_SIZE, 0, 0, 0, FW_BOLD, 0, 0, 0,
								   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
								   FIXED_PITCH | FF_SWISS, NULL);

			const auto fnttmp = SelectObject(hdc, hf);

			int nowy = Y_START;
			const auto colback = GetTextColor(hdc);
			const auto bkback = GetBkColor(hdc);

			SetBkColor(hdc, 0);
			const bool pad_mode = cfg_show_mode == SHOW_MODE_PAD;
			// inputs
			{
				const int row = cfg_show_next + (int)(!pad_mode);
				for (int i = 0; i < row; i++)
				{
					const int tar = mgrp->get_last_runned_input_index() + (row - (pad_mode?0:1) - i);
					const size_t all_input_sz = mgrp->get_all_inputs().size();
					if (all_input_sz != 0 && (tar == mgrp->get_tstart_index() || tar + 1 == all_input_sz))
					{
						SetBkColor(hdc, COL_START_END_BACK);
					}
					else if (i + 1 == row && !pad_mode)
					{
						SetBkColor(hdc, COL_NOW_BACK);
					}
					else
					{
						SetBkColor(hdc, 0);
					}

					const auto& s = get_data(tar);
					const char* ap = mgrp->get_mapping().all_show.c_str();
					for (int j = 0, nowx = X_START; *ap; j++, nowx += X_STEP, ap++)
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
				nowy += max(0, FONT_SIZE - Y_STEP);
			}


			//if (is_waiting_event)
			//{
			//	SetTextColor(hdc, COL_KEY);
			//	TextOutA(hdc, X_START, nowy - Y_STEP, "[WAITING...]", lstrlenA("[WAITING...]"));
			//}
		
			if(pad_mode) {
				SetBkColor(hdc, 0);
				nowy++;
				if (cfg_show_next > 0)
				{ // split line
					const auto hPen = CreatePen(PS_SOLID, PAD_LINE_HEIGHT, COL_PAD_LINE);
					const auto hOldPen = SelectObject(hdc, hPen);
					MoveToEx(hdc, X_START, nowy, NULL);
					LineTo(hdc, X_START + X_STEP * mgrp->get_mapping().empty_show.size(), nowy);
					SelectObject(hdc, hOldPen);
					DeleteObject(hPen);
					nowy += PAD_LINE_HEIGHT;
				}
				const auto last_input = mgrp->get_last_input().input_status;

				for (const auto& [row, idx, v, c] : PAD_DATA)
				{
					const int y = nowy +Y_STEP * row;
					SetTextColor(hdc, ((last_input & v) == 0) ? COL_PAD_OFF : COL_PAD_ON);
					TextOutA(hdc, X_START + X_STEP * idx, y, c, lstrlenA(c));
				}
				// DPAD Triangles
				for (const auto& [row, idx, v] : DPAD_DATA)
				{
					const int top_y = nowy + Y_STEP * row;
					const int bottom_y = top_y + Y_STEP;
					const int left_x = X_START + X_STEP * idx; 
					const int right_x = left_x + X_STEP;
					const auto br_back = SelectObject(hdc, CreateSolidBrush(((last_input & v) == 0) ? COL_PAD_OFF : COL_PAD_ON));
					if ((v & VPAD_DOWN) | (v & VPAD_UP))
					{
						const int mid_x = (left_x + right_x) >> 1;
						const bool IS_UP = v & VPAD_UP;
						points_buffer[0] = {mid_x - (DPAD_BOX_SIZE >> 1), IS_UP ? bottom_y : top_y};
						points_buffer[1] = {mid_x + (DPAD_BOX_SIZE >> 1), IS_UP ? bottom_y : top_y};
						points_buffer[2] = {mid_x, IS_UP? bottom_y - DPAD_BOX_SIZE : top_y + DPAD_BOX_SIZE};
					}
					else // left or right
					{
						const int mid_y = (top_y + bottom_y) >> 1;
						const bool IS_RIGHT = v & VPAD_RIGHT;
						points_buffer[0] = {IS_RIGHT ? left_x : right_x, mid_y - (DPAD_BOX_SIZE >> 1)};
						points_buffer[1] = {IS_RIGHT ? left_x : right_x, mid_y + (DPAD_BOX_SIZE >> 1)};
						points_buffer[2] = {IS_RIGHT ? left_x + DPAD_BOX_SIZE : right_x - DPAD_BOX_SIZE, mid_y};
					}
					Polygon(hdc, points_buffer, 3);
					DeleteObject(SelectObject(hdc, br_back));
				}

				nowy += Y_STEP * 3 + FONT_SIZE;
			}

			if (cfg_show_frames != SHOW_FRAME_MODE_NONE) 
			{ // frame
				const bool is_finished = !mgrp->is_running();
				int tar_frame;

				if (cfg_show_frames == SHOW_FRAME_MODE_FULL || is_finished)
					tar_frame = mgrp->get_now_frame_count();
				else if (mgrp->get_requested_show_frames() >= 0) 
					tar_frame = mgrp->get_requested_show_frames();
				else tar_frame = -1;
				nowy++;

				if (tar_frame >= 0)
				{
					SetBkColor(hdc, 0);
					const auto st = to_string(tar_frame) + "f   ";
					const auto cst = st.c_str();
					if (is_finished)
					{
						SetTextColor(hdc, COL_FIN_FRAME);
						TextOutA(hdc, X_START, nowy, cst, lstrlenA(cst));
					}
					else
					{
						SetTextColor(hdc, COL_FRAME);
						TextOutA(hdc, X_START, nowy, cst, lstrlenA(cst));
					}
				}
				nowy += FONT_SIZE;
			}

			if (resize_request)
			{
				window_height = CalcWindowSize(hwnd, WINDOW_W, nowy + 1).second + Y_END_PADDING;
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, WINDOW_W, window_height, SWP_SHOWWINDOW | SWP_NOMOVE);

				resize_request = false;
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
	hwnd = UtilCreateWindow(TEXT("Input viewer"), TEXT("Input viewer"), WndProc, 0, GUI_NO_CLOSE_WINDOW);
	if (!hwnd)
		return;
	ShowWindow(hwnd, SW_SHOW);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 110, WINDOW_W, window_height, SWP_SHOWWINDOW);
	FixWindowCornor(hwnd);
	UpdateWindow(hwnd);
	MessageLoop(hwnd);

}

void keyseq_refresh(const bool& hard)
{
	InvalidateRect(hwnd, NULL, hard);
}


void keyseq_init(MacroManager* _mgrp)
{
	mgrp = _mgrp;
	std::thread _t(window_main);
	_t.detach();
}

void keyseq_set_window_pos(const int& x, const int& y) {
	SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
}

void keyseq_change_config(const int& nextf, const int& show_mode, const int& show_frame_mode){
	cfg_show_next = nextf;
	cfg_show_mode = show_mode;
	cfg_show_frames = show_frame_mode;
	resize_request = true;
	keyseq_refresh(true);
}
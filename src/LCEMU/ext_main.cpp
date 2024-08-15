#include "ext_main.h"
#include "vpad_ids.h"
#include "utils.h"
#include "macro_manager.h"
#include "gui/key_sequence.h"
#include "../gui/guiWrapper.h"

using namespace std;
namespace fs = std::filesystem;

#define TOOL_NAME "LCEMU"
#define TOOL_VER "0.0.13-rev2"
#define TOOL_VER_EX ""

constexpr int CS_W = 300;
constexpr int CS_H = 480;

std::mutex cout_mtx;
std::mutex running_mtx;
extern WindowInfo g_window_info;

inline static void set_mapping(MappingData& data,const char c, uint32 m, const char s, const int idx)
{
	data.to_key[c] = m;
	data.to_show[c] = s;
	data.to_show_index[c] = idx;
	data.idxtoshow[idx] = s;
}
static void setup_mapping(MappingData& data)
{
	int idx = 1;
	// J = B
	set_mapping(data, 'B', VPAD_B, 'B', idx);
	set_mapping(data, 'J', VPAD_B, 'B', idx++);

	set_mapping(data, 'A', VPAD_A, 'A', idx++);
	set_mapping(data, 'Y', VPAD_Y, 'Y', idx++);
	set_mapping(data, 'X', VPAD_X, 'X', idx++);


	set_mapping(data, 'S', VPAD_L, 'L', idx);
	set_mapping(data, 'L', VPAD_L, 'L', idx++);

	set_mapping(data, 'R', VPAD_R, 'R', idx++);

	set_mapping(data, 'V', VPAD_DOWN, 'v', idx);
	set_mapping(data, 'D', VPAD_DOWN, 'v', idx++);

	set_mapping(data, '~', VPAD_UP, '^', idx);
	set_mapping(data, 'U', VPAD_UP, '^', idx++);

	set_mapping(data, '>', VPAD_RIGHT, '>', idx++);
	set_mapping(data, '<', VPAD_LEFT, '<', idx++);


	set_mapping(data, 'P', VPAD_PLUS, '+', idx++);
	set_mapping(data, 'M', VPAD_MINUS, '-', idx++);

	set_mapping(data, 'N', VPAD_NULL, 0, 0); // neutral

	data.none_show.resize(idx - 1, ' '); 
	data.all_show.resize(idx - 1, ' ');
	for (int i = 1; i < idx; i++)
		data.all_show[i - 1] = data.idxtoshow[i];
}

static void output(const string& s, bool new_line = true)
{
	std::lock_guard<std::mutex> _l(cout_mtx);
	cout << s;
	if (new_line) cout << '\n';
}


MacroManager macro_mgr;

static void ctrl_loop()
{
	fs::path macro_dir = fs::current_path() / "macro";
	string prepath_str = "./macro.txt";
	while (1)
	{
		string instr;
		output("> ", false);

		getline(cin, instr);
		{
			std::lock_guard<std::mutex> _l(running_mtx);
			boost::trim(instr);
			vector<string> strs = split_space(instr);
			string cmd = boost::to_upper_copy( strs[0]);
			
			if (cmd == "PWD")
			{
				output(macro_dir.string());
			}
			else if (cmd == "CLS")
			{
				clear_screen();
			}
			else if (cmd == "SEQPOS")
			{
				int py, px;
				if (strs.size() < 2 || !try_int(strs[1], px, 0) || !try_int(strs[2], py, 0))
				{
					output("[ERROR] Arg error.");
					continue;
				}
				keyseq_set_window_pos(px, py);
			}

			#pragma region Macro
			else if (cmd == "PRE")
			{
				output(prepath_str);
			}
			else if (cmd == "START" || cmd == "S")
			{
				macro_mgr.start();
				keyseq_reset();
				output("Started");
			}
			else if (cmd == "STOP" || cmd == "X")
			{
				macro_mgr.stop();
				output("Stopped");
			}
			else if (cmd == "LOAD" || cmd == "L" || cmd == "RELOAD" || cmd == "R" || cmd == "RS")
			{
				bool is_rs = cmd == "RS";
				if (is_rs)
					macro_mgr.stop();
				if (macro_mgr.is_running())
				{
					output("[ERROR] Stop the running macro.");
					output("Load failed");
					continue;
				}
				else
				{
					bool is_load = cmd == "LOAD" || cmd == "L";
					if (is_load && strs.size() < 2)
					{
						output("[ERROR] Please specify a file.");
						continue;
					}
					string path_str = (is_load)? strs[strs.size() - 1] : prepath_str;
					if (is_load) prepath_str = path_str;
					fs::path path;
					if (!try_eval_relative(path_str, macro_dir, path))
					{
						output("[ERROR] File not found: " + prepath_str);
						output("Load failed");
						continue;
					}
					bool loadret;
					{
						std::lock_guard<std::mutex> _l(cout_mtx);
						loadret = macro_mgr.load(path);
					}
					if (!loadret)
					{
						output("Load failed");
						continue;
					}
					
					output("Loaded: " + path_str);
					output(format("{} frames", macro_mgr.all_inputs.size()));
					keyseq_set(macro_mgr.all_inputs, macro_mgr.tstart_index);
					if (is_rs)
					{
						macro_mgr.start();
						keyseq_reset();
						output("Started");
					}
				}
			}
			else if (cmd == "DUMPMACRO" || cmd == "DUMPMACRO2")
			{
				if (macro_mgr.all_inputs.empty())
				{
					output("[ERROR] Macro is empty.");
					continue;
				}
				int offset = 0;
				if (strs.size() >= 2)
				{
					if (!try_int(strs[1], offset, 0))
					{
						output("[ERROR] Parse error.");
						continue;
					}
					else if (offset >= macro_mgr.all_inputs.size() || offset < 0)
					{
						output("[ERROR] Out of range.");
						continue;
					}
				}
				auto npath = macro_mgr.get_now_path().string() + ".dump.txt";
				FILE* fp;
				if (fopen_s(&fp, npath.c_str(), "w"))
					output("[ERROR] Could not create a file.");
				else
				{
					bool rle_style = cmd == "DUMPMACRO2";
					if (rle_style)
					{
						fprintf(fp, "input,length\n");
					}
					else
					{
						fprintf(fp, "start(f),end(f),input\n");
					}
					string pre = macro_mgr.all_inputs[offset];
					int st = offset;
					for (int i = offset + 1; i <= macro_mgr.all_inputs.size(); i++)
					{
						if (macro_mgr.all_inputs.size() == i || pre != macro_mgr.all_inputs[i])
						{
							const int l = st + 1 - offset;
							const int r = i - offset;
							if (rle_style)
							{
								fprintf(fp, "%s,%d\n", pre.c_str(),r - l + 1);
							}
							else
							{
								fprintf(fp, "%d,%d,%s\n", l, r, pre.c_str());
							}
							st = i;
							if (macro_mgr.all_inputs.size() > i)
								pre = macro_mgr.all_inputs[i];
						}
					}

					fclose(fp);
					output("Success.");
				}

			}
			#pragma endregion

			else
			{
				output("Unknown command");
			}
		} // release lock
	}
}

void ext_init()
{
	setup_mapping(macro_mgr.mapping);
	keyseq_init(macro_mgr.mapping.all_show); // after mapping
#ifndef CEMU_DEBUG_ASSERT
	FreeConsole();
	AllocConsole();
	FILE* fp;
	HWND csw = GetConsoleWindow();
	SetWindowPos(csw, HWND_TOP, 288, 0, CS_W, CS_H, SWP_SHOWWINDOW);
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
#endif
	auto title = format("{} : {}{}", TOOL_NAME, TOOL_VER,TOOL_VER_EX);
	SetConsoleTitleA(title.c_str());
	cout << title << endl;
	std::thread thr(ctrl_loop);
	thr.detach();
}

bool ext_onframe(uint32be& status)
{
	std::lock_guard<std::mutex> _l(running_mtx);
	// Frame Advance
	// Todo: Config
	if (g_window_info.get_keystate(VK_LSHIFT))
		Sleep(480);


	if (macro_mgr.is_running())
	{
		bool ret = macro_mgr.on_frame(status);
		keyseq_on_frame(macro_mgr);
		return ret;
	}
	else
	{
		return false;
	}
}
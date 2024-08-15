#pragma once
#include <windows.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>
#include "vpad_ids.h"
#include "utils.h"

using namespace std;
namespace fs = std::filesystem;
#define MAX_CALL_DEPTH 8192

// TODO improve
enum WaitEvents : int
{
	WaitEvent_NONE = 0,
	WaitEvent_SMM147_LEVEL_LOADED = 1,
};

enum MacroCodeType
{
	MacroCode_None,
	MacroCode_Input,
	MacroCode_SLP,
	MacroCode_QUIT,
	MacroCode_TSTART,
	MacroCode_CALL,
	MacroCode_EV_WAIT,
};

struct MappingData
{
	std::unordered_map<char, uint32> to_key;
	std::unordered_map<char, char> to_show;
	std::unordered_map<char, int> to_show_index;
	char idxtoshow[16];
	std::string none_show;
	std::string all_show;
};

struct MacroCode
{
	MacroCodeType type = MacroCode_None;
	int repeat = 1;
	bool eof = false;
	int line = 0;
	std::vector<std::string> strargs;
	std::vector<int> intargs;
	uint32be input_status = 0;
	MacroCode(const MacroCodeType type, const int line, const int rep = 1)
		: type(type), line(line),repeat(rep) {}
	MacroCode(const MacroCodeType type, const int line, const std::string& first_str, const int rep = 1)
		: type(type), line(line), repeat(rep)
	{
		strargs.emplace_back(first_str);
	}
	MacroCode() {}
};

struct RunInfo
{
	RunInfo* parent = nullptr;
	std::vector<MacroCode>* target;
	int now_step = 0;
	int now_pos = 0;
	int depth = 0;
};


#define ERROR_RETURN(mess) return macro_error(mess, lineno), unload(), false
struct MacroManager
{
  private:
	bool m_running = false;
	map<string, vector<MacroCode>> m_sub_macro;
	vector<MacroCode> m_main_macro;
	RunInfo* m_now_run = nullptr;
	bool m_has_tstart = false;
	int m_total_frames = 0;
	int m_last_run_index = -1;
	fs::path m_now_path;
	fs::path m_now_parent_path;
	WaitEvents m_waitevent = WaitEvent_NONE;
	const void macro_error(const string& mes, const int line) const
	{
		put_with_color(format("[ERROR] {}: L{}", mes, line), FOREGROUND_RED);
	}
	const void macro_warning(const string& mes, const int line) const
	{
		put_with_color(format("[WARNING] {}: L{}", mes, line), FOREGROUND_GREEN);
	}
	void free_runinfo(RunInfo* node)
	{
		if (node == nullptr)
			return;
		RunInfo* par = node->parent;
		delete node;
		free_runinfo(par);
	}

  public:
	MappingData mapping;
	vector<string> all_inputs;
	int tstart_index;
	void unload()
	{
		stop();
		m_sub_macro.clear();
		m_main_macro.clear();
		m_has_tstart = false;
		all_inputs.clear();
	}
	// todo: rework -> macro_compiler.cpp
	bool load(const fs::path& path)
	{
		if (!fs::exists(path))
		{
			put_with_color("[ERROR] File not found : " + path.string(), FOREGROUND_RED);
			return false;
		}
		unload();
		std::ifstream infile(path);
		string line;
		int lineno = 0;
		m_now_path = fs::absolute(path);
		m_now_parent_path = m_now_path.parent_path();
		vector<MacroCode>* now_vec = &m_main_macro;
		bool is_in_main_macro = true;
		vector<pair<string, int>> called_sub;

		while (std::getline(infile, line))
		{
			lineno++;
			auto slash_pos = line.find("//");
			if (slash_pos != string::npos)
			{
				line = line.substr(0, slash_pos);
			}
			boost::trim(line);
			if (line.empty())
				continue;
			vector<string> dats;
			boost::split(dats, line, boost::is_any_of("*")); // repeater
			int rep = 1;
			if (dats.size() >= 2)
			{
				if (dats.size() > 2 || !try_int(dats[1], rep, 1))
				{
					ERROR_RETURN("Parse error");
				}
			}

			boost::trim(dats[0]);
			if (dats[0].empty())
			{
				macro_warning("Ignored empty code", lineno);
				continue;
			}

			auto def_dat = dats[0];
			boost::to_upper(dats[0]);

			if (dats[0].starts_with('!')) // command
			{
				vector<string> subcmd = split_space(dats[0]);
				if (rep > 1 && (subcmd[0] != "!CALL"))
				{
					macro_warning("Can't repeat", lineno);
				}

				if (subcmd[0] == "!SLP")
				{
					int wait = 500;
					if (subcmd.size() >= 2)
					{
						if (subcmd.size() > 2 || !try_int(subcmd[1], wait, 500))
							ERROR_RETURN("Parse error");
					}
					MacroCode d(MacroCode_SLP, lineno);
					d.intargs.emplace_back(wait);
					now_vec->push_back(d);
				}
				else if (subcmd[0] == "!SLPL")
				{
					MacroCode d(MacroCode_SLP, lineno);
					d.intargs.emplace_back(1000);
					now_vec->push_back(d);
				}
				else if (subcmd[0] == "!CALL")
				{
					if (subcmd.size() != 2 && subcmd.size() != 3)
					{
						ERROR_RETURN("Arg error");
					}
					int lim = MAX_CALL_DEPTH + 4;
					if (subcmd.size() == 3)
					{
						if (!try_int(subcmd[2], lim, MAX_CALL_DEPTH + 4))
						{
							ERROR_RETURN("Parse error");
						}
					}
					MacroCode d(MacroCode_CALL, lineno, subcmd[1], rep);
					d.intargs.push_back(lim);
					called_sub.emplace_back(subcmd[1], lineno); // check after
					now_vec->push_back(d);
				}
				else if (subcmd[0] == "!TSTART")
				{
					m_has_tstart = true;
					now_vec->emplace_back(MacroCode_TSTART, lineno, 1);
				}
				else if (subcmd[0] == "!QUIT")
				{
					now_vec->emplace_back(MacroCode_QUIT, lineno, 1);
				}

				//else if (subcmd[0] == "!!WAITEV") // !!: consume a frame
				//{
				//	if (subcmd.size() != 2 || subcmd[1] != "SMM147_LEVEL_LOAD") // todo improve
				//	{
				//		ERROR_RETURN("Parse error");
				//	}
				//	MacroCode d(MacroCode_EV_WAIT, lineno);
				//	d.intargs.emplace_back(WaitEvent_SMM147_LEVEL_LOADED);
				//	d.input_status = VPAD_NULL;
				//	d.strargs.emplace_back(mapping.none_show);
				//	now_vec->push_back(d);
				//}
				else
				{
					ERROR_RETURN(format("Unknown command: \"{}\"", subcmd[0]));
				}
			}
			else if (dats[0].starts_with('#'))
			{
				vector<string> subcmd = split_space(dats[0]);
				// preprocessor
				if (subcmd[0] == "#SUB")
				{
					if (subcmd.size() != 2)
					{
						ERROR_RETURN("Arg error");
					}
					if (!is_in_main_macro)
					{
						ERROR_RETURN("Subroutines cannot be nested");
					}
					string name = subcmd[1];
					if (m_sub_macro.contains(name))
					{
						ERROR_RETURN(format("\"{}\" is already defined", name));
					}
					is_in_main_macro = false;
					m_sub_macro[name] = vector<MacroCode>();
					now_vec = &m_sub_macro[name];
				}
				else if (subcmd[0] == "#ENDSUB" || subcmd[0] == "#ESUB")
				{
					if (is_in_main_macro)
					{
						ERROR_RETURN(format("Unexcepted {}", subcmd[0]));
					}
					is_in_main_macro = true;
					now_vec = &m_main_macro;
				}
				// todo #EXPAND
				else
				{
					ERROR_RETURN(format("Unknown preprocessor \"{}\"", subcmd[0]));
				}
			}
			else // macro
			{
				bool check = true;
				string optim;
				//for (auto& e : def_dat)
				//	if (isalpha(e) && islower(e))
				//		macro_warning("Lower case letter", lineno);
				for (auto& c : dats[0])
				{
					if (isspace(c))
						continue;
					check &= mapping.to_key.contains(c);
					optim.push_back(c);
				}
				if (!check)
					macro_warning("Contains unknown key", lineno);

				string show = mapping.none_show;
				uint32be status = 0;
				for (auto& c : optim)
				{
					if (!mapping.to_key.contains(c) || mapping.to_show_index[c] <= 0)
						continue;
					status |= mapping.to_key[c];
					show[mapping.to_show_index[c] - 1] = mapping.to_show[c];
				}
				MacroCode d(MacroCode_Input, lineno, show, rep);
				d.input_status = status;
				now_vec->push_back(d);
			}
		}
		lineno++;
		if (!is_in_main_macro)
		{
			ERROR_RETURN("Excepted: ENDSUB");
		}
		for (auto& [name, lin] : called_sub)
		{
			if (!m_sub_macro.contains(name))
			{
				macro_error(format("Unknown name: \"{}\"", name), lin);
				return unload(), false;
			}
		}

		generate_all_inputs();
		return true;
	}
	void init_runinfo()
	{
		free_runinfo(m_now_run);
		m_now_run = new RunInfo();
		m_now_run->target = &m_main_macro;
	}

	void start()
	{
		stop();
		init_runinfo();
		m_waitevent = WaitEvent_NONE;
		m_last_run_index = -1;
		m_total_frames = m_has_tstart ? -1 : 0;
		m_running = true;
	}

	void stop()
	{
		free_runinfo(m_now_run);
		m_running = false;
		m_now_run = nullptr;
	}

	const bool is_running() const
	{
		return m_running;
	}
	const bool is_waiting_event() const
	{
		return m_waitevent != WaitEvent_NONE;
	}
	const int get_now_frame_count() const
	{
		return max(0, m_total_frames);
	}
	const int get_last_input_index() const
	{
		return m_last_run_index;
	}
	const bool is_timer_started() const
	{
		return m_total_frames >= 0;
	}
	const fs::path get_now_path() const
	{
		return m_now_path;
	}

	bool on_event(WaitEvents ev) {
		if (m_waitevent == ev)
		{
			m_waitevent = WaitEvent_NONE;
			return true;
		}
		return false;
	}

	bool read_next(MacroCode& next)
	{
		auto data = m_now_run->target;
		if (m_now_run->now_pos >= data->size()) // end of vector
		{
			if (m_now_run->parent != nullptr)
			{
				auto node = m_now_run;
				m_now_run = node->parent;
				delete node;
				return read_next(next);
			}
			else
			{
				delete m_now_run;
				m_now_run = nullptr;
				next = MacroCode();
				next.eof = true;
				return false;
			}
		}
		else if (m_now_run->now_step < data->at(m_now_run->now_pos).repeat)
		{
			m_now_run->now_step++;
			next = data->at(m_now_run->now_pos);
			return true;
		}
		else // step == rep
		{
			m_now_run->now_step = 0;
			m_now_run->now_pos++;
			return read_next(next);
		}
	}

	bool get_next_input(MacroCode& next, bool runmode)
	{
		while (read_next(next) && next.type != MacroCode_Input) // all command
		{
			if (runmode)
			{
				if (next.type == MacroCode_SLP)
				{
					Sleep(next.intargs.at(0));
				}
				else if (next.type == MacroCode_TSTART)
				{
					m_total_frames = 0;
				}

			}
			else
			{
				if (next.type == MacroCode_TSTART)
				{
					tstart_index = all_inputs.size();
				}
			}

			if (next.type == MacroCode_EV_WAIT)
			{
				if (runmode)
					m_waitevent = (WaitEvents)next.intargs[0];
				break; // consume a frame
			}
			else if (next.type == MacroCode_CALL)
			{
				if (m_now_run->depth >= next.intargs.at(0))
				{
					// skip
					continue;
				}
				else if (m_now_run->depth >= MAX_CALL_DEPTH)
				{
					if (!runmode)
					{
						// todo: improve
						macro_error(format("Maximum recursion depth({}) exceeded.", MAX_CALL_DEPTH), next.line);
					}
					next.eof = true;
					break;
				}
				RunInfo* inf = new RunInfo();
				inf->parent = m_now_run;
				inf->target = &m_sub_macro[next.strargs.at(0)];
				inf->depth = m_now_run->depth + 1;
				m_now_run = inf;
			}
			else if (next.type == MacroCode_QUIT)
			{
				next.eof = true;
				break;
			}
		}
		return !next.eof; // next == input or eof
	}

	// todo: improve
	void generate_all_inputs()
	{
		stop();
		init_runinfo();
		MacroCode next;
		all_inputs.resize(0);
		tstart_index = 0;
		while (get_next_input(next, false))
			all_inputs.emplace_back(next.strargs.at(0));
	}

	bool on_frame(uint32be& status)
	{
		if (is_waiting_event())
			return m_running; // no change
		MacroCode next;
		if (!get_next_input(next, true))
		{
			stop();
		}
		else
		{
			if (is_timer_started())
				m_total_frames++;

			status = next.input_status;
		}
		m_last_run_index++;
		return m_running;
	}
};

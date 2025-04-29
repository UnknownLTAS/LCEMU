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
#include "macro_structs.h"
#include "macro_compiler.h"

using namespace std;
namespace fs = std::filesystem;

struct MacroManager
{
  private:
	bool m_running = false;
	RunInfo* m_now_run = nullptr;
	Routines m_now_routines;
	bool m_has_tstart = false;
	bool m_withmode = false;
	int m_current_total_frames = 0;
	int m_last_runned_index = -1;
	int m_tstart_index;
	int m_requested_show_frames = -1;
	MacroCode m_last_code;
	fs::path m_now_path;
	WaitEvents m_waitevent = WaitEvent_NONE;
	vector<string> m_all_inputs;
	MappingData m_mapping;
	
	CompiledMacro m_compiledMacro;

	void free_runinfo(RunInfo* node)
	{
		if (node == nullptr)
			return;
		RunInfo* par = node->parent;
		delete node;
		free_runinfo(par);
	}
	void init_runinfo()
	{
		free_runinfo(m_now_run);
		m_now_run = new RunInfo();
		m_now_run->target = &m_compiledMacro;
		m_now_routines.clear();
		m_now_routines.push(&m_compiledMacro.sub_macro_table);
	}

	void init_params()
	{
		m_last_code = MacroCode(MacroCode_None);
		m_waitevent = WaitEvent_NONE;
		m_last_runned_index = -1;
		m_current_total_frames = m_has_tstart ? -1 : 0;
		m_requested_show_frames = -1;
	}

  public:
	void unload()
	{
		stop();
		init_runinfo();
		init_params();
		m_compiledMacro.clear();
		m_has_tstart = false;
		m_all_inputs.clear();
	}

	bool load(const fs::path& abs_path);

	void start(const bool with)
	{
		stop();
		init_runinfo();
		init_params();
		m_withmode = with;
		m_running = true;
	}

	void stop()
	{
		free_runinfo(m_now_run);
		m_running = false;
		m_now_run = nullptr;
		m_now_routines.clear();
		// do not init params(keep info)
	}
#pragma region Getter

	[[nodicard]] bool is_running() const
	{
		return m_running;
	}
	[[nodicard]] bool is_waiting_event() const
	{
		return m_waitevent != WaitEvent_NONE;
	}
	[[nodicard]] bool is_timer_started() const
	{
		return m_current_total_frames >= 0;
	}
	[[nodicard]] int get_now_frame_count() const
	{
		return max(0, m_current_total_frames);
	}
	[[nodicard]] int get_last_runned_input_index() const
	{
		return m_last_runned_index;
	}
	[[nodicard]] int get_tstart_index() const
	{
		return m_tstart_index;
	}
	[[nodicard]] int get_requested_show_frames() const
	{
		return m_requested_show_frames;
	}
	[[nodicard]] const MacroCode& get_last_input() const
	{
		return m_last_code;
	}
	[[nodicard]] MappingData& get_mapping()
	{
		return m_mapping;
	}
	[[nodicard]] fs::path get_now_path() const
	{
		return m_now_path;
	}
	[[nodicard]] const vector<string>& get_all_inputs() const
	{
		return m_all_inputs;
	}
	[[nodicard]] size_t get_total_frame_count() const
	{
		return m_all_inputs.size();
	}

#pragma endregion

  private:
	bool read_next(MacroCode& next);

	bool get_next_input(MacroCode& next, const bool& runmode);


  public:


	bool on_event(const WaitEvents ev)
	{
		if (m_waitevent == ev)
		{
			m_waitevent = WaitEvent_NONE;
			return true;
		}
		return false;
	}

	bool on_frame(uint32be& status)
	{
		if (is_waiting_event())
			return m_running; // no change
		MacroCode next;
		if (!get_next_input(next, true))
		{
			stop();
			m_last_code = MacroCode(MacroCode_None);
		}
		else
		{
			if (is_timer_started())
				m_current_total_frames++;

			if (m_withmode)
				status |= next.input_status;
			else
				status = next.input_status;
			swap(m_last_code, next);
		}
		m_last_runned_index++;
		return m_running;
	}
};


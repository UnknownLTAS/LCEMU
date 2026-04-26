#include "macro_manager.h"
#include "gui/key_sequence.h"
bool MacroManager::read_next(MacroCode& next)
{
	const auto& data = m_now_run->target->macro;
	if (m_now_run->now_pos >= data.size()) // end of vector
	{
		if (m_now_run->parent != nullptr)
		{
			// end of sub routine
			auto node = m_now_run;
			m_now_run = node->parent;
			delete node;
			m_now_routines.pop();
			return read_next(next);
		}
		else // end of macro
		{
			delete m_now_run;
			m_now_run = nullptr;
			next = MacroCode();
			next.eof = true;
			return false;
		}
	}
	else if (m_now_run->now_step < data.at(m_now_run->now_pos).repeat)
	{
		// repeating
		m_now_run->now_step++;
		next = data.at(m_now_run->now_pos);
		return true;
	}
	else // step == rep
	{
		m_now_run->now_step = 0;
		m_now_run->now_pos++;
		return read_next(next);
	}
}

bool MacroManager::get_next_input(MacroCode& next, const bool& runmode)
{
	while (read_next(next) && next.type != MacroCode_Input) // all command
	{
		if (runmode)
		{
			switch (next.type)
			{
			case MacroCode_SLP:
				Sleep(next.intargs.at(0));
				break;
			case MacroCode_TSTART:
				m_current_total_frames = 0;
				break;
			case MacroCode_SHOWFRAMES:
				m_requested_show_frames = m_current_total_frames;
				break;
			case MacroCode_SEQCFG:
				keyseq_change_config(next.intargs.at(0), next.intargs.at(1), next.intargs.at(2));
				break;
			}
		}
		else
		{
			switch (next.type)
			{
			case MacroCode_TSTART:
				m_has_tstart = true;
				// current m_all_inputs size = index
				m_tstart_index = m_all_inputs.size();
				break;
			}
		}

		// common
		if (next.type == MacroCode_EV_WAIT)
		{
			if (runmode)
				m_waitevent = (WaitEvents)next.intargs[0];
			break; // consume a frame
		}
		else if (next.type == MacroCode_CALL)
		{
			if (m_now_run->depth >= next.intargs.at(1))
			{
				// skip
				continue;
			}
			RunInfo* inf = new RunInfo();
			inf->parent = m_now_run;
			inf->target = m_now_routines.get(next.intargs.at(0));
			inf->depth = m_now_run->depth + 1;
			m_now_routines.push(&inf->target->sub_macro_table);
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


static inline bool printMessages(const vector<MacroMessage>& messages) {
	bool err = false;
	for (const auto& msg : messages)
	{
		string type;
		int color;
		if (msg.type == MSG_ERROR)
		{
			type = "ERROR";
			color = FOREGROUND_RED;
			err = true;
		}
		else if (msg.type == MSG_WARNING)
		{
			type = "WARNING";
			color = FOREGROUND_GREEN;
		}
		else
		{
			type = "INFO";
			color = FOREGROUND_BLUE | FOREGROUND_RED; 
		}
		put_with_color(format("[{}] {}: L{} ({})", type, msg.message, msg.line, msg.file.string()), color);
	}
	return err;
}

bool MacroManager::load(const fs::path& path, const bool& ignoreCountUp)
{
	fs::path abs_path;
	if (!try_eval_relative(path, m_working_dir, abs_path) || !fs::exists(abs_path))
	{
		put_with_color("[ERROR] File not found : " + path.string(), FOREGROUND_RED);
		return false;
	}
	unload();
	m_now_path = abs_path;
	CompileResult res;
	const bool ret = compile(m_now_path, m_mapping, res, {ignoreCountUp});
	printMessages(res.messages);
	if (!ret)
		return unload(), false;
	m_compiledMacro = res.macro;

	init_runinfo();
	// Generate all inputs
	MacroCode next;
	m_all_inputs.clear();
	m_tstart_index = 0;
	while (get_next_input(next, false))
		m_all_inputs.emplace_back(next.strargs.at(0));

	return true;
}


// 0-indexed, [l, r)
const vector<tuple<string, size_t, size_t>> MacroManager::generate_compressed_inputs(const size_t& offset) const
{
	if (offset >= m_all_inputs.size())
		return {};
	string pre = m_all_inputs[offset];
	size_t st = offset;
	vector<tuple<string, size_t, size_t>> ret; 
	for (size_t i = offset + 1; i <= m_all_inputs.size(); i++)
	{
		if (m_all_inputs.size() == i || pre != m_all_inputs[i])
		{
			ret.emplace_back(pre, st - offset, i - offset);
			st = i;
			if (i < m_all_inputs.size())
				pre = m_all_inputs[i];
		}
	}
	return ret;
}

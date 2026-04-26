#include "./macro_compiler.h"
#include "../utils.h"
#include "../gui/key_sequence.h"

constexpr int SLP_DEFAULT_MS = 500;
constexpr int SLPL_MS = 1000;

inline static bool isLabel(const string& str)
{
	return !str.empty() && str[0] == '@';
}

inline static bool getLabelName(const string& label, string& ret)
{
	if (!isLabel(label))
		return false;

	ret = label.substr(1, label.size() - 1);
	boost::trim(ret);
	boost::to_upper(ret);
	return true;
}

inline static void deleteComment(string& str)
{
	const auto slash_pos = str.find("//");
	if (slash_pos != string::npos)
		str.erase(str.begin() + slash_pos, str.end());
}

struct Compiler
{
	CompileOption m_option;
	Compiler(const MappingData* mapping, const CompileOption& option)
		: m_mapping(mapping), m_option(option)
	{
	}

	void init() {
		m_messages.clear();
		m_file_table.clear();
		m_routine_table.clear();
		m_parent_file_ids.clear();
	}

	bool run(const filesystem::path& file, CompiledMacro& result) {
		const auto f = filesystem::absolute(file);
		m_parent_file_ids.emplace(m_file_table.getId(f.string()));
		return compile(f, result);
	}

	bool check(CompiledMacro& result) {
		Routines routines;
		routines.push(&result.sub_macro_table);
		return checkCalls(result, routines, 0);
	}


	[[nodiscard]] vector<MacroMessage>& getMessages() {
		return m_messages;
	}

  private:
	const MappingData* m_mapping;
	vector<MacroMessage> m_messages;
	IdTable<string, int> m_file_table;
	IdTable<string, int> m_routine_table;
	set<int> m_parent_file_ids;

#define ERROR_RETURN(msg) return m_messages.emplace_back(MSG_ERROR, msg, file_path, lineno), false;
#define WARNING(msg) (m_messages.emplace_back(MSG_WARNING, msg, file_path, lineno));
#define INFO(msg) (m_messages.emplace_back(MSG_INFO, msg, file_path, lineno));
#define COMPILE_INFO (CompileInfo{file_id, lineno})

	bool compile(
		istream& macro_stream,
		CompiledMacro& result,
		const std::filesystem::path& file_path,
		int& lineno,
		const bool& require_end,
		Label* end_label = nullptr
	)
	{
		const auto file_id = m_file_table.getId(file_path.string());
		string line;
		while (std::getline(macro_stream, line))
		{
			lineno++;
			deleteComment(line);
			boost::trim(line);
			if (line.empty())
				continue;
			string label_name;
			if (getLabelName(line, label_name))
			{
				if (end_label != nullptr && label_name == end_label->name)
				{
					end_label->found = true;
					return true;
				}
				continue;
			}
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
				WARNING("Ignored empty code");
				continue;
			}

			const string raw_dats0 = dats[0];
			boost::to_upper(dats[0]);
			if (dats[0].starts_with('!')) // command
			{
				const vector<string> subcmd = split_space(dats[0]);

				if (subcmd[0] == "!SLP")
				{
					int wait = SLP_DEFAULT_MS;
					if (subcmd.size() >= 2)
					{
						if (subcmd.size() > 2 || !try_int(subcmd[1], wait, 500))
							ERROR_RETURN("Parse error");
					}
					MacroCode d(MacroCode_SLP, COMPILE_INFO, rep);
					d.intargs.emplace_back(wait);
					result.macro.emplace_back(d);
				}
				else if (subcmd[0] == "!SLPL")
				{
					MacroCode d(MacroCode_SLP, COMPILE_INFO, rep);
					d.intargs.emplace_back(SLPL_MS);
					result.macro.emplace_back(d);
				}
				else if (subcmd[0] == "!CALL")
				{
					if (subcmd.size() != 2 && subcmd.size() != 3)
					{
						ERROR_RETURN("Arg error");
					}
					int lim = MAX_CALL_DEPTH;
					if (subcmd.size() == 3 && !try_int(subcmd[2], lim, 0))
					{
						ERROR_RETURN("Parse error");
					}
					const auto rid = m_routine_table.getId(subcmd[1]);
					MacroCode d(MacroCode_CALL, COMPILE_INFO, rep);
					d.intargs.emplace_back(rid);
					d.intargs.emplace_back(lim);
					result.macro.emplace_back(d);
				}
				else if (subcmd[0] == "!TSTART")
				{
					result.macro.emplace_back(MacroCode_TSTART, COMPILE_INFO, rep);
				}
				else if (subcmd[0] == "!QUIT")
				{
					result.macro.emplace_back(MacroCode_QUIT, COMPILE_INFO, rep);
				}
				else if (subcmd[0] == "!SHOWFRAMES")
				{
					result.macro.emplace_back(MacroCode_SHOWFRAMES, COMPILE_INFO, rep);
				}
				else if (subcmd[0] == "!SEQCFG")
				{
					vector<int> args(3);
					if (subcmd.size() != 4 ||
						!try_int(subcmd[1], args[0], 0) ||
						!try_int(subcmd[2], args[1], 0) ||
						!try_int(subcmd[3], args[2], 0) ||
						!keyseq_config_validate(args[0], args[1], args[2]))
					{
						ERROR_RETURN("Parse error");
					}
					MacroCode d(MacroCode_SEQCFG, COMPILE_INFO, rep);
					d.intargs.swap(args);
					result.macro.emplace_back(d);
				}
				// else if (subcmd[0] == "!!WAITEV") // !!: consume a frame
				//{
				//	if (subcmd.size() != 2 || subcmd[1] != "SMM147_LEVEL_LOAD") // todo improve
				//	{
				//		ERROR_RETURN("Parse error");
				//	}
				//	MacroCode d(MacroCode_EV_WAIT, COMPILE_INFO, rep);
				//	d.intargs.emplace_back(WaitEvent_SMM147_LEVEL_LOADED);
				//	d.input_status = VPAD_NULL;
				//	d.strargs.emplace_back(m_mapping.empty_show);
				//	result.macro.push_back(d);
				// }
				else
				{
					ERROR_RETURN(format("Unknown command: \"{}\"", subcmd[0]));
				}
			}
			else if (dats[0].starts_with('#'))
			{
				const vector<string> subcmd = split_space(dats[0]);
				const vector<string> raw_subcmd = split_space(raw_dats0);
				if (rep > 1)
				{
					WARNING(format("Can't repeat preprocessors: {}", subcmd[0]));
				}
				// preprocessor
				if (subcmd[0] == "#SUB")
				{
					if (subcmd.size() != 2)
					{
						ERROR_RETURN("Arg error");
					}
					const auto rid = m_routine_table.getId( subcmd[1]);
					if (result.sub_macro_table.contains(rid))
					{
						ERROR_RETURN(format("\"{}\" is already defined", subcmd[1]));
					}

					CompiledMacro sub_macro;
					if (!compile(macro_stream, sub_macro, file_path, lineno, true))
						return false;

					result.sub_macro_table.emplace(rid, sub_macro);
				}
				else if (subcmd[0] == "#COUNTUP")
				{
					if (m_option.ignoreCountUp)
					{
						INFO("Ignored: #COUNTUP");
						continue;
					}
					if (subcmd.size() != 2)
						ERROR_RETURN("Arg error");
					filesystem::path path;
					if (!try_eval_relative_weak(filesystem::path(raw_subcmd.back()), file_path.parent_path(), path))
						ERROR_RETURN("Path error:" + raw_subcmd.back());
					int now_count = 0;
					if (fs::exists(path))
					{
						ifstream countfile(path);
						if (!countfile.is_open())
							ERROR_RETURN("File cannot open:" + path.string());
						string countstr;
						std::getline(countfile, countstr);
						boost::trim(countstr);
						if (!countstr.empty() && !try_int(countstr, now_count, 0))
							ERROR_RETURN("Cannot parse current count:" + path.string());
					}


					{
						ofstream countfile(path);
						if (!countfile.is_open())
							ERROR_RETURN("Error writing to file" + path.string());
						countfile << now_count + 1 << endl;
						countfile.close();
					}

				}
				else if (subcmd[0] == "#LOAD")
				{
					if (subcmd.size() < 2 && subcmd.size() > 4)
					{
						ERROR_RETURN("Arg error");
					}
					
					filesystem::path path;
					if (!try_eval_relative(filesystem::path(raw_subcmd.back()), file_path.parent_path(), path) || !filesystem::exists(path))
					{
						ERROR_RETURN("File not found:" + path.string());
					}
					const auto load_file = m_file_table.getId(path.string());
					if (!m_parent_file_ids.emplace(load_file).second)
					{
						ERROR_RETURN("Found recursive #LOAD");
					}
					if (subcmd.size() == 2)
					{
						if (!compile(path, result))
							return false;
					}
					else
					{
						Label start(subcmd[1]);
						if (subcmd.size() == 3)
						{
							if (!compile(path, {&start, nullptr}, result))
								return false;
							if (!start.found)
								WARNING(format("Undefined label \"{}\"", subcmd[1]));
						}
						else
						{
							Label last(subcmd[2]);
							if (!compile(path, {&start, &last}, result))
								return false;
							if (!start.found)
							{
								WARNING(format("Undefined label \"{}\"", subcmd[1]));
							}
							else if (!last.found)
							{
								WARNING(format("Undefined label \"{}\"", subcmd[2]));
							}
						}

					}

					m_parent_file_ids.erase(load_file);
				}
				else if (subcmd[0] == "#LOOP")
				{
					int loop_cout;
					if (subcmd.size() != 2 || !try_int(subcmd[1], loop_cout, 0))
					{
						ERROR_RETURN("Arg error");
					}
					CompiledMacro sub_macro;
					if (!compile(macro_stream, sub_macro, file_path, lineno, true))
						return false;
					const auto internal_id = m_routine_table.skip();
					MacroCode d(MacroCode_CALL, COMPILE_INFO, loop_cout);
					d.intargs.emplace_back(internal_id);
					d.intargs.emplace_back(MAX_CALL_DEPTH);
					result.macro.emplace_back(d);

					result.sub_macro_table.emplace(internal_id, sub_macro);
				}
				else if (subcmd[0] == "#ENDSUB" || subcmd[0] == "#ESUB" || subcmd[0] == "#END")
				{
					if (!require_end)
						ERROR_RETURN(format("Unexcepted {}", subcmd[0]));
					return true;
				}
				else
				{
					ERROR_RETURN(format("Unknown preprocessor \"{}\"", subcmd[0]));
				}
			}
			else // macro
			{
				bool check = true;
				// copy
				string show = m_mapping->empty_show;
				uint32be status = 0;
				for (const char& c : dats[0])
				{
					if (isspace(c))
						continue;
					if (m_mapping->to_key.contains(c))
					{
						if (m_mapping->to_show_index.at(c) <= 0)
							continue;
						status |= m_mapping->to_key.at(c);
						show[m_mapping->to_show_index.at(c) - 1] = m_mapping->to_show.at(c);
					}
					else
					{
						check = false;
						continue;
					}
				}
				if (!check)
					WARNING("Contains unknown key");

				MacroCode d(MacroCode_Input, COMPILE_INFO, show, rep);
				d.input_status = status;
				result.macro.emplace_back(d);
			}
		}

		// eof
		lineno++;
		if (require_end)
			ERROR_RETURN("Excepted: END");
			
		return true;
	}

	bool compile(const filesystem::path& file_path, CompiledMacro& result)
	{
		std::ifstream infile(file_path);
		int lineno = 0;
		const bool ret = compile(infile, result, filesystem::absolute(file_path), lineno, false);
		infile.close();
		return ret;
	}

	bool compile(const filesystem::path& file_path, const pair<Label*, Label*>& labels, CompiledMacro& result)
	{
		std::ifstream infile(file_path);
		int lineno = 0;
		string line;
		string label_data;
		assert(labels.first != nullptr);
		while (std::getline(infile, line))
		{
			lineno++;
			deleteComment(line);
			boost::trim(line);
			if (getLabelName(line, label_data) && label_data == labels.first->name)
			{
				labels.first->found = true;
				break;
			}
		}
		if (!labels.first->found)
			return true;
		const bool ret = compile(infile, result, filesystem::absolute(file_path), lineno, false, labels.second);
		infile.close();
		return ret;
	}

	bool checkCalls(const CompiledMacro& macro, Routines& routine, const int depth) {
		for (const auto& code : macro.macro)
		{
			if (code.type == MacroCode_CALL)
			{
				if (depth >= MAX_CALL_DEPTH)
				{
					string file_path_str;
					m_file_table.searchInverse(code.compile_info.file_id, file_path_str);
					const filesystem::path file_path(file_path_str);
					const int lineno = code.compile_info.line_no;
					ERROR_RETURN(format("Subroutine call reaches depth limit ({})", MAX_CALL_DEPTH));
				}
				if (depth >= code.intargs.at(1))
					continue;

				const auto next = routine.get(code.intargs.at(0));
				if (next == nullptr)
				{
					string file_path_str;
					m_file_table.searchInverse(code.compile_info.file_id, file_path_str);
					const filesystem::path file_path(file_path_str);
					const int lineno = code.compile_info.line_no;
					string sub_name;
					m_routine_table.searchInverse(code.intargs.at(0), sub_name);
					ERROR_RETURN(format("Undefined subroutine \"{}\"", sub_name));
				}
				else
				{
					routine.push(&next->sub_macro_table);
					if (!checkCalls(*next, routine, depth + 1))
					{
						return false;
					}
					routine.pop();
				}
			}
		}
		return true;
	}
}; // struct Compiler

bool compile(const filesystem::path& file, const MappingData& mapping, CompileResult& result, const CompileOption& option)
{
	Compiler compiler(&mapping, option);
	compiler.init();
	bool ret = compiler.run(file, result.macro);
	if (ret)
	{
		ret &= compiler.check(result.macro);
	}
	result.messages.swap(compiler.getMessages());
	return ret;
}

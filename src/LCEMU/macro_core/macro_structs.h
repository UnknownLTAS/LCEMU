#pragma once
#include <unordered_map>
#include <string>

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
	MacroCode_SHOWFRAMES,
	MacroCode_SEQCFG,
	MacroCode_EV_WAIT,
};

struct MappingData
{
	std::unordered_map<char, uint32> to_key;
	std::unordered_map<char, char> to_show;
	std::unordered_map<char, int> to_show_index;
	char idxtoshow[16];
	std::string empty_show;
	std::string all_show;
	void build_show_data(const size_t& sz, const size_t& offset) {
		all_show.resize(sz);
		empty_show.assign(sz, ' ');
		for (size_t i = 0; i < sz; i++)
			all_show[i] = idxtoshow[offset + i];
	}
	void set(const char& c, const uint32& m, const char& s, const int& idx) {
		to_key[c] = m;
		to_show[c] = s;
		to_show_index[c] = idx;
		idxtoshow[idx] = s;
	}
};

struct CompileInfo
{
	int file_id;
	int line_no;
};

struct MacroCode
{
	MacroCodeType type = MacroCode_None;
	int repeat = 0;
	bool eof = false;
	CompileInfo compile_info = {};
	std::vector<std::string> strargs;
	std::vector<int> intargs;
	uint32be input_status = 0;
	MacroCode(const MacroCodeType& type, const CompileInfo& compile_info, const int rep)
		: type(type), compile_info(compile_info), repeat(rep) {}
	MacroCode(const MacroCodeType& type, const CompileInfo& compile_info, const std::string& first_str, const int rep)
		: type(type), compile_info(compile_info), repeat(rep)
	{
		strargs.emplace_back(first_str);
	}
	MacroCode() {}
	MacroCode(const MacroCodeType& type)
		  : type(type) {}

};

struct CompiledMacro
{
	std::vector<MacroCode> macro;
	std::map<int, CompiledMacro> sub_macro_table;
	void clear() {
		macro.clear();
		for (auto& [_, sub] : sub_macro_table)
			sub.clear();
		sub_macro_table.clear();
	}
};

struct RunInfo
{
	RunInfo* parent = nullptr;
	CompiledMacro* target = nullptr;
	int now_step = 0;
	int now_pos = 0;
	int depth = 0;
};

struct Routines
{
	std::vector<std::map<int, CompiledMacro>*> data;
	void push(std::map<int, CompiledMacro>* table)
	{
		data.emplace_back(table);
	}
	void pop() {
		if (!data.empty())
			data.pop_back();
	}
	CompiledMacro* get(const int& name) const {
		if (data.empty())
			return nullptr;
		for (int i = data.size() - 1; i >= 0; i--)
		{
			if (data[i]->contains(name))
			{
				return &data[i]->at(name);
			}
		}
		return nullptr;
	}
	void clear() {
		data.clear();
	}

};

struct Label
{
	std::string name;
	bool found = false;
	Label(const std::string& _name)
		: name(_name) {}
};


enum MacroMessageType
{
	MSG_ERROR,
	MSG_WARNING,
	MSG_INFO
};

struct MacroMessage
{
	MacroMessageType type;
	std::string message;
	std::filesystem::path file;
	int line;
	MacroMessage(const MacroMessageType& type, const std::string& message, const std::filesystem::path& file, const int& line)
		: type(type), message(message), file(file), line(line)
	{}
};


struct CompileResult
{
	CompiledMacro macro;
	std::vector<MacroMessage> messages;
};
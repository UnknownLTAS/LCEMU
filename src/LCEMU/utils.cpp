#include "utils.h"
using namespace std;

template<typename T>
static T convert_int(const std::string& _str, T& tar, const T& fail, function<T(string&, int&)> fn)
{
	try
	{
		auto str = boost::trim_copy(_str); // need copy
		int base = 10;
		if (str.starts_with("0x") || str.starts_with("0X"))
			base = 16;
		else if (str.starts_with("0b") || str.starts_with("0B"))
		{
			 // need?
		}
		tar = fn(str, base);
		return true;
	} catch (...)
	{
		tar = fail;
		return false;
	}
}

bool try_int(const std::string& str, int& tar, const int& fail)
{
	return convert_int<int>(str, tar, fail, [](string& s, int& base) { return stoi(s, nullptr, base); });
}

bool try_ll(const std::string& str, long long int& tar, const long long int& fail)
{
	return convert_int<long long int>(str, tar, fail, [](string& s, int& base) { return stoll(s, nullptr, base); });
}

bool try_ull(const std::string& str, unsigned long long int& tar, const unsigned long long int& fail)
{
	return convert_int<unsigned long long int>(str, tar, fail, [](string& s, int& base) { return stoull(s, nullptr, base); });
}

bool try_dbl(const std::string& _str, double& tar, const double fail)
{
	try
	{
		auto str = boost::trim_copy(_str); // need copy
		tar = stod(str);
		return true;
	} catch (...)
	{
		tar = fail;
		return false;
	}
}


std::vector<std::string> split_space(const std::string& str)
{
	std::vector<std::string> res;
	std::string now;
	int pos = 0;
	while (pos <= str.size())
	{
		if (pos == str.size() || isspace(str[pos]))
		{
			if (!now.empty())
			{
				res.push_back(now);
				now.clear();
			}
		}
		else
		{
			now.push_back(str[pos]);
		}
		pos++;
	}
	if (res.empty())
		res.push_back("");
	return res;
}

bool try_eval_relative(const std::filesystem::path& p, const std::filesystem::path& bs, std::filesystem::path& ret)
{
	if (p.is_relative())
	{
		try
		{
			ret = std::filesystem::canonical(bs / p);
			return true;
		} catch (...)
		{
			return false;
		}
	}
	else
	{
		ret = p;
		return true;
	}
}

const void put_with_color(const string& mes, const int color)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO s;
	GetConsoleScreenBufferInfo(console, &s);
	SetConsoleTextAttribute(console, color);
	cout << mes << endl;
	SetConsoleTextAttribute(console, s.wAttributes);
}

void clear_screen(char fill)
{
	COORD tl = {0, 0};
	CONSOLE_SCREEN_BUFFER_INFO s;
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console, &s);
	DWORD written, cells = s.dwSize.X * s.dwSize.Y;
	FillConsoleOutputCharacter(console, fill, cells, tl, &written);
	FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
	SetConsoleCursorPosition(console, tl);
}

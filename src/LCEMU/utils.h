#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <windows.h>
#include <dwmapi.h>

bool try_int(const std::string& str, int& tar, const int& fail);
bool try_ll(const std::string& str, long long int& tar, const long long int& fail);
bool try_ull(const std::string& str, unsigned long long int& tar, const unsigned long long int& fail);
bool try_dbl(const std::string& _str, double& tar,const double fail);

std::vector<std::string> split_space(const std::string& str);

bool try_eval_relative(const std::filesystem::path& p, const std::filesystem::path& bs, std::filesystem::path& ret);
const void put_with_color(const std::string& mes, const int color);
void clear_screen(char fill = ' ');
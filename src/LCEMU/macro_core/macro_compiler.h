#pragma once
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <string>
#include "./macro_structs.h"

using namespace std;
constexpr int MAX_CALL_DEPTH = 512;

bool compile(const filesystem::path& file, const MappingData& mapping, CompileResult& result);
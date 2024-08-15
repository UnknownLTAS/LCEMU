#pragma once
#include <thread>
#include <windows.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>


void ext_init();
bool ext_onframe(uint32be&);

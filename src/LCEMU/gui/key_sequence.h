#pragma once
#include <windows.h>
#include <thread>
#include <string>
#include <vector>
#include "gui_utils.h"
#include "../macro_manager.h"

void keyseq_init(std::string&);
void keyseq_set(std::vector<std::string>&, int);
void keyseq_reset();
void keyseq_on_frame(const MacroManager&);
void keyseq_set_window_pos(int x, int y);

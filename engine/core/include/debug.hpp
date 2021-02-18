#pragma once

#include <string_view>

void draw_debug_ui();

void load_debug_options();
void save_debug_options();

std::string_view get_shader_source_directory();

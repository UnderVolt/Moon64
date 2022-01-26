#pragma once

#include <vector>
#include <string>

void disassemble_sound(std::string out, std::vector<std::string> samples);
void assemble_banks(std::string banks, std::string dir);
void assemble_m64(std::string banks, std::string dir);
void bind_engine();
void end();
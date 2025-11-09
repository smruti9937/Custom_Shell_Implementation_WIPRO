#pragma once
#include <vector>
#include <string>

// Returns true if the command was handled by a builtin (cd/exit)
bool handle_builtin(const std::vector<std::string>& args);

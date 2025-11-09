#pragma once
#include <vector>
#include <string>

// Execute a command (no piping, no redirection yet).
// If background == true, do not wait for child.
void execute_simple_command(const std::vector<std::string> &args, bool background);

void execute_pipeline(const std::vector<std::vector<std::string>> &segments, bool background);
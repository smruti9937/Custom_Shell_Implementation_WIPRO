#pragma once
#include <sys/types.h>

// Foreground pid that signal handlers will forward to.
// Defined in main.cpp; other modules can use it (extern).
extern volatile pid_t g_foreground_pid;

// Install signal handlers (SIGINT, SIGTSTP, SIGCHLD).
void setup_signal_handlers();

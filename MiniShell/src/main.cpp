#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"
#include "builtins.h"
#include "executor.h"
#include "jobs.h"
#include "shell.h"

// Definition of global foreground pid
volatile pid_t g_foreground_pid = 0;

// SIGINT handler: forward Ctrl-C to foreground process (if any)
static void sigint_handler(int) {
    if (g_foreground_pid > 0) {
        kill(g_foreground_pid, SIGINT);
    } else {
        // If no foreground job, redisplay prompt newline
        std::cout << "\n";
    }
}

// SIGTSTP handler: forward Ctrl-Z to foreground process (if any)
static void sigtstp_handler(int) {
    if (g_foreground_pid > 0) {
        kill(g_foreground_pid, SIGTSTP);
    } else {
        std::cout << "\n";
    }
}

// SIGCHLD handler: reap children and mark jobs as exited
static void sigchld_handler(int) {
    // Use waitpid with WNOHANG in a loop
    while (true) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) break;
        // If process exited normally or by signal, mark job exited (and remove if desired)
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            mark_job_exited(pid);
            // We don't remove immediately to allow 'jobs' to show recently finished; remove to avoid stale entries:
            remove_job(pid);
        }
    }
}

void setup_signal_handlers() {
    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);

    struct sigaction sa_tstp;
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, nullptr);

    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);
}

static bool is_all_space(const std::string &s) {
    for (char c : s) if (!isspace(static_cast<unsigned char>(c))) return false;
    return true;
}

int main() {
    setup_signal_handlers();

    std::string line;
    while (true) {
        // Display prompt
        std::cout << "miniShell> " << std::flush;
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl-D) or input closed
            std::cout << "\n";
            break;
        }
        if (is_all_space(line)) continue;

        auto tokens = tokenize(line);
        if (tokens.empty()) continue;

        // Handle background operator (& at end)
        bool background = false;
        if (!tokens.empty() && tokens.back() == "&") {
            background = true;
            tokens.pop_back();
        }

        // If builtin and single segment (no pipes), run builtins directly
        // But builtins like fg/bg/jobs assume simple tokens. We'll only check builtins before exec when there's no '|'
        bool contains_pipe = false;
        for (const auto &t : tokens) if (t == "|") { contains_pipe = true; break; }

        if (!contains_pipe && handle_builtin(tokens)) {
            continue;
        }

        // If there is a pipeline, split tokens into segments
        std::vector<std::vector<std::string>> segments;
        std::vector<std::string> current;
        for (const auto &t : tokens) {
            if (t == "|") {
                segments.push_back(current);
                current.clear();
            } else {
                current.push_back(t);
            }
        }
        if (!current.empty()) segments.push_back(current);

        if (segments.size() > 1) {
            execute_pipeline(segments, background);
        } else {
            // single segment: if builtin wasn't handled earlier, execute as external command
            execute_simple_command(segments.front(), background);
        }
    }

    return 0;
}

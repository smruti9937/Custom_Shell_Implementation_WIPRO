#include "builtins.h"
#include "jobs.h"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>

// Helper to print jobs cleanly
static void print_jobs() {
    auto jobs = list_jobs();
    for (const auto &j : jobs) {
        std::cout << "[" << j.pid << "] "
                  << (j.running ? "Running" : "Stopped/Done")
                  << "  " << j.cmdline << "\n";
    }
}

bool handle_builtin(const std::vector<std::string>& args) {
    if (args.empty()) return false;
    const std::string &cmd = args[0];

    if (cmd == "exit") {
        int code = 0;
        if (args.size() >= 2) code = std::atoi(args[1].c_str());
        std::exit(code);
    }

    if (cmd == "cd") {
        const char *path = nullptr;
        if (args.size() >= 2) path = args[1].c_str();
        if (!path) path = getenv("HOME");
        if (!path) path = "/";
        if (chdir(path) != 0) {
            perror("cd");
        }
        return true;
    }

    if (cmd == "jobs") {
        print_jobs();
        return true;
    }

    if (cmd == "fg") {
        if (args.size() < 2) {
            std::cerr << "fg: usage: fg <pid>\n";
            return true;
        }
        pid_t pid = static_cast<pid_t>(std::stoi(args[1]));
        if (kill(pid, SIGCONT) < 0) perror("kill(SIGCONT)");
        // Wait for the pid in foreground
        int status = 0;
        if (waitpid(pid, &status, WUNTRACED) < 0) perror("waitpid");
        if (WIFSTOPPED(status)) {
            // if stopped again, keep the job
            mark_job_exited(pid); // mark not running/stopped
        } else {
            remove_job(pid);
        }
        return true;
    }

    if (cmd == "bg") {
        if (args.size() < 2) {
            std::cerr << "bg: usage: bg <pid>\n";
            return true;
        }
        pid_t pid = static_cast<pid_t>(std::stoi(args[1]));
        if (kill(pid, SIGCONT) < 0) perror("kill(SIGCONT)");
        mark_job_running(pid);
        return true;
    }

    return false; // not a builtin
}

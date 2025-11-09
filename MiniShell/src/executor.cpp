#include "executor.h"
#include "jobs.h"
#include "shell.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>

static char** vec_to_cstr(const std::vector<std::string>& args) {
    char **argv = new char*[args.size() + 1];
    for (size_t i = 0; i < args.size(); ++i)
        argv[i] = strdup(args[i].c_str());
    argv[args.size()] = nullptr;
    return argv;
}

static void apply_redirections(const std::vector<std::string> &tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "<" && i + 1 < tokens.size()) {
            int fd = open(tokens[i+1].c_str(), O_RDONLY);
            if (fd < 0) {
                perror(("open " + tokens[i+1]).c_str());
                _exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 stdin"); _exit(1); }
            close(fd);
        } else if (tokens[i] == ">" && i + 1 < tokens.size()) {
            int fd = open(tokens[i+1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(("open " + tokens[i+1]).c_str());
                _exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 stdout"); _exit(1); }
            close(fd);
        }
    }
}

static std::vector<std::string> cleaned_tokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if ((tokens[i] == "<" || tokens[i] == ">") && i + 1 < tokens.size()) {
            ++i; // skip filename
            continue;
        }
        out.push_back(tokens[i]);
    }
    return out;
}

void execute_simple_command(const std::vector<std::string> &args, bool background) {
    if (args.empty()) return;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        // Child process: apply redirections and exec.
        apply_redirections(args);
        auto argv_tokens = cleaned_tokens(args);
        if (argv_tokens.empty()) _exit(0);
        char **argv = vec_to_cstr(argv_tokens);
        execvp(argv[0], argv);
        // If exec fails:
        perror("execvp");
        // cleanup
        for (size_t i = 0; argv[i]; ++i) free(argv[i]);
        delete[] argv;
        _exit(127);
    } else {
        // Parent
        if (background) {
            add_job(pid, args.empty() ? std::string() : args[0]);
            std::cout << "[bg] started pid: " << pid << "\n";
        } else {
            // foreground: set global fid for signal forwarding
            g_foreground_pid = pid;
            int status = 0;
            // use WUNTRACED to detect if child stopped (SIGTSTP)
            if (waitpid(pid, &status, WUNTRACED) < 0) {
                perror("waitpid");
            }
            if (WIFSTOPPED(status)) {
                // add to job list and mark not running (stopped)
                add_job(pid, args.empty() ? std::string() : args[0]);
            } else {
                // normal exit -> ensure job removed
                remove_job(pid);
            }
            g_foreground_pid = 0;
        }
    }
}

void execute_pipeline(const std::vector<std::vector<std::string>> &segments, bool background) {
    if (segments.empty()) return;
    size_t n = segments.size();

    // Create pipes (n-1)
    std::vector<int> pipes;
    pipes.resize(2 * (n > 0 ? n-1 : 0));
    for (size_t i = 0; i + 1 < n; ++i) {
        if (pipe(&pipes[2*i]) < 0) {
            perror("pipe");
            return;
        }
    }

    std::vector<pid_t> pids(n, 0);

    for (size_t i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // close pipes
            for (int fd : pipes) if (fd > 0) close(fd);
            return;
        }
        if (pid == 0) {
            // Child
            // If not first, set stdin to previous pipe read end
            if (i != 0) {
                if (dup2(pipes[2*(i-1)], STDIN_FILENO) < 0) { perror("dup2 stdin"); _exit(1); }
            }
            // If not last, set stdout to this pipe write end
            if (i != n-1) {
                if (dup2(pipes[2*i + 1], STDOUT_FILENO) < 0) { perror("dup2 stdout"); _exit(1); }
            }
            // Close all pipe fds in child
            for (size_t j = 0; j < pipes.size(); ++j) {
                close(pipes[j]);
            }
            // apply redirection tokens (each segment may have < or >)
            apply_redirections(segments[i]);

            auto argv_tokens = cleaned_tokens(segments[i]);
            if (argv_tokens.empty()) _exit(0);
            char **argv = vec_to_cstr(argv_tokens);
            execvp(argv[0], argv);
            perror("execvp");
            _exit(127);
        } else {
            // Parent
            pids[i] = pid;
        }
    }

    // Close all pipe fds in parent
    for (size_t j = 0; j < pipes.size(); ++j) close(pipes[j]);

    // Background or foreground handling:
    if (background) {
        // Track first process as job representative
        add_job(pids.front(), "pipeline");
        std::cout << "[bg] pipeline started pid: " << pids.front() << "\n";
    } else {
        // Wait for all children; if any stopped, add to jobs
        g_foreground_pid = pids.back(); // one of the child pids (approximation)
        for (size_t i = 0; i < pids.size(); ++i) {
            int status = 0;
            if (waitpid(pids[i], &status, WUNTRACED) < 0) {
                perror("waitpid");
            }
            if (WIFSTOPPED(status)) {
                add_job(pids[i], "pipeline");
            } else {
                remove_job(pids[i]);
            }
        }
        g_foreground_pid = 0;
    }
}



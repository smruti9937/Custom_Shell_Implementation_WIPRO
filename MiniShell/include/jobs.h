#pragma once
#include <string>
#include <vector>
#include <sys/types.h>

struct Job {
    pid_t pid;
    std::string cmdline;
    bool running; // true if running, false if stopped/finished
};

void init_job_system();
void add_job(pid_t pid, const std::string &cmdline);
void remove_job(pid_t pid);
std::vector<Job> list_jobs();
void mark_job_exited(pid_t pid); // called by SIGCHLD handler
void mark_job_running(pid_t pid);

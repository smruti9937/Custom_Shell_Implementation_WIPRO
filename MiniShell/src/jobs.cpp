#include "jobs.h"
#include <vector>
#include <algorithm>
#include <mutex>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

static std::vector<Job> g_jobs;
static std::mutex g_jobs_mutex;

void add_job(pid_t pid, const std::string &cmdline) {
    std::lock_guard<std::mutex> lk(g_jobs_mutex);
    g_jobs.erase(std::remove_if(g_jobs.begin(), g_jobs.end(),
        [pid](const Job &j){ return j.pid == pid; }), g_jobs.end());
    g_jobs.push_back(Job{pid, cmdline, true});
}

void remove_job(pid_t pid) {
    std::lock_guard<std::mutex> lk(g_jobs_mutex);
    g_jobs.erase(std::remove_if(g_jobs.begin(), g_jobs.end(),
        [pid](const Job& j){ return j.pid == pid; }), g_jobs.end());
}

std::vector<Job> list_jobs() {
    std::lock_guard<std::mutex> lk(g_jobs_mutex);
    return g_jobs;
}

void mark_job_exited(pid_t pid) {
    std::lock_guard<std::mutex> lk(g_jobs_mutex);
    for (auto &j : g_jobs) {
        if (j.pid == pid) j.running = false;
    }
}

void mark_job_running(pid_t pid) {
    std::lock_guard<std::mutex> lk(g_jobs_mutex);
    for (auto &j : g_jobs) {
        if (j.pid == pid) j.running = true;
    }
}

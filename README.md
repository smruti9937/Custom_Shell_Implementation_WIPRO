# Custom Shell (UNIX-like) — C++

A modular UNIX-like command shell written in modern C++.  
Supports built-ins, external commands, pipes, I/O redirection, background jobs, and signal handling.

![screenshot](docs/screenshot.png)

---

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Build & Run](#build--run)
- [Usage Examples](#usage-examples)
- [Design Notes](#design-notes)
- [Troubleshooting](#troubleshooting)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

---

## Overview
This project implements a **custom command shell** in C++ that mimics basic Linux terminal behavior.

**Core capabilities**
- Command parsing with quotes (`" "` and `' '`).
- Built-ins: `cd`, `exit`, `jobs`, `fg`, `bg`.
- External command execution: `ls`, `grep`, `echo`, etc.
- Background processes with `&`.
- Pipelining with `|` and I/O redirection (`>`, `>>`, `<`).
- Signal handling for `Ctrl+C` and `Ctrl+Z`.
- Job control & status reporting.

---

## Features

| Feature               | Description                              | Example                                   |
|-----------------------|------------------------------------------|-------------------------------------------|
| Command Execution     | Run external Linux commands              | `ls -la`, `cat file.txt`                  |
| Built-ins             | Internal shell commands                  | `cd /tmp`, `jobs`, `fg %1`, `bg %2`, `exit` |
| Pipes                 | Chain command outputs                     | `ls | grep .cpp`                           |
| Redirection           | I/O redirect (`>`, `>>`, `<`)            | `ls > out.txt`, `grep hi < in.txt`        |
| Background Jobs       | Add `&` to run in background             | `sleep 5 &`                               |
| Signal Handling       | Handle interrupts/stops                  | `Ctrl+C` (SIGINT), `Ctrl+Z` (SIGTSTP)     |
| Job Control           | Track/resume/bring back jobs             | `jobs`, `fg %1`, `bg %1`                  |

---

## Project Structure


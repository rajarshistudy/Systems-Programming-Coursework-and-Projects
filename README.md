# Systems-Programming-Coursework-and-Projects
This repository contains the major programming assignments, concepts, and implementations from my Systems Programming course at UMass Amherst. The work progresses from low-level Unix concepts to building full systems-level software, culminating in a working Unix-like shell implemented in C.

## Overview
The projects in this repository reflect a student's journey into how operating systems and user-level programs interact. Early assignments introduced system calls, process control, file descriptors, and signals. Later work focused on concurrency, memory management, signal safety, and robust error handling, all leading up to implementing a complete shell.

##  Key Projects
### Unix Shell (tsh)
A fully functional Unix-style shell written in C with:
- Job control (fg/bg)
- Process groups and signal handling
- I/O redirection and pipelines
- Error handling and signal masking
- Builtins and command evaluation logic

### Process Management & Signals
Assignments implementing:
- Fork/exec process flow  
- Signal masks  
- Safe handlers for SIGCHLD, SIGINT, and SIGTSTP  
- Zombie process prevention  

### Memory & File System Interactions
Tasks involving:
- Heap allocation  
- File descriptor manipulation  
- Buffering and I/O behavior  

## Technologies & Concepts
- C programming (systems-level)  
- UNIX  
- Signals, processes, pipes, job control  
- Debugging with gdb   
- Performance and correctness under real constraints

## Purpose
This repository demonstrates hands-on systems engineering; understanding how OS abstractions are built and implementing real functionality from the ground up. The final shell project reflects my ability to write robust, low-level software with attention to correctness, safety, and edge-case behavior. It also serves to document any and all my work related to Systems Engineering. The Repository has been designed such that it can be used for reference in future projects.



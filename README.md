# Multi-threaded-DNS-Resolver
Eric David, Fall 2025

A multi-threaded DNS resolver implemented in C using POSIX threads. Supports concurrent hostname ingestion and resolution through a producer–consumer architecture with mutex synchronization, shared buffers, audit-friendly logging, and controlled thread teardown via poison-pill signaling. This project uses a shared array implemented as a circular queue for inter-thread communication.

1. To build the program, navigate to the directory containing the multi-lookup.c, util.c, array.c files.
2. run `make` to build the program.
3. run `make clean` to cleanup any extra .o files or executables.
4. run `./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]` to execute the program. 

For example, `./multi-lookup 10 10 serviced.txt resolved.txt input/names*.txt` will run the program with 10 requester threads, 10 resolver threads, a requester log "serviced.txt", a resolver log "resolved.txt", and input files matching the pattern names*.txt (names1.txt, names2.txt, etc.) from the input directory. 

The input file is included as well with example domain names to test with.


# Multi-threaded-DNS-Resolver
A multi-threaded DNS resolver implemented in C using POSIX threads. Supports concurrent hostname ingestion and resolution through a producer–consumer architecture with mutex synchronization, shared buffers, audit-friendly logging, and controlled thread teardown via poison-pill signaling.

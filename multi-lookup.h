#ifndef MUTLI_LOOKUP_H
#define MUTLI_LOOKUP_H

#include "array.h"
#include "util.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>

// array size and max name length defined in array.h
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

typedef struct{
 
    char *requestor_logfile; //name of file to write the domain names into
    array *shared; // our shared array

    char **inputted_files; //list of inputted files
    int max_files; //total amount of files
    int *current_file; //counter to track current file
    
    pthread_mutex_t *logfile_lock; //mutex lock on log file
    pthread_mutex_t *current_file_lock; //lock for current_file file index
    pthread_mutex_t *stdout_lock; //lock on stdout
}requester;

typedef struct{

    char *resolver_logfile; //file to write ip addresses to
    array *shared; //shared array to read domain names from
    pthread_mutex_t *logfile_lock; //mutex lock on log file
    pthread_mutex_t *stdout_lock;
}resolver;

void *resolver_func(void *arg);
void *requester_func(void *arg);  



#endif
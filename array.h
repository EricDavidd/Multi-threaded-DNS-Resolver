#ifndef ARRAY_H
#define ARRAY_H

#define ARRAY_SIZE 8
#define MAX_NAME_LENGTH 64

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>


typedef struct {
    char buffer[ARRAY_SIZE][MAX_NAME_LENGTH];
    int head;
    int tail;

    pthread_mutex_t mutex;  // mutex for critical section
    sem_t items_available; //semaphore for consumer  
    sem_t space_available; //semaphore for producer 

}array;

int  array_init(array *s);                   // initialize the array
int  array_put (array *s, char *hostname);   // place element into the array, block when full
int  array_get (array *s, char **hostname);  // remove element from the array, block when empty
void array_free(array *s);                   // free the array's resources


#endif
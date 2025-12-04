#include "array.h"


int  array_init(array *s)                  // initialize the array
{
    s->head = 0;
    s->tail = -1;

    pthread_mutex_init(&s->mutex, NULL);

    //https://man7.org/linux/man-pages/man3/sem_init.3.html
    sem_init(&s->items_available, 0, 0); //set equal to 0 (b/c 0 items have been made)
    sem_init(&s->space_available, 0, ARRAY_SIZE); //set to array size (b/c full array is available)

    return 0;
}

int  array_put (array *s, char *hostname)  // place element into the array, block when full
{
    unsigned int position_to_put;
    sem_wait(&s->space_available);
    pthread_mutex_lock(&s->mutex);

    //putting it in
    position_to_put = (s->tail + 1) % ARRAY_SIZE; //warp around b/c circular queue
    strncpy(s->buffer[position_to_put], hostname, MAX_NAME_LENGTH - 1);
    s->buffer[position_to_put][MAX_NAME_LENGTH - 1] = '\0'; // if longer than buffer

    s->tail = position_to_put;

    pthread_mutex_unlock(&s->mutex);

    sem_post(&s->items_available);   //more item 

    return 0;
}

int  array_get (array *s, char **hostname) // remove element from the array, block when empty
{
    sem_wait(&s->items_available); //decrease item
    pthread_mutex_lock(&s->mutex);

    //remove an element
    const char *host_removed = s->buffer[s->head];
    size_t length = strlen(host_removed) + 1;
    char *dst = (char *)malloc(length);
    if (!dst) {
        pthread_mutex_unlock(&s->mutex);
        sem_post(&s->items_available);
        return -1; // allocation failed
    }
    memcpy(dst, host_removed, length);
    *hostname = dst;

    s->head = (s->head + 1) % ARRAY_SIZE;

    pthread_mutex_unlock(&s->mutex);
    sem_post(&s->space_available);

    return 0;
}

void array_free(array *s)                 // free the array's resources
{
    pthread_mutex_destroy(&s->mutex);
    sem_destroy(&s->items_available);
    sem_destroy(&s->space_available);
}
#include "multi-lookup.h"
#define POISON_PILL "POISON-" 

void *requester_func(void *arg)
{
    //for calculating how loing it takes
    requester *req = (requester *)arg; 
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    int serviced_count = 0;
    char *fileName = malloc(MAX_NAME_LENGTH * sizeof(char));
    char *hostName = malloc(MAX_NAME_LENGTH * sizeof(char));

    while (1)
    {
        pthread_mutex_lock(req->current_file_lock); //protect current file index with lock, make sure no two threads are gonna read the index at same time
        
        // if current index is less than then increment index by 1 (theres more files)
        if (*(req->current_file) < req->max_files)
        {
            //copy filename for us to read 
            strcpy(fileName, req->inputted_files[*(req->current_file)]); 
            *(req->current_file) = *(req->current_file) + 1;
            pthread_mutex_unlock(req->current_file_lock);
        }
        else //index goes over, so no more files to read so end
        {
            pthread_mutex_unlock(req->current_file_lock);

            gettimeofday(&end, NULL);
            long seconds = end.tv_sec - start.tv_sec;
            long microsec  = end.tv_usec - start.tv_usec;

            if (microsec < 0) 
            { 
                seconds -= 1; 
                microsec += 1000000; 
            }
            

            pthread_mutex_lock(req->stdout_lock);
            printf("thread %lu serviced %d files in %ld.%ld seconds\n", pthread_self(), serviced_count, seconds, microsec);
            pthread_mutex_unlock(req->stdout_lock);

            free(fileName);
            free(hostName);
            pthread_exit(NULL);
        }

        FILE *input = fopen(fileName, "r"); //using filename, open it as actual file
        if (!input) //if file can't open
        {   
            pthread_mutex_lock(req->stdout_lock);
            fprintf(stderr, "invalid file %s\n", fileName);
            pthread_mutex_unlock(req->stdout_lock);
            continue; //move on
        }

        serviced_count++;

        while(fgets(hostName, MAX_NAME_LENGTH, input)) //go through and read line by line
        {
            size_t length = strlen(hostName);
            if (length && hostName[length - 1] == '\n') //get rid of newline characters
            {
                hostName[length-1] = '\0';
            }

            char *copy_hostname = strdup(hostName); //duplicate the hostname
            if (!copy_hostname) 
            {
                pthread_mutex_lock(req->stdout_lock);
                fprintf(stderr, "strdup failed for hostname\n");
                pthread_mutex_unlock(req->stdout_lock);
            }
            else if (array_put(req->shared, copy_hostname) != 0) //actually put in shared array
            {
                pthread_mutex_lock(req->stdout_lock);
                fprintf(stderr, "error putting hostname into shared array");
                pthread_mutex_unlock(req->stdout_lock);
                free(copy_hostname);
            }

            pthread_mutex_lock(req->logfile_lock);
            FILE *log = fopen(req->requestor_logfile, "a+"); //open the logging file for requester
            if (log) 
            {
                fprintf(log, "%s\n", hostName);
                fclose(log);
            }
            pthread_mutex_unlock(req->logfile_lock);
            free(copy_hostname);  
        }

        fclose(input);
    }    
}

void *resolver_func(void *arg)
{
    resolver *res = (resolver *)arg; 
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    int serviced_count = 0;

    char ip_string[MAX_IP_LENGTH]; 

    while(1)
    {
        char *hostname = NULL;

        if (array_get(res->shared, &hostname) != 0)  //actually get from head from shared buffer
        {
            continue;
        }

        if (strcmp(hostname, POISON_PILL) == 0) //poison pill, end thread if it is
        {
            free(hostname);

            gettimeofday(&end, NULL);
            long seconds  = end.tv_sec  - start.tv_sec;
            long microsec  = end.tv_usec - start.tv_usec;

            if (microsec < 0) 
            { 
                seconds -= 1; 
                microsec += 1000000; 
            }

            pthread_mutex_lock(res->stdout_lock);
            printf("thread %lu resolved %d hostnames in %ld.%ld seconds\n", pthread_self(), serviced_count, seconds, microsec);
            pthread_mutex_unlock(res->stdout_lock);

            pthread_exit(NULL);
        }

        if (dnslookup(hostname, ip_string, MAX_IP_LENGTH) == UTIL_SUCCESS) // use util function to lookup
        {
            pthread_mutex_lock(res->logfile_lock);
            FILE *log = fopen(res->resolver_logfile, "a"); //we are appending to file
            if (log)
            {
                fprintf(log, "%s, %s\n", hostname, ip_string);
                fclose(log);
            }
            pthread_mutex_unlock(res->logfile_lock);
        }
        else //if lookup fails or can't find it
        {
            pthread_mutex_lock(res->logfile_lock);
            FILE *log = fopen(res->resolver_logfile, "a"); 
            if (log)
            {
                fprintf(log, "%s, NOT_RESOLVED\n", hostname);
                fclose(log);
            }
            pthread_mutex_unlock(res->logfile_lock);
        }
        serviced_count = serviced_count + 1;
        free(hostname);
    }
}

int main(int argc, char **argv)
{
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    if (argc < 6) 
    {

        printf("Missing arguments. Usage: multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
        return -1;
    }
    if (argc > 5 + MAX_INPUT_FILES) // too many files
    {
        fprintf(stderr, "Too many input arguments\n");
        return -1;
    }

    //get number of threads, and get the logs from arguments
    int numOfRequesters = atoi(argv[1]);
    int numOfResolvers  = atoi(argv[2]);
    char *requestLog = argv[3];
    char *resolveLog = argv[4];

    if (numOfRequesters <= 0 || numOfResolvers <= 0) 
    {
        fprintf(stderr, "Need at least one resolver and requester thread.\n");
        return -1;
    }
    if (numOfRequesters > MAX_REQUESTER_THREADS || numOfResolvers > MAX_RESOLVER_THREADS) 
    {
        fprintf(stderr, "Too many resolvers or requester threads.\n");
        return -1;
    }

    //get our logs for the threads
    FILE *t1 = fopen(requestLog, "w+");
    FILE *t2 = fopen(resolveLog, "w+");
    if (!t1 || !t2) 
    {
        fprintf(stderr, "Could not open logs for writing\n");
        if (t1) 
        {
            fclose(t1);
        }
        if (t2)
        {
            fclose(t2);
        }
        return -1;
    }
    fclose(t1);
    fclose(t2);

    //get all the file names and put them in an array
    int file_count = argc - 5;
    char *inputs[file_count]; //array of all the file names, from the arguments in command
    for (int i = 0; i < file_count; i++)
    {
        inputs[i] = argv[5 + i];
    }

    array shared_array;
    array_init(&shared_array);


    //mutex locks
    pthread_mutex_t logfile_lock, current_file_lock, stdout_lock;
    if (pthread_mutex_init(&logfile_lock, NULL) != 0) 
    {
        fprintf(stderr, "Mutex init failed (logfile_lock)\n");
        return -1;
    }
    if (pthread_mutex_init(&current_file_lock, NULL) != 0) 
    {
        fprintf(stderr, "Mutex init failed (current_file_lock)\n");
        pthread_mutex_destroy(&logfile_lock);
        return -1;
    }
    if (pthread_mutex_init(&stdout_lock, NULL) != 0) 
    {
        fprintf(stderr, "Mutex init failed (stdout_lock)\n");
        pthread_mutex_destroy(&logfile_lock);
        pthread_mutex_destroy(&current_file_lock);
        return -1;
    }

    //get thread arguments
    requester *requester_args = malloc(sizeof(*requester_args));
    resolver  *resolver_args = malloc(sizeof(*resolver_args));
    int *current_file_index = malloc(sizeof(int));

    if (!requester_args || !resolver_args || !current_file_index) 
    {
        fprintf(stderr, "failed to allocate for requestor and resolver structs\n");
        free(requester_args); 
        free(resolver_args); 
        free(current_file_index);
        pthread_mutex_destroy(&stdout_lock);
        pthread_mutex_destroy(&current_file_lock);
        pthread_mutex_destroy(&logfile_lock);
        return -1;
    }
    *current_file_index = 0;

    //requester thread
    requester_args->requestor_logfile = requestLog;
    requester_args->shared            = &shared_array;
    requester_args->inputted_files    = inputs;
    requester_args->max_files         = file_count;
    requester_args->current_file      = current_file_index;
    requester_args->logfile_lock      = &logfile_lock;
    requester_args->current_file_lock = &current_file_lock;
    requester_args->stdout_lock       = &stdout_lock;

    //resolver thread
    resolver_args->resolver_logfile  = resolveLog;
    resolver_args->shared            = &shared_array;
    resolver_args->logfile_lock      = &logfile_lock;
    resolver_args->stdout_lock       = &stdout_lock;

    //create threads
    pthread_t requesterThreads[numOfRequesters];
    pthread_t resolverThreads[numOfResolvers];

    for (int i = 0; i < numOfRequesters; i++) 
    {
        pthread_create(&requesterThreads[i], NULL, (void*)requester_func, requester_args);
    }
    for (int i = 0; i < numOfResolvers; i++) 
    {
        pthread_create(&resolverThreads[i], NULL, (void*)resolver_func, resolver_args);
    }

    for (int i = 0; i < numOfRequesters; i++) 
    {
        pthread_join(requesterThreads[i], NULL);
    }

    for (int i = 0; i < numOfResolvers; i++) 
    {
        array_put(&shared_array, POISON_PILL);  //put in poison pill
    }
    
    for (int i = 0; i < numOfResolvers; i++) 
    {
        pthread_join(resolverThreads[i], NULL);
    }


    //free and destroy everything
    pthread_mutex_destroy(&stdout_lock);
    pthread_mutex_destroy(&current_file_lock);
    pthread_mutex_destroy(&logfile_lock);

    free(current_file_index);
    free(requester_args);
    free(resolver_args);

    array_free(&shared_array);

    gettimeofday(&end, NULL);
    long seconds  = end.tv_sec  - start.tv_sec;
    long microsec  = end.tv_usec - start.tv_usec;
    if (microsec < 0) 
    { 
        seconds -= 1; 
        microsec += 1000000; 
    }

    printf("./multi-lookup: total time is %ld.%ld seconds\n", seconds, microsec);

    return 0;
}



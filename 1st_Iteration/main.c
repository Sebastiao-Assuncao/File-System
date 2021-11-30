#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

pthread_mutex_t commandMutex = PTHREAD_MUTEX_INITIALIZER;

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];

int numberCommands = 0;
int headQueue = 0;

void validate_arguments (char* argv[]) {

    if (argv[1] == NULL || argv[2] == NULL) /* Validate input and output files */
    {
        fprintf(stderr, "Error: input or output file unavailable.\n");
        exit(EXIT_FAILURE);
    }

    if ((numberThreads = atoi(argv[3])) < 1)  /* Validate number os threads */
    {
        fprintf(stderr, "Error: number of threads not valid.\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[4], "mutex") !=0 && strcmp(argv[4], "rwlock") != 0 &&
     strcmp(argv[4], "nosync") != 0) /* Validate sync strategy */
     {
        fprintf(stderr, "Error: sync strategy not valid.\n");
        exit(EXIT_FAILURE);
     }
     else
        if ((syncStrategy = argv[4][0]) == 'n' && numberThreads != 1) {
            fprintf(stderr, "Error: nosync requires only one thread.\n");
            exit(EXIT_FAILURE);
        }
       
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {

    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char* file){
    char line[MAX_INPUT_SIZE];

    FILE *fi;

    /* open input file w/ validation */
    if ((fi = fopen(file, "r")) == NULL)
    {
        fprintf(stderr, "Error: not able to open input file\n");
        exit(EXIT_FAILURE);
    }
    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fi)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    /* close input file */
    if (fclose(fi) == EOF)
        fprintf(stderr, "Error: not able to close input file\n");
}

/*
 * Locks the commandMutex and checks for error.
 */
void command_lock(){

    if (syncStrategy == 'n')
        return;

    if (pthread_mutex_lock(&commandMutex) != 0){
        perror("Error: not able to lock commandMutex.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlocks the commandMutex and checks for error.
 */
void command_unlock(){

    if (syncStrategy == 'n')
        return;

    if (pthread_mutex_unlock(&commandMutex) != 0){
        perror("Error: not able to lock commandMutex.\n");
        exit(EXIT_FAILURE);
    }
}

void *applyCommands(){
    while (1){
        

        command_lock();

        if(numberCommands <= 0){
            command_unlock();
            break;
        }

        const char* command = removeCommand();
        command_unlock();

        if (command == NULL){
            break;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                case 'f':
                    printf("Create file: %s\n", name);
                    lock(0);
                    create(name, T_FILE);
                    unlock();
                    break;
                case 'd':
                    printf("Create directory: %s\n", name);
                    lock(0);
                    create(name, T_DIRECTORY);
                    unlock();
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
                }
                    break;

            case 'l': 
                lock(1);
                searchResult = lookup(name);
                unlock();
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;

            case 'd':
                printf("Delete: %s\n", name);
                lock(0);
                delete(name);
                unlock();
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

/*
 * Creates the number of threads given in numberThreads and waits for all to join
 */
void handleThreads(){

    pthread_t tid[numberThreads];

    for (int i=0; i < numberThreads; i++) {
        if (pthread_create (&tid[i], NULL, applyCommands, NULL) != 0){
            fprintf(stderr, "Error: not able do create thread.\n");
            exit(EXIT_FAILURE);                
        }
    }

    /* wait for all threads' execution */
    for (int i=0; i<numberThreads; i++) {
        if (pthread_join (tid[i], NULL) != 0)
            fprintf(stderr, "Error: waiting for thread gone wrong.\n");
    }    
}

/*
 * Unlocks the commandMutex and checks for error.
 * 
 * Inputs:
 *  - begin: time when the execution began
 *  - end: time when the execution ended
 * 
 * Returns: Execution time
 */
void printExecutionTime (struct timeval * begin, struct timeval * end){

    printf ("TecnicoFS completed in %0.4f seconds.\n",
         (double) (end->tv_usec - begin->tv_usec) / 1000000 +
         (double) (end->tv_sec - begin->tv_sec));
}

int main(int argc, char* argv[]) {

    struct timeval begin, end;
    
    validate_arguments(argv);

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput(argv[1]);

    /* attribute current time do variable begin */
    gettimeofday(&begin, NULL);

    /* create and wait for threads to join */
    handleThreads();

    /* open output file w/ validation */
    FILE *fo;
    if ((fo = fopen(argv[2], "w")) == NULL){
        fprintf(stderr, "Error: not able do open output file\n");
        exit(EXIT_FAILURE);
    }

    print_tecnicofs_tree(fo);

    /* closes output file */
    if (fclose(fo) == EOF){
        fprintf(stderr, "Error: not able do close output file.\n");
    }

    /* release allocated memory */
    destroy_fs();
    
    /* destroy commandMutex */
    if(pthread_mutex_destroy(&commandMutex) != 0){
        perror("Error: unable to destroy commandMutex.\n");
        exit(EXIT_FAILURE);   
    }

    /* attribute current time do variable end */
    gettimeofday(&end, NULL);
    printExecutionTime(&begin, &end);

    exit(EXIT_SUCCESS);
}
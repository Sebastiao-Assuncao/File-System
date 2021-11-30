#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include "fs/operations.h"

#define MAX_COMMANDS 15000
#define MAX_INPUT_SIZE 100

pthread_mutex_t commandMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t canAddCommand, canRemoveCommand;

int headQueue = 0;

int numberThreads = 0, insertionPtr = 0, removalPtr = 0, numberCommands = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE], endOfFile = 0;

void validate_arguments (char* argv[]) {

    if (argv[1] == NULL || argv[2] == NULL) /* validate input and output files */
    {
        fprintf(stderr, "Error: input or output file unavailable.\n");
        exit(EXIT_FAILURE);
    }

    if ((numberThreads = atoi(argv[3])) < 1)  /* validate number os threads */
    {
        fprintf(stderr, "Error: number of threads not valid.\n");
        exit(EXIT_FAILURE);
    }  
}

/*
 * Locks the commandMutex and checks for error.
 */
void command_lock(){

    if (pthread_mutex_lock(&commandMutex) != 0){
        perror("Error: not able to lock commandMutex.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlocks the commandMutex and checks for error.
 */
void command_unlock(){

    if (pthread_mutex_unlock(&commandMutex) != 0){
        perror("Error: not able to lock commandMutex.\n");
        exit(EXIT_FAILURE);
    }
}

/*
* Inserts a command on the command array
* Input:
*   - data: data of the command
* Returns:
*   - SUCCESS
*/
int insertCommand(char* data) {
    command_lock();
    
    /* if the command array is full, wait until it isn't */
    while (numberCommands == MAX_COMMANDS)
        if (pthread_cond_wait(&canAddCommand, &commandMutex) != 0) {
            perror("Error: waiting for condition var gone wrong.\n");
            exit (EXIT_FAILURE);
        }
    
    strcpy(inputCommands[insertionPtr], data);
    insertionPtr++;

    /* if insertion pointer is at the end of the array, point to the beggining */
    if(insertionPtr == MAX_COMMANDS) insertionPtr = 0;
    
    numberCommands++;
    if (pthread_cond_signal(&canRemoveCommand) != 0) {
        perror("Error: sending signal to waiting thread gone wrong.\n");
        exit (EXIT_FAILURE);       
    }

    command_unlock();
    
    return SUCCESS;
}

/* 
* Removes a command from the command array
* Returns:
*   - command: the command to be executed
*   - NULL: if there is an error
*/
char* removeCommand() {
    char * command;
    command_lock();

    /* if there are no commands in the command array, wait until there are */
    while(numberCommands == 0 && endOfFile == 0)
        if (pthread_cond_wait(&canRemoveCommand, &commandMutex) != 0) {
            perror("Error: waiting for condition var gone wrong.\n");
            exit (EXIT_FAILURE);          
        }

    /* if there are no more commands and the file is entirely read, return NULL to end the threads execution */
    if (numberCommands == 0)
        if (endOfFile == 1){
            command_unlock();
            return NULL;
        }

    command = inputCommands[removalPtr];
    removalPtr++;

    /* f removal pointer is at the end of the array, point to the beggining */
    if (removalPtr == MAX_COMMANDS) removalPtr = 0;

    numberCommands--;
    if (pthread_cond_signal(&canAddCommand) != 0) {
        perror("EError: sending signal to waiting thread gone wrong.\n");
        exit (EXIT_FAILURE);
    }

    command_unlock();

    return command;
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
        char name2[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line) == SUCCESS)
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line) == SUCCESS)
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line) == SUCCESS)
                    break;
                return;
            case 'm':
                /* re-scan to adjust inputs to move operation requirements */
                numTokens = sscanf(line, "%c %s %s", &token, name, name2);
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line) == SUCCESS)
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }

    /* at the end, toggle 'end of file' flag and broadcast to threads waiting to remove command */
    command_lock();
    endOfFile = 1;
    if (pthread_cond_broadcast(&canRemoveCommand) != 0 ) {
        perror ("Error: not able to free all waiting threads.\n");
        exit (EXIT_FAILURE);
    }
    command_unlock();

    /* close input file */
    if (fclose(fi) == EOF)
        fprintf(stderr, "Error: not able to close input file\n");
}

void *applyCommands(){
    while (1){

        const char* command = removeCommand();

        if (command == NULL){
            break;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        char name2[MAX_INPUT_SIZE];
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
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup_aux(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            case 'm':
                sscanf(command, "%c %s %s", &token, name, name2);
                printf("Move %s to %s\n", name, name2);
                move(name, name2);
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
void initThreads(pthread_t tid[]){

    for (int i=0; i < numberThreads; i++) {
        if (pthread_create (&tid[i], NULL, applyCommands, NULL) != 0){
            perror("Error: not able do create thread.\n");
            exit(EXIT_FAILURE);                
        }
    }
}

/*
* Waits for all the threads to finish their execution
*/
void joinThreads(pthread_t tid[]){
    int returnval;

    //wait for all threads' execution
    for (int i=0; i<numberThreads; i++) {
        if ((returnval = pthread_join (tid[i], NULL)) != 0){
            printf("error joining thread: %s\n", strerror(returnval));
            perror("Error: waiting for thread gone wrong.\n");
        }
    }    
}

/*
 * Prints execution time
 * 
 * Inputs:
 *  - begin: time when the execution began
 *  - end: time when the execution ended
 */
void printExecutionTime (struct timeval * begin, struct timeval * end){

    printf ("TecnicoFS completed in %0.4f seconds.\n",
         (double) (end->tv_usec - begin->tv_usec) / 1000000 +
         (double) (end->tv_sec - begin->tv_sec));
}

int main(int argc, char* argv[]) {

    struct timeval begin, end;

    validate_arguments(argv);

    pthread_t tid[numberThreads];

    /* init filesystem */
    init_fs();

    /* initialize threads to read and execute commands */
    initThreads(tid);

    /* assign current time do variable 'begin' */
    gettimeofday(&begin, NULL);

    /* process input and print tree */
    processInput(argv[1]);
    
    /* wait for threads to join */
    joinThreads(tid);

    /* open output file w/ validation */
    FILE *fo;
    if ((fo = fopen(argv[2], "w")) == NULL){
        fprintf(stderr, "Error: not able do open output file\n");
        exit(EXIT_FAILURE);
    }

    print_tecnicofs_tree(fo);

    /* closes output file */
    if (fclose(fo) == EOF){
        fprintf(stderr, "Error: not able do close output file\n");
    }

    /* release allocated memory */
    destroy_fs();
    
    /* destroy commandMutex */
    if(pthread_mutex_destroy(&commandMutex) != 0){
        perror("Error: unable to destroy commandMutex.\n");
        exit(EXIT_FAILURE);
    }

    
    /* assign current time do variable 'end' */
    gettimeofday(&end, NULL);
    printExecutionTime(&begin, &end);

    exit(EXIT_SUCCESS);
}
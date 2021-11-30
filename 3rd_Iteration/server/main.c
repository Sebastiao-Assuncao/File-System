#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include "fs/operations.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 100
#define OUT_BUFFER_SIZE 8

int numberThreads = 0, sockfd = 0;

/*
 * Validates the arguments given in the shell
 */
void validate_arguments(int argc, char *argv[])
{

    if (argc != 3)
    {
        perror("Error: number of arguments not valid.\n");
        exit(EXIT_FAILURE);
    }

    if ((numberThreads = atoi(argv[1])) < 1)
    { /* validate number of threads */
        perror("Error: number of threads not valid.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Initializes the server's socket.
 * Input:
 *  - socketName: name of the socket
 *  - addr: socket address
 * 
 * Returns: actual length of the 'sockaddr_un' structure
 */
int setSockAddrUn(char *socketName, struct sockaddr_un *addr)
{

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, socketName);

    return SUN_LEN(addr);
}

/*
 * Recieves command from a client.
 * Input:
 *  - command: used to get the command
 *  - client_addr: client socket address
 */
void recieveCommand(char *command, struct sockaddr_un *client_addr)
{
    int c;
    socklen_t addrlen;
    char in_buffer[MAX_INPUT_SIZE];

    addrlen = sizeof(struct sockaddr_un);

    c = recvfrom(sockfd, in_buffer, sizeof(in_buffer) - 1, 0, (struct sockaddr *)client_addr, &addrlen);

    if (c <= 0)
    {
        command = NULL;
        return;
    }

    in_buffer[c] = '\0';

    strcpy(command, in_buffer);
}

/*
 * Sends command result to a client.
 * Input:
 *  - result: the result of the command
 *  - client_addr: client socket address
 */
void sendCommandResult(int result, struct sockaddr_un *client_addr)
{

    char out_buffer[OUT_BUFFER_SIZE];
    int c, addrlen;

    addrlen = sizeof(struct sockaddr_un);

    c = sprintf(out_buffer, "%d", result);

    sendto(sockfd, out_buffer, c + 1, 0, (struct sockaddr *)client_addr, addrlen);
}

void *applyCommands()
{
    char command[MAX_INPUT_SIZE];
    struct sockaddr_un client_addr;

    while (1)
    {

        recieveCommand(command, &client_addr);

        if (command == NULL)
            continue;

        char token;
        char arg1[MAX_INPUT_SIZE];
        char arg2[MAX_INPUT_SIZE];
        int result;
        int numTokens = sscanf(command, "%c %s %s", &token, arg1, arg2);
        if (numTokens < 2)
        {
            fprintf(stderr, "Error: invalid command in Queue\n");
            result = FAIL;
        }

        switch (token)
        {
        case 'c':
            switch (arg2[0])
            {
            case 'f':
                printf("Create file: %s\n", arg1);
                result = create(arg1, T_FILE);
                break;
            case 'd':
                printf("Create directory: %s\n", arg1);
                result = create(arg1, T_DIRECTORY);
                break;
            default:
                perror("Error: invalid create command\n");
                result = FAIL;
                break;
            }
            break;
        case 'l':
            result = lookup_aux(arg1);
            if (result >= 0)
                printf("Search: %s found\n", arg1);
            else
                printf("Search: %s not found\n", arg1);
            break;
        case 'd':
            printf("Delete: %s\n", arg1);
            result = delete (arg1);
            break;
        case 'm':
            printf("Move %s to %s\n", arg1, arg2);
            result = move(arg1, arg2);
            break;
        case 'p':
            result = printFS(arg1);
            break;
        default: /* error */
            perror("Error: invalid command\n");
            result = FAIL;
            break;
        }

        sendCommandResult(result, &client_addr);
    }
    return NULL;
}

/*
 * Creates the number of threads given in numberThreads and waits for all to join
 */
void initThreads(pthread_t tid[])
{

    for (int i = 0; i < numberThreads; i++)
    {
        if (pthread_create(&tid[i], NULL, applyCommands, NULL) != 0)
        {
            perror("Error: not able do create thread.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*
* Waits for all the threads to finish their execution
*/
void joinThreads(pthread_t tid[])
{
    int returnval;

    //wait for all threads' execution
    for (int i = 0; i < numberThreads; i++)
    {
        if ((returnval = pthread_join(tid[i], NULL)) != 0)
        {
            printf("error joining thread: %s\n", strerror(returnval));
            perror("Error: waiting for thread gone wrong.\n");
        }
    }
}

int main(int argc, char *argv[])
{

    struct timeval begin, end;
    pthread_t tid[numberThreads];
    char *socketName;
    struct sockaddr_un server_addr;
    socklen_t addrlen;

    validate_arguments(argc, argv);

    /* create socket without name and check for error */
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    socketName = argv[2];
    unlink(socketName);

    /* bind socket with desired socketName given and check for error */
    addrlen = setSockAddrUn(socketName, &server_addr);
    if (bind(sockfd, (struct sockaddr *)&server_addr, addrlen) < 0)
    {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

    /* init filesystem */
    init_fs();

    /* initialize threads to read and execute commands */
    initThreads(tid);

    /* since the server is shut down with the signal ctrl+C, from this point on nothing is executed */

    /* wait for threads to join */
    joinThreads(tid);

    /* release allocated memory */
    destroy_fs();

    /* close and unlink socket */
    close(sockfd);
    unlink(socketName);

    exit(EXIT_SUCCESS);
}
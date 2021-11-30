#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd; 
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;
char message[MAX_INPUT_SIZE], buffer[BUFFER_SIZE];

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/*
 * Requests server to create a node.
 * Input:
 *  - filename: name of the node to create
 *  - nodeType: type of node (file or directory)
 * Returns: command result
 */
int tfsCreate(char *filename, char nodeType) {

  sprintf(message, "c %s %c", filename, nodeType);

  if (sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client tfsCreate: sendto error\n");
    return FAIL;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client tfsCreate: recvfrom error\n");
    return FAIL;
  } 

  return atoi(buffer);
}

/*
 * Requests server to delete a node.
 * Input:
 *  - path: path of the node to delete
 * Returns: command result
 */
int tfsDelete(char *path) {

  sprintf(message, "d %s", path);

  if (sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client tfsDelete: sendto error\n");
    return FAIL;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client tfsDelete: recvfrom error\n");
    return FAIL;
  } 

  return atoi(buffer);
}

/*
 * Requests server to move a node.
 * Input:
 *  - from: path of the node to move
 *  - to: path to move the node to
 * Returns: command result
 */
int tfsMove(char *from, char *to) {

  sprintf(message, "m %s %s", from, to);

  if (sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client tfsMove: sendto error\n");
    return FAIL;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client tfsMove: recvfrom error\n");
    return FAIL;
  } 

  return atoi(buffer);
}

/*
 * Requests server to lookup a node.
 * Input:
 *  - path: path of the node to lookup
 * Returns: command result
 */
int tfsLookup(char *path) {

  sprintf(message, "l %s", path);

  if (sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client tfsLookup: sendto error\n");
    return FAIL;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client tfsLookup: recvfrom error\n");
    return FAIL;
  } 

  return atoi(buffer);
}

/*
 * Requests server to print the node tree.
 * Input:
 *  - outFilePath: path of the output file
 * Returns: command result
 */
int tfsPrint(char *outFilePath) {

  sprintf(message, "p %s", outFilePath);

  if (sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client tfsPrint: sendto error\n");
    return FAIL;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client tfsPrint: recvfrom error\n");
    return FAIL;
  } 

  return atoi(buffer);
}

/*
 * Mount the socket.
 * Input:
 *  - sockPath: path of the server's socket
 * Returns: SUCCESS/FAIL
 */
int tfsMount(char * sockPath) {

  createClientSocket();

  if((servlen = setSockAddrUn(sockPath, &serv_addr)) == 0)
    return FAIL;

  return SUCCESS;
}

/*
 * Unmount the socket.
 */
void tfsUnmount() {

  if(close(sockfd) != 0)
    perror("Error: client close unsuccesful\n");

  if(unlink(client_addr.sun_path) != 0)
    perror("Error: client socket unlink unsuccesful\n");
}

/*
 * Creates the client's socket
 */
void createClientSocket() {

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket\n");
    exit(EXIT_FAILURE);
  }

  bzero((char *)&client_addr, sizeof(struct sockaddr_un));
  client_addr.sun_family = AF_UNIX;

  strcpy(client_addr.sun_path, "/tmp/clientSocket-XXXXXX");

  if(mkstemp(client_addr.sun_path) == -1){
    perror("Error: mkstemp unsuccesful\n");
    exit(EXIT_FAILURE);
  }

  if(unlink(client_addr.sun_path) != 0){
    perror("Error: client socket pre-unlink unsuccesful\n");
    exit(EXIT_FAILURE);
  }

  clilen = sizeof(client_addr.sun_family) + strlen(client_addr.sun_path);

  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error\n");
    exit(EXIT_FAILURE);
  }  
}

#ifndef API_H
#define API_H

#define SUCCESS 0
#define FAIL -1

#define BUFFER_SIZE 9

#include "tecnicofs-api-constants.h"

int tfsCreate(char *path, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsMount(char* serverName);
void tfsUnmount();
int tfsPrint(char *outFilePath);
void createClientSocket();

#endif /* CLIENT_H */

#ifndef FS_H
#define FS_H
#include "state.h"

#define MAXIMUM_LOCKED_INODES 100

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name, int count, int locked_inumbers[], char caller, int previously_locked_inumbers[]);
int lookup_aux (char *name);
int move(char * old_path, char * new_path);
void print_tecnicofs_tree(FILE *fp);


#endif /* FS_H */

#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY, -1);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}

	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {


		if (dirEntries[i].inumber != FREE_INODE) {
	
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	int inumber;
	
	if (entries == NULL) {
		return FAIL;
	}

	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
			inumber = entries[i].inumber;
            return inumber;
        }
    }
	return FAIL;
}

/*
* Returns the number of directories existent in the path
* Input:
*	- name: path of node
* Returns: number of directories
*/
int count_number_paths(char name[]) {

	int count = 1, i;

	for (i=0; name[i] != '\0'; i++ ) {
		if (name[i] == '/')
			count++;
	}

	if (name[i-1] == '/')
		count--;

	return count;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	int locked_inumbers[MAXIMUM_LOCKED_INODES];
	int count;
	type pType;
	union Data pdata;

	/* initialize locked inodes array */
	for (int n=0; n< MAXIMUM_LOCKED_INODES; n++) {
		locked_inumbers[n] = -1;
	}

	strcpy(name_copy, name);

	count = count_number_paths(name_copy);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	
	parent_inumber = lookup(parent_name, count, locked_inumbers, CREATE, NULL);

	/* if parent doesn't exist, return FAIL */
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	/* if parent isn't a directory, return FAIL */
	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);

		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* if inode already exists, return FAIL */
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType, parent_inumber);

	/* if there is an error creating new inode, return FAIL */
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* lock new inode and add it to the locked inodes array */
	lock(child_inumber, READ);
	locked_inumbers[count] = child_inumber;

	/* add inode to parent directory and returns FAIL if it isn't successful */
	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* unlocks all the inodes that were locked during lookup and the new inode itself */
	unlock_array(locked_inumbers);

	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	int locked_inumbers[MAXIMUM_LOCKED_INODES];
	int count;

	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	/* initialize locked inodes array */
	for (int n=0; n< MAXIMUM_LOCKED_INODES; n++) {
		locked_inumbers[n] = -1;
	}

	strcpy(name_copy, name);

	count = count_number_paths(name_copy);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, count, locked_inumbers, DELETE, NULL);

	/* if parent doesn't exist, return FAIL */
	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	/* if parent isn't a directory, return FAIL */
	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	/* if child doesn't exist, return FAIL */
	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* lock inode to delete and add it to locked inodes array */
	lock(child_inumber, READ);
	locked_inumbers[count] = child_inumber;

	inode_get(child_inumber, &cType, &cdata);

	/* if inode to delete is a non-empty directory, return FAIL */
	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* delete the inode, return FAIL if not successful */
	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		unlock_array(locked_inumbers);
		return FAIL;
	}

	/* unlocks all the inodes that were locked during lookup and the new inode itself */
	unlock_array (locked_inumbers);

	return SUCCESS;
}

/*
* Auxiliar function used in command 'l' from main to lookup.
* Input:
*	- name: path of node
* Returns:
*	inumber: identifier of the i-node, if found
*	FAIL: otherwise
*/
int lookup_aux (char *name) {
	int locked_inumbers[MAXIMUM_LOCKED_INODES];
	int count = count_number_paths(name);

	/* initialize locked inodes array */
	for (int n=0; n< MAXIMUM_LOCKED_INODES; n++) {
		locked_inumbers[n] = -1;
	}

	int current_inumber = lookup (name, count, locked_inumbers, LOOKUP, NULL);

	unlock_array(locked_inumbers);

	return current_inumber;
}

/*
* Checks if an inode is already locked, given a locked inumbers array
* Input:
* 	- current_inumber: identifier of the inode
*	- previously_locked_inumbers: array with the inumbers of the locked inodes
* Returns:
*	SUCCESS: if inode is already locked 
*	FAIL: otherwise
*/
int check_lock_state(int current_inumber, int previously_locked_inumbers[]) {
	
	for (int i=0; previously_locked_inumbers[i] != -1 ; i++) {
		if (previously_locked_inumbers[i] == current_inumber)
			return SUCCESS;
	}

	return FAIL;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 *  - count: number of directories existent in the path
 *  - locked_inumbers: array to save the inumbers of inodes that are locked while path is being covered
 *  - caller: flag to know if the last directory is locked for READ or for WRITE
 *  - previously_locked_inumbers: array only used when the caller is MOVE to check if an inode is already locked
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int count, int locked_inumbers[], char caller, int previously_locked_inumbers[]) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;
	int i = 0;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* if path name is the root itself, lock it according to the caller and add it to the locked inodes array*/
	if (count == 1) { 
		lock(current_inumber, caller);
		locked_inumbers[i] = current_inumber;
		return current_inumber;
	}

	/* else, read lock the root and add it to the locked inodes array */
	lock(current_inumber, READ);
	locked_inumbers[i++] = current_inumber;

	inode_get(current_inumber, &nType, &data);
	
	char *path = strtok_r(full_path, delim, &saveptr);

		/* search for all sub nodes */
		while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
			
			/* if lookup was invoked by move function, only locks the inodes that were not locked yet*/
			if (caller == MOVE) {
				/* current_inumber already locked -> continue */
				if (check_lock_state(current_inumber, previously_locked_inumbers) == SUCCESS){
					inode_get(current_inumber, &nType, &data);	
					path = strtok_r(NULL, delim, &saveptr); 
					continue;
				}
			}
			
			/* if current_inumber is the last inode on the path, lock it according to the caller */
			if (i == count-1) {
				lock(current_inumber, caller);
				locked_inumbers[i] = current_inumber;
				return current_inumber;
			}

			/* else, lock the inode for read */
			lock(current_inumber, READ);
			locked_inumbers[i] = current_inumber;
			i++;

			inode_get(current_inumber, &nType, &data);	
			path = strtok_r(NULL, delim, &saveptr); 
		}

	return current_inumber;
}

/*
* Given the origin path from the move() function, verifies if it is a valid path according to the rules of move().
* Input:
*	- old_parent_inumber: inumber of the parent directory we want to move the inode from
* 	- old_parent_name: name of the parent director we want to move the inode from
*	- old_child_name: old name of the inode we want to move
*	- old_path_count: number of directories on the old path
*	- locked_origin_inumbers: array containing the inumbers of the locked inodes in the origin path
*	- locked_final_inumbers: array containing the inumbers of the locked inodes in the final path
*	- inumber: inumber of the inode we want to move
* Returns:
*	- SUCCESS or FAIL
*/
int validate_origin_path(int * old_parent_inumber, char * old_parent_name, char * old_child_name, int old_path_count, 
  int locked_origin_inumbers[], int locked_final_inumbers[], int * inumber){

	/* if old_path's parent doesn't exist, return FAIL */
	if ((*old_parent_inumber = lookup(old_parent_name, old_path_count, locked_origin_inumbers, MOVE, locked_final_inumbers)) == FAIL) {
		printf("Invalid old_path parent: %s\n", old_parent_name);
		unlock_array(locked_origin_inumbers);
		unlock_array(locked_final_inumbers);
		return FAIL;
	}

	
	type nType;
	union Data data;
	
	inode_get(*old_parent_inumber, &nType, &data);

	/* if old child doesn't exist in old parent directory, return FAIL */
	if((*inumber = lookup_sub_node(old_child_name, data.dirEntries)) == FAIL){
		printf("Inode to move doesn't exist: %s\n", old_child_name);
		unlock_array(locked_origin_inumbers);
		unlock_array(locked_final_inumbers);
		return FAIL;
	}

	/* read lock inode to me moved */
	lock(*inumber, READ);

	locked_origin_inumbers[old_path_count] = *inumber;

	return SUCCESS;

}

/*
* Given the final path from the move() function, verifies if it is suitable to recieve the inode we want to move.
* Input:
*	- new_parent_inumber: inumber of the parent directory we want to move the inode to
* 	- new_parent_name: name of the parent director we want to move the inode to
*	- new_child_name: new name of the inode we want to move
*	- new_path: input given by the user as the new path for the inode
*	- new_path_count: number of directories on the new_path
*	- locked_final_inumbers: array containing the inumbers of the locked inodes in the final path
*	- locked_origin_inumbers: array containing the inumbers of the locked inodes in the origin path
* Returns:
*	- SUCCESS or FAIL
*/
int validate_final_path(int * new_parent_inumber, char * new_parent_name, char * new_child_name, char * new_path, 
  int new_path_count, int locked_final_inumbers[], int locked_origin_inumbers[]){
	
	/* if new path's parent doesn't exist, return FAIL */
	if ((*new_parent_inumber = lookup(new_parent_name, new_path_count, locked_final_inumbers, MOVE, locked_origin_inumbers)) == FAIL){
		printf("New path is not valid: %s\n", new_path);
		unlock_array(locked_origin_inumbers);
		unlock_array(locked_final_inumbers);
		return FAIL;
	}

	type nType;
	union Data data;

	inode_get(*new_parent_inumber, &nType, &data);

	/* if the new_path already exists, return FAIL */
	if(lookup_sub_node(new_child_name, data.dirEntries) != FAIL){
		printf("New path already exists\n");
		unlock_array(locked_origin_inumbers);
		unlock_array(locked_final_inumbers);
		return FAIL;
	}

	return SUCCESS;
}

/*
* Moves an inode to a different path. If it is a directory, takes all childs with it
* Input:
*	- old_path: path of the inode we want to move
*	- new_path: new path we want the inode to move to
* Returns:
*	- SUCCESS or FAIL
*/
int move(char * old_path, char * new_path){

	int inumber, new_parent_inumber, old_parent_inumber;
	/* arrays to save locked inodes when covering the old and new paths */
	int locked_origin_inumbers[MAXIMUM_LOCKED_INODES], locked_final_inumbers[MAXIMUM_LOCKED_INODES];
	char *new_parent_name, *new_child_name, *old_parent_name, *old_child_name;
	char old_name_copy[MAX_FILE_NAME], new_name_copy[MAX_FILE_NAME];
	int old_path_count = count_number_paths(old_path), new_path_count = count_number_paths(new_path);

	/* initialize locked inodes arrays */
	for (int n=0; n < MAXIMUM_LOCKED_INODES; n++) {
		locked_origin_inumbers[n] = -1;
		locked_final_inumbers[n] = -1;
	}

	strcpy(old_name_copy, new_path);
	split_parent_child_from_path(old_name_copy, &new_parent_name, &new_child_name);

	strcpy(new_name_copy, old_path);
	split_parent_child_from_path(new_name_copy, &old_parent_name, &old_child_name);

	/* if new parent's name is the same as the inode to be moved, return FAIL */
	if (strcmp(old_path, new_parent_name) == 0) {
		printf("failed to move %s to %s. can't move directory inside itself.\n", old_path, new_path);
		return FAIL;
	}

	/* used to impose an order of search to prevent deadlocks */
	if(old_path_count < new_path_count || (old_path_count == new_path_count && strcmp(old_parent_name, new_parent_name) <= 0))
		if(validate_origin_path(&old_parent_inumber, old_parent_name, old_child_name, old_path_count,
	  	  locked_origin_inumbers, locked_final_inumbers, &inumber) == FAIL) {
			return FAIL;
			}

	if(validate_final_path(&new_parent_inumber, new_parent_name, new_child_name, new_path, new_path_count, 
	  locked_final_inumbers, locked_origin_inumbers) == FAIL)
		return FAIL;
	
	/* used to impose an order of search to prevent deadlocks */
	if(new_path_count < old_path_count || (old_path_count == new_path_count && strcmp(old_parent_name, new_parent_name) > 0))
		if(validate_origin_path(&old_parent_inumber, old_parent_name, old_child_name, old_path_count,
	  	  locked_origin_inumbers, locked_final_inumbers, &inumber) == FAIL)
			return FAIL;


	/* remove the inode we want to move from its old parent directory. if not successful, return FAIL */
	if (dir_reset_entry(old_parent_inumber, inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       old_child_name, old_parent_name);
		unlock_array(locked_origin_inumbers);
		unlock_array(locked_final_inumbers);
		return FAIL;
	}	

	/* add the inode we want to move to the new parent directory. if not successful, return FAIL */
	if (dir_add_entry(new_parent_inumber, inumber, new_child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       new_child_name, new_parent_name);
		unlock_array(locked_origin_inumbers);
		unlock_array(locked_final_inumbers);
		return FAIL;
	}
	
	/* unlock all inodes that were locked in lookup function call and were saved in both arrays */
	unlock_array(locked_origin_inumbers);
	unlock_array(locked_final_inumbers);
	
	return SUCCESS;
} 

/*
 * Prints the node tree do an output file
 * Input:
 *  - outFile: path of the output file
 * Returns: SUCCESS/FAIL
 */
int printFS(char *outFile){

	/* open output file w/ validation */
    FILE *fo;
    if ((fo = fopen(outFile, "w")) == NULL){
        fprintf(stderr, "Error: not able do open output file\n");
        return FAIL;
    }

	lock(0, WRITE);

	print_tecnicofs_tree(fo);

	unlock(0);

    /* closes output file */
    if (fclose(fo) == EOF){
        fprintf(stderr, "Error: not able do close output file\n");
    }

	return SUCCESS;
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}

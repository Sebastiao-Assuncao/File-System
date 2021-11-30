# SO2021

## Goal

Develop a user-level File System that keeps its contents in primary memory - TecnicoFS.

## Usage

The server program must recieve the following four arguments in the command line:

***server_name*** *inputfile outputfile numthreads synchstrategy*

## Examples

c /a f -> creates file a on root
c /b d -> creates directory b on root
c /c f -> creates file c on root
l /a -> searches for i-node associated to given path
d /a -> deletes i-node a
m /c /b/c -> moves i-node /c from root to directory /b

## Development

Project divided into 3 iterations:


### 1st Iteration

#### 1. Load entry file

Execute call sequences loaded from the *inputfile*. Possible commands:

##### Command 'c':

- Arguments: *name type*
Adds a new entry to the directory, whoose name and type are indicated in the arguments. The type can be either 'd' (directory) or 'f' (file).

##### Command 'l':

- Arguments: *name*
Search the file system for a file or directory with the given name.

##### Command 'd':

-Arguments: *name*
Delete the file or directory with the given name from the file system.

#### 2. Server Parallelization

##### 2.1 Thread Pool

The server must execute operations in parallel, using a thread pool with the number of threads specified by *numthreads*. The pool is only created after the *inputfile* has been totally loaded.

##### 2.2 Global Lock Synchronization

The threads' synchronization must be kept by using one of 2 strategies:

###### i) 

Global Mutex (pthread_mutex)

###### ii) 

Using a read-write lock instead of a global mutex (pthread_rw_lock)


The strategy to be used is specified using *synchstrategy* (mutex, rwlock or nosync).

#### 3. Server Termination

When all operations have been executed, the server must print the execution time and export the final contents of the file system to the *outputfile*.


### 2nd Iteration

#### 1. Fine Synchronization Of Inodes

In this iteration, the previous synchronization strategies are abandoned and each inode has its own lock.

#### 2. Incremental execution of commands

In this iteration, the *inputfile* load and the execution of commands are executed in parallel.

#### 3. New Operation: Move File Or Directory

##### Command 'm':

- Arguments: *pathname1 pathname2*
Moves file of directory with name *pathname1* into the location specified in *pathname2*.


### 3rd Iteration

#### 1. Communication With Client Processes

The server no longer loads calls from a file. Instead, used a *Unix* socket of *datagram* type through which it recieves (and answers to) requests made from "client" processes.

The executable now recieves the following arguments:

***server_name*** *numthreads socketname*

#### 2. New Operation 'p'

##### Command 'p':

- Arguments: *outputfile*
Prints the current contents of the file system on the *outputfile*.
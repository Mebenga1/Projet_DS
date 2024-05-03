#ifndef SOCKET_THREAD_H

#define SOCKET_THREAD_H

#include <arpa/inet.h> // Header for IP address manipulation
#include <netinet/in.h> // Header for Internet protocol family
#include <pthread.h> // Header for POSIX threads
#include <stdbool.h> // Header for boolean types
#include <stdio.h> // Standard I/O functions
#include <stdlib.h> // Standard library functions
#include <string.h> // String manipulation functions
#include <sys/socket.h> // Header for socket operations
#include <unistd.h> // Header for POSIX operating system API
#include <fcntl.h> // Header for file control operations
#include <sys/stat.h> // Header for file status functions
#include <sys/types.h> // Header for system data types
#include <asm-generic/socket.h> // Generic socket definitions
#include <time.h> // Header for time functions
#include <errno.h>
#include <syslog.h>

extern pthread_mutex_t mutex; // declare a mutex

// Forward declaration of the File_Info_server structure
struct File_Info_server;

// Typedef for easier use of the File_Info_server structure
typedef struct File_Info_server *file;

// Function to delete all lines associated with an IP address before rewriting to avoid duplicates in the file
void remove_client_hold_data(char* ip_supprimer, char* nom_fichier);

// Function prototype for receive_request function
void *receive_request(void *args);

// Print the datas of client
void Write_File_Info(file client_data, int data_count,char* ip_client,int port);

// Function that retrieves information from files and sends to the client
void extractFileInfo(int new_socket_fd);

#endif

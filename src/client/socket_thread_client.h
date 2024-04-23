#ifndef SOCKET_THREAD_H
#define SOCKET_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h> // Provides definitions of structures and functions for creating and managing sockets
#include <netinet/in.h> // Provides definitions of structures and constants for Internet addresses and ports
#include <arpa/inet.h> // Provides functions for converting between IP address and port number representations as strings and binary representations
#include <syslog.h> // Adds the header for logging with rsyslog
#include <sys/types.h>
#include <sys/stat.h> // Provides definitions of structures and functions for getting file information
#include <dirent.h> // Provides definitions of structures and functions for manipulating directories and file entries
#include <sys/inotify.h> // Provides definitions of structures and functions for using inotify, a file monitoring mechanism.

struct File_Info;
typedef struct File_Info File; // Defines a structure for file information

void Send_File_Info(int port, char* Ip_server,char *path_directory); /* Function that retrieves file names and sizes from the directory and send to the server*/

void Monitoring_directory(int port, char* Ip_server,char *path_directory); /* Function for monitoring the directory for files */

#endif

#include "socket_thread_client.h"

struct File_Info {
    char filename[1024];
    long filesize;
};

void Send_File_Info(int port, char* Ip_server,char *path_directory) {

    int socket_fd = 0; // File descriptor for the new socket
    struct sockaddr_in server_address; // Structure for the server's IP address and connection port

    // Creates a socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // Checks if socket creation fails
        perror("Socket creation failed");
        exit(-1);
    } // Added missing closing parenthesis

    // Initializes values in the server_address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // Sets the port number by converting from string to integer

    // Converts the server's IP address from string to binary format
    if (inet_pton(AF_INET, Ip_server, &server_address.sin_addr) <= 0) {
        perror("Invalid or unsupported IP address");
        exit(-2);
    }

    // Connects to the server
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(-3);
    }


    DIR *directory; // Pointer to the directory stream
    struct dirent *entry; // Pointer to directory entries
    struct stat file_stat; // File information structure
    File file_info[1024]; // Instance of the File_Info structure
    int client_count = 0;

    // Opens the specified directory
    if ((directory = opendir(path_directory)) == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", path_directory); // Prints an error message
        exit(-1); // Exits the program with an error code
    }

    // Reads directory entries
    while ((entry = readdir(directory)) != NULL) {
        // Excludes the directories "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *full_path;
        full_path = malloc(1024); // Allocates memory for the full path
        snprintf(full_path, 1024, "%s/%s", path_directory, entry->d_name); // Constructs the full path

        // Retrieves file information
        if (stat(full_path, &file_stat) == 0) {
            // Assigns file information to the file_info structure
            strcpy(file_info[client_count].filename, entry->d_name); // Copies the file name
            file_info[client_count].filesize = file_stat.st_size; // Assigns the file size
            client_count++;
            free(full_path);
        } else {
            fprintf(stderr, "Error getting information of file: %s\n", entry->d_name);
            continue; // Skips to the next entry
        }
    }

    for (int i = 0; i < client_count; i++) {
        fprintf(stdout, " %s, %ld bytes\n", file_info[i].filename, file_info[i].filesize); // Prints file information
    }
    closedir(directory); // Closes the directory stream

    // Sends file_info via the socket
    if(send(socket_fd, file_info, client_count * sizeof(struct File_Info), 0) > 0){
        fprintf(stdout, " \nDonnées (%d) envoyé avec succès !!!!! \n", client_count);
    }
    else{
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
     // Closes the socket
    close(socket_fd);

}

/*********************** Monitoring of the directory ******************************************/

void Monitoring_directory(int port, char* Ip_server,char *path_directory) {

    int inotify_fd = inotify_init(); // Initializes inotify

    if (inotify_fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int watch_fd = inotify_add_watch(inotify_fd, path_directory, IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO); // Adds a watch to the specified directory
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    int send_flag = 0; // Flag to track if Send_File_Info has been called

    while (1) { // Infinite loop for continuous monitoring
        char* array_event = malloc(1024 * (sizeof(struct inotify_event) + 16)); // Allocates memory for inotify events
        ssize_t num_bytes; // Number of bytes read from inotify

        num_bytes = read(inotify_fd, array_event, 1024 * (sizeof(struct inotify_event) + 16)); // Reads inotify events
        if (num_bytes < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        int i = 0;
        while (i < num_bytes) {
            struct inotify_event *event = (struct inotify_event *)&array_event[i]; // Pointer to the current inotify event
            if (event->len > 0) {
                if (event->mask & (IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO)) {
                    if (!send_flag) {
                        Send_File_Info(port, Ip_server,path_directory); // Calls a function to send file information to the server
                        printf("\nThe directory was modified.\n\n"); // Prints a message about directory modification
                        send_flag = 1; // Set the flag to indicate that Send_File_Info has been called
                    }

                }
            }
            i += sizeof(struct inotify_event) + event->len; // Moves to the next event
        }
        free(array_event); // Frees allocated memory
        // Reset the send_flag if needed
        send_flag = 0;
    }
    inotify_rm_watch(inotify_fd, watch_fd); // Removes the watch
    close(inotify_fd); // Closes inotify
}

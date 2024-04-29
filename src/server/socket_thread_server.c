#include "socket_thread_server.h"

// Define a structure to hold file information on the server side
struct File_Info_server {
    char filename[1024];
    long filesize;
};

// Function to remove data associated with a specific client IP
void remove_client_hold_data(char* remove_ip, char* file_name) {
    FILE *original_file = fopen(file_name, "r"); // Open the original file
    FILE *temp = fopen("temp.txt", "w"); // Open a temporary file
    char line[256]; // Buffer to hold each line of the file
    int remove_id = 0; // Flag to indicate whether to remove the line

    // Loop through each line of the original file
    while (fgets(line, sizeof(line), original_file)) {
        char ip[16]; // Buffer to hold the IP address
        sscanf(line, "%15s", ip); // Read the IP address from the line

        // If the IP address matches the one to be removed
        if (strstr(line, remove_ip)) {
            remove_id = 1; // Set the flag to remove the line
        } else if (line[0] != '\t' && remove_id) { // If a new IP address is found
            remove_id = 0; // Reset the flag
        }

        // If the IP address does not match the one to be removed
        if (!remove_id) {
            fputs(line, temp); // Write the line to the temporary file
        }
    }

    fclose(original_file);
    fclose(temp);

    // Replace the original file with the temporary file
    remove(file_name);
    rename("temp.txt", file_name);
}

// Function to handle incoming requests from clients
void *receive_request(void *args) {
    int new_socket_fd = *(int *)args; // Get the new socket file descriptor
    struct sockaddr_in client_addr; // Structure to hold client address information
    file client_files[1024]; // Array to hold the client's files
    int file_count = 0; // Counter for the number of files

    socklen_t client_addr_size = sizeof(client_addr); // Get the size of the client address structure
    getpeername(new_socket_fd, (struct sockaddr *)&client_addr, &client_addr_size); // Get the client's address

    char client_ip[INET_ADDRSTRLEN]; // Buffer to hold the client's IP address
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN); // Convert the client's IP address to a string

    // Receive the entire array from the client at once
    ssize_t bytes_received = recv(new_socket_fd, client_files, sizeof(client_files), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
           syslog(LOG_INFO, "Client %s disconnected.\n", client_ip);
        } else {
           syslog(LOG_ERR, "Error receiving data from client %s\n", client_ip);
        }
    } else if (bytes_received > 0) {
        // Calculate the number of files received
        file_count = bytes_received / sizeof(struct File_Info_server);
        printf(" Data (%d) received successfully\n", file_count);
        syslog(LOG_INFO, "Data (%d) received successfully\n", file_count);
    }

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Initialize a mutex
    pthread_mutex_lock(&mutex); // Lock the mutex

    remove_client_hold_data(client_ip,"./file/client_data.txt"); // Remove the client's old data

    FILE *log_file = fopen("./file/client_data.txt", "a"); // Open the log file
    if (log_file == NULL) {
         syslog(LOG_ERR, "Error opening file ./file/client_data.txt");
        return NULL;
    }
    fprintf(log_file, "%s\t%d\n", client_ip, ntohs(client_addr.sin_port)); // Write the client's IP address and port to the log file
    fclose(log_file); // Close the log file

    log_file = fopen("./file/client_data.txt", "a"); // Reopen the log file
    if (log_file == NULL) {
        syslog(LOG_ERR, "Error opening file ./file/client_data.txt ");
        return NULL;
    }
    // Loop through each file received from the client
    for(int i = 0; i < file_count; i++) {
        fprintf(log_file, "\t%s\t%ld bytes\n", client_files[i].filename, client_files[i].filesize); // Write the file name and size to the log file
        fprintf(stdout, " %s\t%ld bytes\n", client_files[i].filename, client_files[i].filesize); // Print the file name and size to the console
    }
    fclose(log_file); // Close the log file

    pthread_mutex_unlock(&mutex); // Unlock the mutex
    pthread_mutex_destroy(&mutex); // Destroy the mutex

    pthread_exit(NULL); // Exit the thread
}

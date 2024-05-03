#include "socket_thread_server.h"

// Define a structure to hold file information on the server side
struct File_Info_server {
    char filename[1024];
    long filesize;
};

pthread_mutex_t mutex; // declares the mutex

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

// Function to write client information to the file
void Write_File_Info(struct File_Info_server* client_data, int data_count, char* ip_client, int port) {
    pthread_mutex_lock(&mutex);

    remove_client_hold_data(ip_client, "./file/client_data.txt"); // Remove the client's old data

    FILE *log_file = fopen("./file/client_data.txt", "a"); // Open the log file
    if (log_file == NULL) {
        syslog(LOG_ERR, "Error opening file ./file/client_data.txt");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "%s\t%d\n", ip_client, port); // Write the client's IP address and port to the log file

    // Loop through each file received from the client
    for (int i = 0; i < data_count; i++) {
        fprintf(log_file, "\t\"%s\"\t%ld bytes\n", client_data[i].filename, client_data[i].filesize); // Write the file name and size to the log file
        //fprintf(stdout, " %s\t%ld bytes\n", client_data[i].filename, client_data[i].filesize); // Print the file name and size to the console
    }

    fclose(log_file); // Close the log file
   // free(client_data);

    pthread_mutex_unlock(&mutex);
}


// Function to handle incoming requests from clients
void *receive_request(void *args) {
    int new_socket_fd = *(int *)args; // Get the new socket file descriptor
    struct sockaddr_in client_addr; // Structure to hold client address information
    file client_files = malloc(1024 * sizeof(struct File_Info_server)); // Array to hold the client's files
    if (client_files == NULL) {
        syslog(LOG_ERR, "Error allocating memory");
        exit(-1);
    }

    int file_count = 0; // Counter for the number of files

    socklen_t client_addr_size = sizeof(client_addr); // Get the size of the client address structure
    getpeername(new_socket_fd, (struct sockaddr *)&client_addr, &client_addr_size); // Get the client's address

    char client_ip[INET_ADDRSTRLEN]; // Buffer to hold the client's IP address
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN); // Convert the client's IP address to a strin
    int port = ntohs(client_addr.sin_port);

    // Receive the entire array from the client at once
ssize_t bytes_request;
int id_request;

do {
    bytes_request = recv(new_socket_fd, &id_request, sizeof(id_request), 0);
    if (bytes_request <= 0) {
        if (bytes_request == 0) {
            syslog(LOG_INFO, "Client %s disconnected.\n", client_ip);
        } else {
            syslog(LOG_ERR, "Error receiving data from client %s\n", client_ip);
        }
        break;
    }

    if (id_request == 1) {
        int bytes_received = recv(new_socket_fd, client_files, 1024 * sizeof(struct File_Info_server), 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                syslog(LOG_INFO, "Client %s disconnected.\n", client_ip);
            } else {
                syslog(LOG_ERR, "Error receiving data from client %s\n", client_ip);
            }
            break;
        }

        // Calculate the number of files received
        file_count = bytes_received / sizeof(struct File_Info_server);
        printf("Data (%d) received successfully\n", file_count);
        syslog(LOG_INFO, "Data (%d) received successfully\n", file_count);

        Write_File_Info(client_files, file_count, client_ip, port);

        // Clear the client_files array for the next iteration
        memset(client_files, 0, 1024 * sizeof(struct File_Info_server));
    } else if (id_request == 2) {
        extractFileInfo(new_socket_fd);
    }
} while (id_request != 0);

    close(new_socket_fd);
    free(client_files);
    pthread_exit(NULL); // Exit the thread
}


// Function that retrieves information from files and sends to the client
void extractFileInfo(int new_socket_fd) {

    pthread_mutex_lock(&mutex);

    // Open the log file for reading
    FILE* log_file = fopen("./file/client_data.txt", "r");
    if (log_file == NULL) {
        syslog(LOG_ERR,"Error opening the file client_data.txt");
        exit(-1);
    }

    // Allocate memory for the file_info array
    struct File_Info_server* file_info = malloc(1024 * sizeof(struct File_Info_server));
    if (file_info == NULL) {
        syslog(LOG_ERR, "Error allocating memory");
        exit(EXIT_FAILURE);
    }

    int count_file = 0; // Counter for the number of files

    char line[256];
    while (fgets(line, sizeof(line), log_file)) {
        if (line[0] == '\t') {
            char* name = malloc(256 * sizeof(char)); // Allocate memory for file name
            char* size = malloc(256 * sizeof(char)); // Allocate memory for file size

            if (sscanf(line, "\t\"%[^\"]\"\t%[0-9] bytes", name, size) == 2) {
                long fileSize = atoi(size); // Convert size to a long integer

                // Copy file name and size to the file_info array
                strcpy(file_info[count_file].filename, name);
                file_info[count_file].filesize = fileSize;
            }
            count_file++;

            // Free the allocated memory for name and size
            free(name);
            free(size);
        }

    }
    // Sends file_info  to client via the socket
    if(send(new_socket_fd, file_info, count_file * sizeof(struct File_Info_server), 0) > 0){
        fprintf(stdout, " \nData (%d) sent successfully !!!!! \n", count_file);
        syslog(LOG_INFO, "Data (%d) sent successfully!", count_file);
    }
    else{
        syslog(LOG_ERR,"Send failed");
        exit(EXIT_FAILURE);
    }
    // Close the log file
    fclose(log_file);
    // Free the allocated memory for file_info array
    free(file_info);
    pthread_mutex_unlock(&mutex);
}

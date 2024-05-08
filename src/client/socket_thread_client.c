#include "socket_thread_client.h"

/***** Defines a structure for file information *******/
struct File_Info {
    char filename[1024];
    long filesize;
};


/********** function that retrieves the names and sizes of files in a directory *******/
File Read_directory(char *path_directory){

    DIR *directory; // Pointer to the directory stream
    struct dirent *entry; // Pointer to directory entries
    struct stat file_stat; // File information structure
    File file_infos = malloc(1024 * sizeof(struct File_Info)); // Instance of the File_Info structure
    if (file_infos == NULL) {
        syslog(LOG_ERR,"Error allocating memory");
        exit(EXIT_FAILURE);
    }

    int client_count = 0; // files counter

    // Opens the specified directory
    if ((directory = opendir(path_directory)) == NULL) {
        syslog(LOG_ERR, "Error opening directory: %s", path_directory); // Prints an error message
        exit(-1); // Exits the program with an error code
    }

    // Reads directory entries
    while ((entry = readdir(directory)) != NULL) {
        // Excludes the directories "." and ".."
       /* if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }*/
        if (entry->d_type == DT_REG) {
            char *full_path;
            full_path = malloc(1024); // Allocates memory for the full path
            snprintf(full_path, 1024, "%s/%s", path_directory, entry->d_name); // Constructs the full path

            // Retrieves file information
            if (stat(full_path, &file_stat) == 0) {
                // Assigns file information to the file_info structure
                strcpy(file_infos[client_count].filename, entry->d_name); // Copies the file name
                file_infos[client_count].filesize = file_stat.st_size; // Assigns the file size
                client_count++;
                free(full_path);
            } else {
                syslog(LOG_ERR, "Error getting information of file: %s", entry->d_name);
                continue; // Skips to the next entry
            }
        }
    }
    closedir(directory); // Closes the directory stream

    return file_infos;
}


/******* function that calculates the number of files in the structure to send to the server ******/
int Get_File_Count(File file_info) {
    int count = 0;
    while (file_info[count].filename[0] != '\0') {
        count++;
    }
    return count;
}


/*************** function of   Send of files informations to server *****************/
void* Send_File_Info(void *args) {

    int socket_fd = *(int *)args;
    File file_info = Read_directory("./data/");
    int file_count = Get_File_Count(file_info);

    char *id_send="data";

    // Send the file request to the server
    if(send(socket_fd, id_send, strlen(id_send),0) > 0){
        syslog(LOG_INFO, "sends successful data request");
    }
    else{
        syslog(LOG_ERR, "Failed to send filed  data request");
    }

    // Sends file_info via the socket
    if(send(socket_fd, file_info, file_count * sizeof(struct File_Info), 0) > 0){
        //fprintf(stdout, " \nData (%d) sent successfully !!!!! \n", file_count);
        syslog(LOG_INFO, "Data (%d) sent successfully!", file_count);

    }
    else{
        syslog(LOG_ERR,"Sends failed, data");
        exit(EXIT_FAILURE);
    }

/*********************** Monitoring of the directory ******************************************/
    syslog(LOG_INFO, "Monitoring directory");

    while (1) {
        sleep(3);

        File file_info_update = Read_directory("./data/");
        int file_count_update = Get_File_Count(file_info_update);

        if (memcmp(file_info, file_info_update, file_count * sizeof(struct File_Info)) != 0) {

            // Send file update request to the server
            if (send(socket_fd, id_send, strlen(id_send), 0) > 0) {
                syslog(LOG_INFO, "File update request sent successfully");
            } else {
                syslog(LOG_ERR, "Failed to send file update request");
            }

            // Send file_info_update via the socket
            if (send(socket_fd, file_info_update, file_count_update * sizeof(struct File_Info), 0) > 0) {
                syslog(LOG_INFO, "Data (%d) updated successfully!", file_count_update);
            } else {
                syslog(LOG_ERR, "Failed to send updated data");
                exit(EXIT_FAILURE);
            }

            // Update file_info for the next comparison
            file_info = realloc(file_info, file_count_update * sizeof(struct File_Info));
            memcpy(file_info, file_info_update, file_count_update * sizeof(struct File_Info));
            file_count = file_count_update;

            // Free the memory used by file_info_update
            free(file_info_update);
        }
    }
    free(file_info);
    pthread_exit(NULL);
}


/*************** function which displays the list of available files **************/
void request_list_files(int socket_fd){

     char *id_send= "list";
     int count_file=0, bytes_received;
     File list_files = malloc(1024* sizeof(struct File_Info));

    // Send the file request to the server
     if(send(socket_fd, id_send,  strlen(id_send),0) > 0){
        syslog(LOG_INFO, "sends successful file list request");
     }
     else{
        syslog(LOG_ERR, "Failed to send file list request");
     }

    // Receive file content from the server
     if((bytes_received= recv(socket_fd, list_files, 1024 * sizeof(struct File_Info), 0)) > 0){
         syslog(LOG_INFO, "successful receipt of file list");
     }
     else{
         syslog(LOG_ERR,"failed to receive file list");
    }

    // Calculate the number of files received
     count_file = bytes_received / sizeof(struct File_Info);

     printf("\t\t LIST OF AVAILABLE FILES \n\n");
    // Prints the list of file informations
     for (int i = 0; i < count_file; i++) {
        fprintf(stdout, " %d) %s, %ld bytes\n",i+1, list_files[i].filename, list_files[i].filesize);
    }

    free(list_files);
}



/***************************** MENU ****************************/
void printMenu(int socket_fd, char *path_directory) {
    int choice;
    int validChoice = 0;

    pthread_t thread_send;

    while (1) {
        printf("\nMenu:\n");
        printf("1. Send file information to the server\n");
        printf("2. List files\n");
        printf("3. Quit\n");
        printf("Your choice: ");

        do {
            if (scanf("%d", &choice) != 1 || choice < 1 || choice > 3) {
                printf("Invalid choice. Please choose a positive integer between 1 and 3: ");
                while (getchar() != '\n');  // Clear input buffer
            } else {
                validChoice = 1;
            }
        } while (!validChoice);

        switch (choice) {
            case 1:
                if (pthread_create(&thread_send, NULL, Send_File_Info, &socket_fd) < 0) {
                    syslog(LOG_ERR, "Thread creation failed");
                    exit(EXIT_FAILURE);
                }

                // detach the thread
                if (pthread_detach(thread_send) != 0) {
                    syslog(LOG_ERR, "Thread detach failed");
                    exit(EXIT_FAILURE);
                }

                break;
            case 2:
                request_list_files(socket_fd);
                break;
            case 3:
                printf("Goodbye!\n");
                return;
            default:
                printf("Invalid choice. Please choose again.\n");
        }
    }
}

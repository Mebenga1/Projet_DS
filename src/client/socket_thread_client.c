#include "socket_thread_client.h"

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))

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

    file_infos = realloc(file_infos, client_count * sizeof(struct File_Info));

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
        //fprintf(stdout, "\n\n");

    int id_send=1;
                // Send the file request to the server
    if(send(socket_fd, &id_send, sizeof(id_send),0) > 0){
        syslog(LOG_INFO, "sends successful file list request");
    }
    else{
        syslog(LOG_ERR, "Failed to send filedl ist request");
    }

    // Sends file_info via the socket
    if(send(socket_fd, file_info, file_count * sizeof(struct File_Info), 0) > 0){
        //fprintf(stdout, " \nData (%d) sent successfully !!!!! \n", file_count);
        syslog(LOG_INFO, "Data (%d) sent successfully!", file_count);

        for (int i = 0; i < file_count; i++) {
            fprintf(stdout, " %s, %ld bytes\n", file_info[i].filename, file_info[i].filesize); // Prints file information
        }
    }
    else{
        syslog(LOG_ERR,"Send failed, %m");
        exit(EXIT_FAILURE);
    }

/*********************** Monitoring of the directory ******************************************/
    int fd, wd;
    char *array_events = malloc(BUF_LEN);

    if (array_events == NULL) {
        syslog(LOG_ERR,"Error allocating array_events");
        exit(EXIT_FAILURE);
    }

    fd = inotify_init();
    if (fd == -1) {
        syslog(LOG_ERR,"Error initializing inotify instance");
        free(array_events);
        exit(EXIT_FAILURE);
    }

    wd = inotify_add_watch(fd, "./data/", IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd == -1) {
        syslog(LOG_ERR,"Error adding directory to watch");
        free(array_events);
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("\nMonitoring directory:  ./data/\n");
    syslog(LOG_ERR,"Monitoring directory");
    while (1) {
        int length, num_events = 0;

        length = read(fd, array_events, BUF_LEN);
        if (length == -1) {
            syslog(LOG_ERR,"Error reading inotify events");
            break;
        }

         File file_info_update = NULL;

        while (num_events < length) {
            struct inotify_event *event = (struct inotify_event *)&array_events[num_events];
            // Check if the event has a non-zero length
            if (event->len) {

                // Check if the event type is create, delete, modify, move from, or move to
                if ((event->mask & IN_CREATE) || (event->mask & IN_DELETE) || (event->mask & IN_MODIFY) || (event->mask & IN_MOVED_FROM) || (event->mask & IN_MOVED_TO)) {

                        //FILE file_info_update= malloc( 1024 * sizeof(struct File_Info));
                         file_info_update = Read_directory("./data/");
                        int file_count_update = Get_File_Count(file_info_update);

                        for (int i = 0; i < file_count_update; i++) {
                            fprintf(stdout, " %s, %ld bytes\n", file_info_update[i].filename, file_info_update[i].filesize); // Prints file information
                        }

                        if(send(socket_fd, &id_send, sizeof(id_send),0) > 0){
                            syslog(LOG_INFO, "sends successful file list request");
                        }
                        else{
                            syslog(LOG_ERR, "Failed to send filedl ist request");
                        }

                        // Sends file_info via the socket
                        if(send(socket_fd, file_info_update, file_count_update * sizeof(struct File_Info), 0) > 0){
                            syslog(LOG_INFO, "Data (%d) update successfully!", file_count_update);
                        }
                        else{
                            syslog(LOG_ERR,"update failed, %m");
                            exit(EXIT_FAILURE);
                        }

                }
            }
            // Move to the next event
            num_events += EVENT_SIZE + event->len;
        }
    }
    free(array_events);
    inotify_rm_watch(fd, wd);
    close(fd);
    free(file_info);
    pthread_exit(NULL);
}


/*************** function which displays the list of available files **************/
void request_list_files(int socket_fd){

     int id_send= 2, count_file=0, bytes_received;
     File list_files = malloc(1024* sizeof(struct File_Info));

    // Send the file request to the server
     if(send(socket_fd, &id_send, sizeof(int),0) > 0){
        syslog(LOG_INFO, "sends successful file list request");
     }
     else{
        syslog(LOG_ERR, "Failed to send filedl ist request");
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

    pthread_t thread_send;

    while (1) {
        printf("\nMenu:\n");
        printf("1. Send file information to the server\n");
        printf("2. List files\n");
        printf("3. Quit\n");
        printf("Your choice: ");

        scanf("%d", &choice);
        switch (choice) {
            case 1:
                if (pthread_create(&thread_send, NULL, Send_File_Info, &socket_fd) < 0) {
                syslog(LOG_ERR, "Thread creation failed: %m");
                exit(EXIT_FAILURE);
                }

                // detach the thread
                if (pthread_detach(thread_send) != 0) {
                    syslog(LOG_ERR, "Thread detach failed: %m");
                    exit(EXIT_FAILURE);
                }
                //Send_File_Info(socket_fd, path_directory);
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

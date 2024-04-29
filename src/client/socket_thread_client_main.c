#include "socket_thread_client.h"

int main(int argc, char *argv[]) {
    if (argc != 3) { // Checks if the correct number of command line arguments is provided
        fprintf(stderr, "Usage: %s <port> <server IP>\n", argv[0]);
        return -1;
    }

    openlog("client", LOG_PID | LOG_CONS, LOG_USER);

    int port= atoi(argv[1]);
    // Sends data to the server
    Send_File_Info(port, argv[2],"./data/"); // Calls a function to send file
    Monitoring_directory(port, argv[2],"./data/"); // Calls a function to monitor the directory and send updates to the server
    closelog();

    return 0;
}

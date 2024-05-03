#include "socket_thread_client.h"

int main(int argc, char *argv[]) {
    if (argc != 3) { // Checks if the correct number of command line arguments is provided
        fprintf(stderr, "Usage: %s <port> <server IP>\n", argv[0]);
        return -1;
    }

    openlog("client", LOG_PID | LOG_CONS, LOG_USER);


    int socket_fd = 0; // File descriptor for the new socket
    struct sockaddr_in server_address; // Structure for the server's IP address and connection port
    int port= atoi(argv[1]); //server port

    // Creates a socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // Checks if socket creation fails
        syslog(LOG_ERR, "Socket creation failed, %m");
        exit(-1);
    } // Added missing closing parenthesis

    // Initializes values in the server_address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // Sets the port number by converting from string to integer

    // Converts the server's IP address from string to binary format
    if (inet_pton(AF_INET, argv[2], &server_address.sin_addr) <= 0) {
        syslog(LOG_ERR, "Invalid or unsupported IP address, %m");
        exit(-2);
    }

    // Connects to the server
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        syslog(LOG_ERR, "Connection failed, %m");
        fprintf(stderr, "Connection failed, %m");
        exit(-3);
    }
    // Sends data to the server

    printMenu(socket_fd,"./data/");



    // Closes the socket
    close(socket_fd);

    closelog();

    return 0;
}

#include "socket_thread_server.h"

int main(int argc, char *argv[]) {
    int server_fd, new_socket_fd; // File descriptors for the server socket and new socket connections
    struct sockaddr_in address; // Structure for server address information
    int addrlen = sizeof(address); // Length of address structure
    int opt = 1; // Socket options
    pthread_t thread_id; // Thread ID variable

    if (argc < 2) { // Checks if port number is provided as a command-line argument
        perror("Usage: ./serveur <port>\n [ERROR]: Port not provided !!");
        exit(EXIT_FAILURE); //
    }

    // Creates a socket file descriptor for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed !!\n");
        exit(EXIT_FAILURE);
    }

    // Sets socket options to allow reusing the address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt error \n");
        exit(EXIT_FAILURE);
    }

    // Configures the server address
    address.sin_family = AF_INET; // Specifies the address family
    address.sin_addr.s_addr = INADDR_ANY; // Binds to any available local IP address
    address.sin_port = htons(atoi(argv[1])); // Converts and sets the port number from command-line argument

    // Binds the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed !!\n");
        exit(EXIT_FAILURE);
    }

    // Starts listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Error initializing listening !!\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server started on port: %d\n\n", atoi(argv[1])); // Prints a message indicating server startup

    while (1) {
        // Accepts incoming connections
        if ((new_socket_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Error accepting incoming connection");
            exit(EXIT_FAILURE);
        }

        char client_ip[INET_ADDRSTRLEN]; // Buffer for client IP address
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN); // Converts client IP to string format

        printf("New connection from %s with port %d \n\n", client_ip, ntohs(address.sin_port)); // Prints client connection details

        int *new_socket_ptr = malloc(sizeof(int)); // Allocates memory for new socket descriptor
        *new_socket_ptr = new_socket_fd; // Assigns the new socket descriptor

        // Creates a thread to handle the new connection
        if (pthread_create(&thread_id, NULL, receive_request, (void *)new_socket_ptr) < 0) {
            perror("Thread creation failed !!");
            exit(EXIT_FAILURE);
        }

        // Waits for the thread to finish before proceeding
        if (pthread_join(thread_id, NULL) != 0) {
            perror("Thread join failed !!");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
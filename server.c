#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080 //defining port number
#define BACKLOG 1 //deifning max number of pending connections in queue
#define BUFFER_SIZE 1024

int main()
{

    
    int sockfd, new_socket;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE] = {0};
    int addrlen = sizeof(addr);
    const char *hello = "Hello from server!";

    // Clear and set up address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT); //set port to 8080
    addr.sin_addr.s_addr = htonl(INADDR_ANY);


    //Creation of a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Socket creation failed");
        exit(1);
    }
    //binding the socket
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failure");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) < 0) {
        perror("Listen failed");
        exit(1);
    }

    if ((new_socket = accept(sockfd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(1);
    }

    printf("Client connected\n");

    read(new_socket, buffer, BUFFER_SIZE);
    printf("Messaged: %s\n", buffer);

    write(new_socket, hello, strlen(hello));
    close(new_socket);
    close(sockfd);
    return 0;

    
    

}

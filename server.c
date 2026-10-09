#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080 //defining port number
#define BACKLOG 1 //deifning max number of pending connections in queue
// MISTAKE FIX: BUFFER_SIZE was used for the buffer below but never defined, so the compiler
// didn't know what it was. Constants must be #defined before they are used.
#define BUFFER_SIZE 1024


static ssize_t read_line(int fd, char *buf, size_t max) {
    size_t total = 0;
    while (total < max - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        buf[total++] = c;
        if (c == '\n') break;
    }
    buf[total] = '\0';
    return (ssize_t)total;
}

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


    //read(new_socket, buffer, BUFFER_SIZE); //This can lead to overflow if the user inputs more than 1024 bytes.
    //printf("Messaged: %s\n", buffer);
    ssize_t n = read_line(new_socket, buffer, sizeof(buffer));
    if (n < 0)       perror("Read failed");
    else if (n == 0) printf("client closed without sending\n");
    else             printf("Message: %s", buffer);   // buffer still has its '\n'

    ssize_t w = send(new_socket, hello, strlen(hello), MSG_NOSIGNAL);
    if (w < 0) {
        // -1 means the call failed, so errno tells us why
        if (errno == EPIPE) {
            printf("client went away\n"); // client closed the connection before we replied
        } else {
            perror("send"); // any other failure: print the system's error message
        }
    // The "else if" must chain off the "if (w < 0)" above, not off the inner if/else (that was the
    // syntax error). It is only checked when w >= 0, i.e. send() worked but maybe not fully.
    // Cast strlen's size_t to ssize_t so we compare signed to signed, avoiding a type-mismatch warning.
    } else if (w < (ssize_t) strlen(hello)) {
        printf("short write: sent %zd of %zu bytes\n", w, strlen(hello));
    }
    close(new_socket);
    close(sockfd);
    return 0;

    
    

}

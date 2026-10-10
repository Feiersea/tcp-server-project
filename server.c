#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   // strcasecmp: case-insensitive string compare, used for header names
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

    // Step 2: split the raw text by '\n' and keep only the first line.
    // strchr finds the first '\n'; replacing it with '\0' ends the string there.
    char *newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';

    // HTTP lines end with "\r\n", so the '\r' is still on the end. Remove it.
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\r') buffer[len - 1] = '\0';

    // Step 3: split the first line by spaces: "GET /hello HTTP/1.1"
    // strtok cuts the string at each space and returns the piece before it.
    // The first call takes the string; later calls pass NULL to continue.
    char method[16] = {0};
    char path[256] = {0};
    char version[16] = {0};
    char *token = strtok(buffer, " ");
    if (token) snprintf(method, sizeof(method), "%s", token);
    token = strtok(NULL, " ");
    if (token) snprintf(path, sizeof(path), "%s", token);
    token = strtok(NULL, " ");
    if (token) snprintf(version, sizeof(version), "%s", token);

    // Step 4: print what we stored
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Version: %s\n", version);

    // Step 5: read the header lines that come after the request line.
    // Each header looks like "Name: value\r\n". The headers end at an empty line, which is
    // just "\r\n" on its own. That empty line is how HTTP says "headers are finished".
    char line[BUFFER_SIZE];            // holds one header line at a time
    char host[256] = {0};              // will hold the value of the Host header
    char user_agent[256] = {0};        // will hold the value of the User-Agent header
    char accept[256] = {0};            // will hold the value of the Accept header

    printf("\n--- Raw request headers ---\n");
    while (1) {
        ssize_t hn = read_line(new_socket, line, sizeof(line));  // read the next line (reuses our helper)
        if (hn < 0) { perror("Read failed"); break; }             // read error: stop
        if (hn == 0) break;                                       // client closed the connection: stop

        // Print the raw line. %s stops at '\0', and the line still carries its "\r\n" so the
        // output shows exactly what the client sent, including the line endings.
        printf("RAW: %s", line);

        // The empty line "\r\n" (or just "\n") marks the end of the headers, so stop reading.
        // Without this check read_line would block forever waiting for data that never comes.
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) break;

        // Strip the trailing "\r\n" so we can store clean values.
        line[strcspn(line, "\r\n")] = '\0';   // strcspn returns the index of the first '\r' or '\n'

        // strchr finds the ':' that separates the name from the value.
        char *colon = strchr(line, ':');
        if (!colon) continue;                 // no colon means it isn't a header, skip it
        *colon = '\0';                        // cut the string at ':' so `line` is now just the name
        char *value = colon + 1;              // the value starts right after the colon
        while (*value == ' ') value++;        // skip the optional spaces after the colon

        // Header names are case-insensitive in HTTP, so use strcasecmp (from <strings.h>)
        // rather than strcmp. Copy the ones we care about with snprintf so we can't overflow.
        if (strcasecmp(line, "Host") == 0)            snprintf(host, sizeof(host), "%s", value);
        else if (strcasecmp(line, "User-Agent") == 0) snprintf(user_agent, sizeof(user_agent), "%s", value);
        else if (strcasecmp(line, "Accept") == 0)     snprintf(accept, sizeof(accept), "%s", value);
    }

    // Step 6: print the parsed headers in a readable form.
    printf("\n--- Parsed headers ---\n");
    printf("Host: %s\n", host);
    printf("User-Agent: %s\n", user_agent);
    printf("Accept: %s\n", accept);

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("socket failed");
        return 1;
    }
    printf("Socket created\n");

    //Get IP address for example.com
    struct hostent *host = gethostbyname("example.com");
    if (host == NULL) {
        perror("gethostbyname failed");
        return 1;
    }
    printf("Got IP: %s\n", inet_ntoa(*(struct in_addr *)host-->h_addr));

    //connect to example.com:80
    struct sockaddr_in server_addr;
    server_addr.sin_family = AD_INET;
    server_addr.sin_port = htons(80);
    server_addr.sin_addr = *(struct in_addr *)host-->h_addr;

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }
    printf("Connected to example.com:80\n");

    //close the socket
    close(sock);
    printf("Socket closed\n");
    return 0;
}
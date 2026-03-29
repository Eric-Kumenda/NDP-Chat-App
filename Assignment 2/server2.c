#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
#endif

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#define PORT 8080

int main() {
    #ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
    #endif

    int sock;
    struct sockaddr_in serv_addr;
    char message[1024], buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("Connected to server! Type 'quit' to exit.\n");

    while (1) {
        printf("You: ");
        if (!fgets(message, sizeof(message), stdin)) break;
        size_t len = strlen(message);
        if (len > 0 && message[len-1] == '\n') message[len-1] = '\0';

        send(sock, message, (int)strlen(message), 0);
        if (strcmp(message, "quit") == 0) { printf("Disconnected.\n"); break; }

        memset(buffer, 0, sizeof(buffer));
        int recv_size = recv(sock, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) { printf("Server disconnected.\n"); break; }

        printf("Server: %s\n", buffer);
        if (strcmp(buffer, "quit") == 0) { printf("Server ended chat.\n"); break; }
    }

    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif

    return 0;
}
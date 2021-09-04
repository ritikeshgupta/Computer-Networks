// A TCP Server
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#define MAX 100

int main(int argv, char** argc)
{
    fflush(stdout);
    fflush(stdin);

    int sockfd, connfd, filefd;
    struct sockaddr_in servaddr, cliaddr;

    // Create socket file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8080);

    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    printf("Server Binded...\n");
    
    if (listen(sockfd, 2) < 0) 
    { 
        perror("listening failed"); 
        exit(EXIT_FAILURE); 
    }
    printf("Server Listening...\n");

    socklen_t len = sizeof(cliaddr);
    if((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
    {
        perror("connection failed"); 
        exit(EXIT_FAILURE);
    }
    printf("Connection established\n");

    char fname[MAX];
    read(connfd, fname, sizeof(fname));
    if((filefd = open(fname, O_RDONLY, 0664)) < 0)
    {
        perror("File not found... Closing connection");
        close(sockfd);
        close(connfd);
        exit(EXIT_FAILURE);
    }

    // File transfer to client
    char buff[MAX];

    while(1)
    {
        memset(buff, 0, sizeof(buff));

        if(read(filefd, buff, sizeof(buff)) == 0)
        {
            printf("\nFile transfer complete... Closing connection\n");
            break;
        }

        if(write(connfd, buff, sizeof(buff)) < 0)
        {
            perror("Write failed");
            close(filefd);
            close(connfd);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    close(filefd);
    close(connfd);
    close(sockfd);
    return 0;
}

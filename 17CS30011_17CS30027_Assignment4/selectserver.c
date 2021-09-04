// A TCP Server
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <dirent.h>
#include <netdb.h>

#define MAX 100

int isaplha(char c)
{
    if(c >= 'a' && c <= 'z') return 1;
    if(c >= 'A' && c <= 'Z') return 1;
    return 0;
}

void tcp_comm(int connfd, int sockfd)
{
    int filefd;
    DIR *dir;
    char dirname[MAX] = "./image/";
    char path[MAX];
    read(connfd, path, sizeof(path));
    strcat(dirname, path);
    strcat(dirname, "/");

    if((dir = opendir(dirname)) == NULL)
    {
        perror("\033[1;32m[TCP]:\033[0m Directory not found... Closing connection");
        close(connfd);
        exit(EXIT_FAILURE);
    }
    // File transfer to client
    char buff[MAX];
    struct dirent *de;
    
    while((de = readdir(dir)) != NULL)
    {
        char path[MAX];
        strncpy(path, dirname, sizeof(dirname));

        if(!isaplha(de->d_name[0]) && (de->d_name[0] < '0' || de->d_name[0] > '9')) continue;
        strcat(path, de->d_name);

        if((filefd = open(path, O_RDONLY, 0777)) < 0)
        {
            perror("\033[1;32m[TCP]:\033[0m Error opening image");
            close(connfd);
            exit(EXIT_FAILURE);
        }
        while(1)
        {
            size_t i = 0;
            memset(buff, 0, sizeof(buff));
            buff[MAX - 1] = '\0';
            if(read(filefd, buff, sizeof(buff) - 1) == 0)
            {
                write(connfd, "$", 1);
                break;
            }
            while(i < sizeof(buff) - 1) buff[i++] = 'a';

            if(write(connfd, buff, sizeof(buff)) < 0)
            {
                perror("\033[1;32m[TCP]:\033[0m Write failed");
                close(filefd);
                close(connfd);
                exit(EXIT_FAILURE);
            }
        }
        close(filefd);
    }
    char end[] = {'E', 'N', 'D', '\0'};
    if(write(connfd, end, sizeof(end)) < 0)
    {
        perror("\033[1;32m[TCP]:\033[0m Write failed");
        closedir(dir);
        close(connfd);
        exit(EXIT_FAILURE);
    }
    
    printf("\033[1;32m[TCP]:\033[0m Images sent\n");
    closedir(dir);
    return;
}

void udp_comm(int sockfd, struct sockaddr_in* cliaddr)
{
    int n;
    socklen_t len;
    char dns[MAX];
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, (char *)dns, MAX, 0, (struct sockaddr *)&cliaddr, &len);

    dns[n] = '\0';
    printf("\033[1;31m[UDP]:\033[0m Domain name from client: %s\n", dns);

    struct hostent *host = gethostbyname(dns);
    char *ip_addr = "Invalid request";
    if(host != NULL) ip_addr = inet_ntoa(*((struct in_addr*)host->h_addr_list[0])); 

    sendto(sockfd, (const char *)ip_addr, strlen(ip_addr), 0, (const struct sockaddr *) &cliaddr, len);
    
    return;
}   

int main(int argv, char** argc)
{
    fflush(NULL);

    int udpfd, tcpfd, t_connfd;
    struct sockaddr_in servaddr, cliaddr1, cliaddr2;

    // Create socket file descriptor
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(tcpfd < 0 || udpfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    if(setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr1, 0, sizeof(cliaddr1));
    memset(&cliaddr2, 0, sizeof(cliaddr2));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8080);

    // Bind the tcp socket with the server address 
    if(bind(tcpfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    // Bind the udp socket with the server address 
    if(bind(udpfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    printf("Server Binded...\n");
    listen(tcpfd, 7);

    fd_set readfd;
    while(1)
    {
        FD_ZERO(&readfd);
        FD_SET(0, &readfd);
        FD_SET(tcpfd, &readfd);
        FD_SET(udpfd, &readfd);

        int ready = 0;
        
        if((ready = select(udpfd + 1, &readfd, NULL, NULL, NULL)) < 0)
        {
            perror("Failed to make select call");
            exit(EXIT_FAILURE);
        }
        else if(ready > 0 && FD_ISSET(tcpfd, &readfd))
        {
            pid_t child;
            if((child = fork()) < 0)
            {
                perror("\033[1;32m[TCP]:\033[0m Error creating fork");
                exit(EXIT_FAILURE); 
            }
            else if(child == 0)
            {
                printf("\033[1;32m[TCP Client]\033[0m\n");
                socklen_t len = sizeof(cliaddr1);
                if((t_connfd = accept(tcpfd, (struct sockaddr *)&cliaddr1, &len)) < 0)
                {
                    perror("\033[1;32m[TCP]:\033[0m Connection failed"); 
                    exit(EXIT_FAILURE);
                }
                printf("\033[1;32m[TCP]:\033[0m Connection established\n");
                tcp_comm(t_connfd, tcpfd);
                printf("\033[1;32m[TCP]:\033[0m Done...\n");

                close(t_connfd);
                exit(0);
            }
            usleep(1000);
        }
        else if(ready > 0 && FD_ISSET(udpfd, &readfd))
        {
            printf("\033[1;31m[UDP Client]\033[0m\n");

            udp_comm(udpfd, &cliaddr2);
            printf("\033[1;31m[UDP]:\033[0m Done...\n");
        }
    }

    fflush(NULL);

    close(tcpfd);
    close(udpfd);
    return 0;
}
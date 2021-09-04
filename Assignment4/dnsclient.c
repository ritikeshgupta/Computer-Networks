// UDP Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#define MAXLINE 1024

int main(int argv, char **argc) {

    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8080); 

    socklen_t len; 

    // Send domain name to server
    char dns[MAXLINE]; 
    printf("Enter the domain name: ");
    scanf("%s", dns);

    sendto(sockfd, (const char *)dns, strlen(dns), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    char h_name[MAXLINE];

    len = sizeof(servaddr);
    int n = recvfrom(sockfd, (char *)h_name, MAXLINE, 0, (struct sockaddr *)&servaddr, &len);
    h_name[n] = '\0';
    printf("IP address received: %s\n", h_name);

    close(sockfd);
    return 0; 
} 

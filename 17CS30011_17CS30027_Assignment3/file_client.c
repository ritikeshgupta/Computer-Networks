// A TCP Client
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <poll.h>

#define MAX 100

int main(int argv, char **argc) {

    fflush(stdout);
    fflush(stdin);

    int sockfd, connfd, filefd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8080);

    socklen_t len = sizeof(servaddr);
    if((connfd = connect(sockfd, (struct sockaddr *)&servaddr, len)) < 0)
    {
        perror("connection failed"); 
        exit(EXIT_FAILURE);
    }
    printf("Connection established\n");

    // get the file name and send to server
    char fname[MAX];
    printf("Enter the file name: ");
    scanf("%s", fname);
    
    write(sockfd, (const char *) fname, sizeof(fname));

    // check if file found
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN | POLLHUP;
    pfd.revents = 0;

    char buff[MAX];

    if(poll(&pfd, 1, 10) > 0)
    {
        if(read(sockfd, buff, sizeof(buff)) <= 0)
        {
            perror("File Not Found");
            close(connfd);
            close(sockfd);
            exit(1);
        }
    }

    char *output = "file_response.txt";
    if((filefd = open(output, O_CREAT | O_WRONLY, 0777)) < 0)
    {
        perror("Error creating file");
        close(connfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // File transfer from server
    int word_count = 0, byte_count = 0;
    int flag = 1;
    while(1)
    {
        size_t i = 0;
        word_count -= (1 - flag);
        while(i < sizeof(buff) && buff[i] != '\0')
        {
            byte_count++;
            if(buff[i] == ',' || buff[i] == ';'  || buff[i] == ':' || buff[i] == '.'
            || buff[i] == ' ' || buff[i] == '\t' || buff[i] == '\n')
            {
                if(!flag) word_count++;
                flag = 1;
            }
            else if(buff[i] >= '\0') flag = 0;
            i++;
        }
        if(!flag) word_count++;

        if(write(filefd, buff, i) < 0)
        {
            perror("Write failed");
            close(filefd);
            close(connfd);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        memset(buff, 0, sizeof(buff));
        if(read(sockfd, buff, sizeof(buff)) <= 0)
        {
            printf("\nFile received...\nSize of the file = %d bytes, no. of words = %d\n", byte_count, word_count);
            break;
        }
    }

    close(filefd);
    close(connfd);
    close(sockfd);
    return 0;
}
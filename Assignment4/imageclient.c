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
#include <dirent.h>

#define MAX 100

int check(char *ch_e)
{   
    int found = 0;
    if(ch_e[0] == 'E' && ch_e[1] == 'N' && ch_e[2] == 'D') found = 1;
    else if(ch_e[1] == 'E' && ch_e[2] == 'N' && ch_e[3] == 'D') found = 1;

    if(!found)
    {
        ch_e[0] = ch_e[2];
        ch_e[1] = ch_e[3];
    }

    return found;
}

int main(int argv, char **argc) {

    fflush(stdout);
    fflush(stdin);

    int sockfd, connfd; 
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
    char subdirname[MAX];
    printf("Enter the subdirectory name: ");
    scanf("%s", subdirname);
    
    write(sockfd, (const char *) subdirname, sizeof(subdirname));

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
            perror("Directory Not Found");
            close(connfd);
            close(sockfd);
            exit(1);
        }
    }

    int img_count = 0;
    char check_end[4] = {'\0', '\0', '\0', '\0'};
    while(1)
    {
        size_t i = 0;

        while(i < sizeof(buff)) 
            if(buff[i++] == '$')
            {
                img_count++;
                printf("Image received\n");
                break;
            }

        check_end[2] = buff[i - 2];
        check_end[3] = buff[i - 1];

        if(check(check_end)) break;
        for(int i = 0; i < sizeof(buff) - 2; i++) if(buff[i] == 'E' && buff[i + 1] == 'N' && buff[i + 2] == 'D') break;

        memset(buff, 0, sizeof(buff));
        ssize_t rd;
        if((rd = read(sockfd, buff, sizeof(buff))) < 0)
        {
            printf("Error reading stream");
            break;
        }
        else if(rd == 0) break;
    }

    printf("Done...\n");
    printf("No. of images received: %d\n", img_count);
    close(connfd);
    close(sockfd);
    return 0;
}
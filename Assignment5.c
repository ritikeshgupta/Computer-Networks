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
#include <signal.h>
#include <errno.h>

#define MAX 1024
#define MAX_CONN 1024
#define DEF_PORT 1234

char data[MAX];

int max(int a, int b){if(a>b)return a; return b;}

void transfer(int src, int dest)
{
    memset(data, 0, MAX);
    int data_size;
    data_size = recv(src, data, MAX, 0);
    send(dest, data, data_size, 0);
}

int main(int argv, char **argc)
{
    fflush(NULL);

    if(argv < 4)
    {
        perror("Usage : ./simproxy <listen-port>, <institute-IP>, <institute-port>\n");
        exit(EXIT_SUCCESS);
    }

    uint16_t LISTEN_PORT = atoi(argc[1]);
    char *INSTI_IP = argc[2]; 
    uint16_t INSTI_PORT = atoi(argc[3]);

    int tcpfd, t_connfd, instifd, i_connfd;
    struct sockaddr_in servaddr_browser, servaddr_insti, cliaddr;

    // Create socket file descriptor
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    instifd = socket(AF_INET, SOCK_STREAM, 0);

    if(tcpfd < 0 || instifd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if(fcntl(tcpfd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("Non-blocking call failed\n");
        exit(EXIT_FAILURE);
    }

    if(fcntl(instifd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("Non-blocking call failed\n");
        exit(EXIT_FAILURE);
    }

    if((setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) || (setsockopt(instifd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0))
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr_browser, 0, sizeof(servaddr_browser)); 
    memset(&servaddr_insti, 0, sizeof(servaddr_insti));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr_browser.sin_family = AF_INET;
    servaddr_browser.sin_addr.s_addr = INADDR_ANY;
    servaddr_browser.sin_port = htons(LISTEN_PORT);

    servaddr_insti.sin_family = AF_INET;
    servaddr_insti.sin_addr.s_addr = inet_addr(INSTI_IP);
    servaddr_insti.sin_port = htons(INSTI_PORT);

    // Bind the tcp socket with the server address 
    if(bind(tcpfd, (const struct sockaddr *)&servaddr_browser, sizeof(servaddr_browser)) < 0) 
    { 
        perror("Bind failed"); 
        exit(EXIT_FAILURE); 
    }
    printf("============ Proxy Server Binded ==========\n");

    if(listen(tcpfd, MAX_CONN) < 0)
    {
        perror("tcpfd Listen failed\n");
        exit(EXIT_FAILURE);
    }
    printf("================ Listening ================\n\n");

    fd_set read_fd;
    fd_set write_fd;

    int browser_fd[MAX_CONN], inst_proxy_fd[MAX_CONN], num_conn = 0, maxfd;

    /* To ignore broken pipe signal */
    signal(SIGPIPE, SIG_IGN);

    while(1)
    {
        FD_ZERO(&read_fd);
        FD_ZERO(&write_fd);
        FD_SET(STDIN_FILENO, &read_fd);
        FD_SET(tcpfd, &read_fd);
        maxfd = tcpfd;

        for(int i = 0 ; i < num_conn ; i++)
        {
            FD_SET(browser_fd[i], &read_fd);
            FD_SET(inst_proxy_fd[i], &read_fd);
            FD_SET(browser_fd[i], &write_fd);
            FD_SET(inst_proxy_fd[i], &write_fd);
            
            maxfd = max(max(maxfd, browser_fd[i]), inst_proxy_fd[i]);
        }

        int flag = -1;
        if((flag = select(maxfd + 1, &read_fd, &write_fd, NULL, NULL)) < 0)
        {
            perror("Error in select() call");
            continue;
        }
        
        if(FD_ISSET(tcpfd, &read_fd))
        {   
            socklen_t len = sizeof(cliaddr);
            if( num_conn > MAX_CONN || (browser_fd[num_conn] = accept(tcpfd, (struct sockaddr *)&cliaddr, &len)) < 0)
            {
                perror("Tcp connection failed\n");
                exit(EXIT_FAILURE);
            }
        
            printf("\033[1;31m[%d]\033[0m :: Connection accepted from \033[1;32m%s\033[0m : %d\n", num_conn, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port) );

            if( (inst_proxy_fd[num_conn] = socket(AF_INET, SOCK_STREAM, 0)) < 0)    // Creating a socket for data transfer between institute proxy and 
            {                                                                       // browser through this code(PROXY). 
                perror("Socket Creation Failed\n");
                exit(EXIT_FAILURE);
            }

            if(fcntl(browser_fd[num_conn], F_SETFL, O_NONBLOCK) < 0)
            {
                perror("Non blocking call failed\n");
                exit(EXIT_FAILURE);
            }

            if(fcntl(inst_proxy_fd[num_conn], F_SETFL, O_NONBLOCK) < 0)
            {
                perror("Non blocking call failed\n");
                exit(EXIT_FAILURE);
            }

            connect(inst_proxy_fd[num_conn], (const struct sockaddr *)&servaddr_insti, sizeof(servaddr_insti)); // Connecting with instiute proxy

            num_conn++;
        }
        
        if(FD_ISSET(STDIN_FILENO, &read_fd))   // If something is typed in the terminal
        {
            char cmd[MAX];
            scanf("%s", cmd);

            if(strcmp(cmd, "exit") == 0)
            {
                for(int i = 0 ; i < num_conn ; i++)
                {
                    close(browser_fd[i]);
                    close(inst_proxy_fd[i]);
                }
                close(tcpfd);
                close(instifd);
                exit(EXIT_SUCCESS);
            }
        }

        for(int i = 0 ; i < num_conn ; i++)
        {
            if(FD_ISSET(browser_fd[i], &read_fd) && FD_ISSET(inst_proxy_fd[i], &write_fd))
                transfer(browser_fd[i], inst_proxy_fd[i]);
    
            if(FD_ISSET(inst_proxy_fd[i], &read_fd) && FD_ISSET(browser_fd[i], &write_fd))
                transfer(inst_proxy_fd[i], browser_fd[i]);
        }
    }
}
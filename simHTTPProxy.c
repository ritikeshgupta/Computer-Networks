#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <dirent.h>

#define PORT 8080
#define BUFF_SIZE 1024
#define MAX 1024
#define MAX_CONN 1024
#define DEF_PORT 1234
#define DEBUG 0
#define URL_SIZE 256

int max(int a, int b){if(a>b)return a; return b;}

typedef struct http_header 
{
    char host[128];
    char type[64];
    char path[128];
    char url[128];
    int port;
}HR;

HR parse_request (char *request_msg) 
{
    HR hr;
    char tmp_url[URL_SIZE];
    int j = 0;
    int i;

    if (DEBUG)
        printf ("Here\n");

    for (i=0; i<15 && request_msg[i] && request_msg[i]!=' '; i++)
        hr.type[i] = request_msg[i];
    hr.type[i] = '\0';
    if (DEBUG)
        printf ("Here\n");
    i++;
    for (; request_msg[i] && request_msg[i]==' '; i++);
    if (DEBUG)
        printf ("Here\n");
    for (; request_msg[i] && request_msg[i]!=' '; i++)
        tmp_url[j++] = request_msg[i];
    tmp_url[j] = '\0';
    if (DEBUG)
        printf ("Here\n");
    int k = 0, l = 0;
    if (j > 4 && (strncmp (tmp_url, "http://", 7) == 0) )
    {
        while (tmp_url[k]!=':') k++;
        k+=3;
        if (DEBUG)
        printf ("Here\n");
        while (k < URL_SIZE && tmp_url[k] && tmp_url[k]!=':' && tmp_url[k]!='/' && tmp_url[k]!=' ') hr.host[l++] = tmp_url[k++];
    } 
    else 
    {
        if (DEBUG)
        printf ("Here\n");
        while (k < URL_SIZE && tmp_url[k] && tmp_url[k]!=':' && tmp_url[k]!=' ') hr.host[l++] = tmp_url[k++];
    }
    if (DEBUG)
        printf ("Here\n");
    hr.host[l] = '\0';
    l = 0;
    if (tmp_url[k]==':') 
    {
        k++;
        char temp[64];
        if (DEBUG)
        printf ("Here\n");
        for (; k < URL_SIZE && tmp_url[k] && tmp_url[k]!=' '; k++) temp[l++] = tmp_url[k];
        temp[l]='\0';
        hr.port = atoi (temp);
    } 
    else 
    {
        hr.port = 80;
    }
    strcpy (hr.url, tmp_url);
    if (DEBUG)
        printf ("Here\n");
    int m=0;
    while (tmp_url[m] && tmp_url[m]!='/') m++;
    m++;
    if (DEBUG)
        printf ("Here\n");
    while (tmp_url[m] && tmp_url[m]!='/') m++;
    m++;
    while (tmp_url[m] && tmp_url[m]!='/') m++;
    m++;
    if (DEBUG)
        printf ("Here\n");
    strcpy (hr.path, "/");
    strcat (hr.path, tmp_url + m);
    return hr;

}

int main(int argc, char *argv[]) 
{
    fflush(NULL);
    signal(SIGPIPE, SIG_IGN);
   
    char request[MAX_CONN][6000];   // temporary buffer
    memset(request, '\0', sizeof(request));

    if (argc != 2) 
    {
        printf("Correct usage : ./SimHTTPProxy <listen port>\n");
        return 0;
    }

    uint16_t LISTEN_PORT = atoi(argv[1]);

    struct sockaddr_in proxyaddr, servaddr_browser, cliaddr;
    socklen_t len, len_cli;

    int tcpfd, maxfd, flag;

    memset(&servaddr_browser, 0, sizeof(servaddr_browser));


    char buff[BUFF_SIZE];
    memset(buff, 0, sizeof(buff));

    // Create tcp socket file descriptor
    if ((tcpfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket in creation failed\n");
        exit(EXIT_FAILURE);
    }

    if (fcntl(tcpfd, F_SETFL, O_NONBLOCK) < 0) 
    {
        perror("Could not make non-blocking socket\n");
        exit(EXIT_FAILURE);
    }

    if((setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0))
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    servaddr_browser.sin_family = AF_INET;
    servaddr_browser.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr_browser.sin_port = htons(LISTEN_PORT);


    // Attach socket to port 8080
    if (bind(tcpfd, (struct sockaddr*)&servaddr_browser, sizeof(servaddr_browser)) < 0)
    {
        perror("In socket Bind failure\n");
        exit(EXIT_FAILURE);
    }
    printf("============ Proxy Server Binded ==========\n");

    // Marks server_fd as a passive socket, now ready to accept incoming connections using connect()
    if (listen(tcpfd, MAX_CONN) < 0)
    {
        perror("Sock in Listen failure\n");
        exit(EXIT_FAILURE);
    }
    printf("================ Listening ================\n\n");

    fd_set read_fd;
    fd_set write_fd;
    FD_ZERO(&read_fd);
    FD_ZERO(&write_fd);

    int browser_fd[MAX_CONN], inst_proxy_fd[MAX_CONN], num_conn = 0;

    while (1) 
    {

        FD_ZERO(&read_fd);
        FD_ZERO(&write_fd);
        FD_SET(STDIN_FILENO, &read_fd);
        maxfd = 1;
        FD_SET(tcpfd, &read_fd);
        maxfd = tcpfd;
        // add all connections to FD SET
        for (int i = 0; i < num_conn; i++) 
        {
            FD_SET(browser_fd[i], &read_fd);
            FD_SET(inst_proxy_fd[i], &read_fd);
            FD_SET(browser_fd[i], &write_fd);
            FD_SET(inst_proxy_fd[i], &write_fd);
            maxfd = max(max(maxfd, browser_fd[i]), inst_proxy_fd[i] );
        }
        maxfd = maxfd + 1;

        
        flag = select(maxfd, &read_fd, &write_fd, NULL, NULL);
        if (flag > 0) 
        {
            if(FD_ISSET(tcpfd, &read_fd))    // New connection
            {
                if (num_conn >= MAX_CONN || (browser_fd[num_conn] = accept(tcpfd, (struct sockaddr *)&cliaddr, &len_cli)) < 0) 
                {
                    perror("Tcp connection failed\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    char *cli_ip; 
                    cli_ip = strdup (inet_ntoa (cliaddr.sin_addr));
                    int cli_Port = (int) ntohs(cliaddr.sin_port);        
                    //printf("Connection accepted from %s:%d\n", cli_ip, cli_Port);

                    num_conn++;
                }
                
            }

            else if(FD_ISSET(STDIN_FILENO, &read_fd))   // If something is typed in the terminal
            {
                char cmd[MAX];
                scanf("%s", cmd);

                if(strcmp(cmd, "exit") == 0)
                {
                    for(int i = 0 ; i < num_conn ; i++)
                    {
                        close(inst_proxy_fd[i]);
                        close(browser_fd[i]);
                    }
                    close(tcpfd);
                    exit(EXIT_SUCCESS);
                }
            }

            for (int i = 0; i < num_conn; i++) 
            {
                char buff[MAX];
                int a, b;

                if(FD_ISSET(browser_fd[i], &read_fd))    // new incoming http request
                {
                    a = read(browser_fd[i], buff, MAX);
                    if(!a)                          
                        continue;
                    strcat(request[i], buff);          // Store in temporary buffer
                    if(strstr(buff, "\r\n\r\n") != NULL)    // Once /r/n/r/n is recieved, send the temporary buffer for parsing and establish new connection with http server
                    {

                        HR hr = parse_request(request[i]);

                        printf ("\033[1;32m%s %s, Host: %s, Path: %s \033[0m\n", hr.type, hr.url, hr.host, hr.path);

                        struct hostent *hn = gethostbyname(hr.host);
                        if(hn == NULL)
                        {
                            perror("invalid host");
                            exit(EXIT_FAILURE);
                        }
                        char ip[MAX];
                        strcpy(ip, inet_ntoa(*( struct in_addr*)hn->h_addr));
                        printf("Resolved ip: %s\n", ip);

                        //tcp connection
                        if ((inst_proxy_fd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
                        {
                            fprintf(stdout, "socket() failed: %s\n", strerror(errno));
                            exit(0);
                        }

                        if (fcntl(inst_proxy_fd[i], F_SETFL, O_NONBLOCK) < 0) 
                        {
                            fprintf(stdout, "fcntl() failed: %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                        }

                        struct sockaddr_in addr;
                        addr.sin_family = AF_INET;
                        addr.sin_port = htons(hr.port);

                        if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
                        {
                            perror("Invalid proxy server address\n");
                            exit(EXIT_FAILURE);
                        }
                        // cannot connect immediately, needs some time
                        connect(inst_proxy_fd[i], (struct sockaddr *)&addr, sizeof(addr));
                    }
                    memset(buff, 0, MAX);

                }
                
                if(FD_ISSET(inst_proxy_fd[i], &write_fd))      // HTTP server ready to accept requests
                {
                    b = write(inst_proxy_fd[i], request[i], strlen(request[i]));
                    memset(request[i], '\0', sizeof(request[i]));
                    if (errno == EPIPE) 
                    {
                        continue;
                    }
                    
                }
                if(FD_ISSET(inst_proxy_fd[i], &read_fd) && FD_ISSET(browser_fd[i], &write_fd))  // Send http response from server to client
                {
                    memset(buff, 0, MAX);
                    a = read(inst_proxy_fd[i], buff, MAX);
                    if(a)
                    {
                        b = send(browser_fd[i], buff, a, 0);
                        if (errno == EPIPE) 
                        {
                            close(browser_fd[i]);
                            continue;
                        }
                    }   
                }
            }
        }
    }
    return 0;
}
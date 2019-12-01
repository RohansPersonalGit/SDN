
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "commonProto.h"
#define PORT "7799" // Port we're listening on

#define TRUE 0
#define FALSE 1
// Get sockaddr, IPv4 or IPv6:

// Return a listening socket
int r = 0; 
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size)
    {
        *fd_size *= 2; // Double it

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

// Main
int main(int argc, char **argv)
{
    struct commandOptions cmdOps;
    int retVal = parseOptions(argc, argv, &cmdOps);
    printf("Command parse outcome %d\n", retVal);

    printf("-k = %d\n", cmdOps.option_k);
    printf("-l = %d\n", cmdOps.option_l);
    printf("-v = %d\n", cmdOps.option_v);
    printf("-r = %d\n", cmdOps.option_r);
    printf("-p = %d\n", cmdOps.option_p);
    printf("-p port = %d\n", cmdOps.source_port);
    printf("Timeout value = %d\n", cmdOps.timeout);
    printf("Host to connect to = %s\n", cmdOps.hostname);
    printf("Port to connect to = %d\n", cmdOps.port);
    char buf[256]; // Buffer for client data

    int fd_count = 0;
    int fd_size = 0;
    if (cmdOps.option_l)
    {
        int listener; // Listening socket descriptor

        int newfd;                          // Newly accept()ed socket descriptor
        struct sockaddr_storage remoteaddr; // Client address
        socklen_t addrlen;

        char remoteIP[INET_ADDRSTRLEN];
        fd_size = 10;
        // Start off with room for 5 connections
        // (We'll realloc as necessary)
        struct pollfd *pfds = malloc(sizeof *pfds * fd_size);
        int readBytes;
        // Set up and get a listening socket
        listener = get_listener_socket(PORT);

        if (listener == -1)
        {
            fprintf(stderr, "error getting listening socket\n");
            exit(1);
        }
        r = validateInputs(cmdOps,&listener); 
        
        // Add the listener to set
        pfds[0].fd = listener;
        pfds[0].events = POLLIN; // Report ready to read on incoming connection
        pfds[1].fd = STDIN_FILENO;
        pfds[1].events = POLLIN;
        fd_count = 2; // For the listener
        int t; 
        if(cmdOps.timeout!=0){
            t = cmdOps.timeout; 
        }else{
            t = -1; 
        }
        // Main loop
        for (;;)
        {
            int poll_count = poll(pfds, fd_count, t);

            if (poll_count == -1)
            {
                perror("poll");
                exit(1);
            }

            // Run through the existing connections looking for data to read
            for (int i = 0; i < fd_count; i++)
            {
                // Check if someone's ready to read
                if (pfds[i].revents & POLLIN)
                { // We got one!!

                    if (pfds[i].fd == listener)
                    {
                        // If listener is ready to read, handle new connection

                        addrlen = sizeof remoteaddr;
                        newfd = accept(listener,
                                       (struct sockaddr *)&remoteaddr,
                                       &addrlen);

                        if (newfd == -1)
                        {
                            perror("accept");
                        }
                        else
                        {
                             //validateInputs(cmdOps,&newfd); 
                            add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
                        }
                    }
                    else if (pfds[i].fd == STDIN_FILENO && (readBytes = read(STDIN_FILENO, buf, sizeof buf)) > 0)
                    {
                        int sender_fd = pfds[i].fd;
                        for (int j = 0; j < fd_count; j++)
                        {
                            int dest_fd = pfds[j].fd;
                            if (dest_fd != listener && dest_fd != sender_fd && dest_fd != STDIN_FILENO)
                            {
                                if (send(dest_fd, buf, readBytes, 0) == -1)
                                {
                                    perror("sensd");
                                }
                            }
                        }
                        memset(buf, 0, sizeof(buf));
                    }
                    else
                    {
                        // If not the listener, we're just a regular client
                        int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

                        int sender_fd = pfds[i].fd;

                        if (nbytes <= 0)
                        {
                            // Got error or connection closed by client
                            if (nbytes == 0)
                            {
                                // Connection closed
                                printf("pollserver: socket %d hung up\n", sender_fd);
                            }
                            else
                            {
                                perror("recv");
                            }

                            close(pfds[i].fd); // Bye!

                            del_from_pfds(pfds, i, &fd_count);
                        }
                        else
                        {
                            // We got some good data from a client
                            fprintf(stdout, buf);
                            if(r){
                            for (int j = 0; j < fd_count; j++)
                            {
                                // Send to everyone!
                                int dest_fd = pfds[j].fd;

                                // Except the listener and ourselves
                                if (dest_fd != listener && dest_fd != sender_fd && dest_fd != STDIN_FILENO)
                                {
                                    if (send(dest_fd, buf, nbytes, 0) == -1)
                                    {
                                        perror("sensd");
                                    }
                                }
                            }
                            }

                            memset(buf, 0, sizeof(buf));
                        }
                    } // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!
    }
    else
    {
        struct pollfd *pfds = malloc(sizeof *pfds * 2);
        int newSocket;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        char port[256];
        sprintf(port, "%d", cmdOps.port);
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        char remoteP[sizeof(int)];
        sprintf(remoteP, "%d", cmdOps.source_port);
        if ((rv = getaddrinfo(cmdOps.hostname, port, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo this \n");
        }
        for (p = servinfo; p != NULL; p = p->ai_next)
        {
            if ((newSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("accept\n");
                continue;
            }
            if (connect(newSocket, p->ai_addr, p->ai_addrlen) == -1)
            {
                perror("connect\n");

                close(newSocket);
                continue;
            }
            break;
        }
        if (p == NULL)
        {
            printf("No Connection Found\n");
            exit(1);
        }
        fd_size = 2;
        pfds[0].fd = newSocket;
        pfds[0].events = POLLIN; // Report ready to read on incoming connection
        pfds[1].fd = STDIN_FILENO;
        pfds[1].events = POLLIN;
        fd_count = 2; // For the listener
        for (;;)
        {
            int poll_count = poll(pfds, fd_count, -1);
            if (poll_count == -1)
            {
                perror("poll");
                exit(1);
            }
            for (int i = 0; i < fd_count; i++)
            {
                if (pfds[i].revents & POLLIN)
                {
                    if (pfds[i].fd == newSocket)
                    {
                        int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
                        int sender_fd = pfds[i].fd;
                        if (nbytes <= 0)
                        {
                            if (nbytes == 0)
                            {
                                printf("The connection closed\n");
                                return 0;
                            }
                            else
                            {
                                perror("recv");
                            }
                        }
                        else
                        {
                            fprintf(stdout, buf);
                        }
                    }
                    else
                    {
                        int ret;
                        if ((ret = read(pfds[i].fd, buf, sizeof buf)) > 0)
                        {
                            if (send(newSocket, buf, ret, 0) == -1)
                            {
                                perror("send");
                                return 0;
                            }
                        }
                    }
                    memset(buf, 0, sizeof buf);
                }
            }
        }
    }

    return 0;
}
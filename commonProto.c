#include "commonProto.h"

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int validateInputs(struct commandOptions f, void *fd)
{
    int socket = *((int *)fd);
    if (f.option_k)
    {
        if (!f.option_l)
        {
            perror("It is an error to use the -k without -l");
            exit(1);
        }
    }
    if (f.timeout > 0)
    {
        struct timeval timeout;
        timeout.tv_sec = f.timeout;
        timeout.tv_usec = 0;
        if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            perror("set sock opt failed");
        }
        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            perror("set sock opt failed");
        }
    }
    if (f.option_p & !f.option_l)
    {
        perror("it is an error to use the -p without -l 0");
        exit(1);
    }
    if (f.option_r && !f.option_l)
    {
        perror("it is an error to use r without l");
        exit(1);
    }
    if (f.option_r)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int get_listener_socket(char *port)
{
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;
    //
    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // Lose the pesky "address already in use" error message
        //allows the server to have multiple sockets to bind
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        //if binding the listener to its
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    // If we got here, it means we didn't get bound
    if (p == NULL)
    {
        return -1;
    }

    freeaddrinfo(ai); // All done with this

    //Listen
    if (listen(listener, 10) == -1)
    {
        return -1;
    }

    return listener;
}
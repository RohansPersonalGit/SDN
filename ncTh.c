#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>  // for open
#include <unistd.h> // for close
#include "Thread.h"
#include "commonProto.h"
#define PORT "7799"
typedef struct fileDesc
{
  int inuse;
  int fd;
} fileDesc;
char client_message[1024];
char buffer[1024];
int inputSize;
int readDone = 1;
int writeDone = 1;
int threadcount = 0;
int connection = 0;
int k = 0;
pthread_t tid[10];
struct fileDesc **fds;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void *handleWrite(void *arg)

{
  int retu = 0;
  int inputs;
  inputs = read(0, client_message, sizeof client_message);
  int clientSocket = *((int *)arg);
  if (inputs > 0)
  {
    retu = send(clientSocket, client_message, sizeof client_message, 0);
    if (retu == -1)
    {
      perror("send");
    }
  }
  memset(client_message, 0, sizeof(client_message));
  writeDone = 1;
  pthread_join(pthread_self(), NULL);
}

void *handleRead(void *arg)
{
  int clientSocket = *((int *)arg);
  int ret;

  if ((ret = recv(clientSocket, buffer, sizeof buffer, 0)) == -1)
  {
    printf("neg 1");
    perror("recv");
    connection = 0;
    close(clientSocket);
  }
  else if (ret > 0)
  {

    fprintf(stdout, buffer);
  }
  else if (ret == 0)
  {
    printf("set connection to 90");
    close(clientSocket);
    printf("after close");
    connection = 0;
  }
  memset(buffer, 0, sizeof(buffer));
  readDone = 1;
  pthread_join(pthread_self(), NULL);
}
void *serverThread(void *arg)
{
  printf("creating a client server new thread\n");
  int newSocket = *((int *)arg);
  int ret;
  pthread_t write;
  pthread_t readt;
  connection = 1;
  while (1)
  {
    if (connection)
    {
      if (writeDone)
      {
        writeDone = 0;
        if (pthread_create(&write, NULL, handleWrite, &newSocket) != 0)
        {

          printf("Failed to create thread\n");
        }
      }
      if (readDone)
      {
        readDone = 0;
        if (pthread_create(&readt, NULL, handleRead, &newSocket) != 0)
        {
          printf("Failed to create thread\n");
        }
      }
    }
    else
    {
      printf("befrore join");
      pthread_join(pthread_self(), NULL);
      free(fds[threadcount]);
      threadcount--;
      if (!k)
      {
        exit(1);
      }
      else
      {
        break;
      }
      {
      }
    }
  }
}

int main(int argc, char **argv)
{
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc

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

  if (cmdOps.option_l & cmdOps.option_k)
  {
    printf("it is an error to use -k with -l");
    exit(1);
  }

  fds = malloc(10 * sizeof(struct fileDesc));
  k = cmdOps.option_k;
  int listener, newSocket;
  struct sockaddr_in serverAddr;
  struct sockaddr_storage clientAddr;
  socklen_t addrlen;
  //Create the socket.
  serverAddr.sin_family = AF_INET;
  fflush(stdout);
  int clientSocket;
  int ret;

  pthread_t client;
  if (!cmdOps.option_l)
  {

    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0)
    {
      fprintf(stderr, "getaddrinfo this is gay\n");
    }
    fprintf(stdout, "before for loop\n");
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
      if ((newSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      {
        perror("claisdfajsd");
        continue;
      }
      printf("iteration\n");
      if (connect(newSocket, p->ai_addr, p->ai_addrlen) == -1)
      {
        perror("asdf\n");

        close(newSocket);
        perror("asdf\n");
        continue;
      }
      connection = 1;
      if (pthread_create(&client, NULL, serverThread, &newSocket) != 0)
      {
        printf("Failed to create thread\n");
      }
      printf("afterconnection\n");
      break;
    }
    if (p == NULL)
    {
      printf("thisis gay\n");
    }
    printf("beforethreadcreate\n");
    while (recv(newSocket, buffer, sizeof buffer, 0) != -1)
    {
      if (pthread_create(&client, NULL, serverThread, &newSocket) != 0)
      {
        printf("Failed to create thread\n");
      }
    }
  }
  else
  {

    listener = get_listener_socket(PORT);
    // Configure settings of the server address struct
    // Address family = Internet
    //Set port number, using htons function to use proper byte order
    serverAddr.sin_port = htons(7799);
    //Set IP address to localhost
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //Set all bits of the padding field to 0
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    //Bind the address struct to the socket
    bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    //Listen on the socket, with 40 max connection requests queued
    if (listen(listener, 10) == 0)
    {
      printf("Listening\n");
    }
    else
    {
      printf("Error\n");
    }
    int i = 0;
    while (1)
    {

      addrlen = sizeof clientAddr;
      newSocket = accept(listener, (struct sockaddr *)&clientAddr, &addrlen);
      threadcount++;
      printf("back here");
      fds[threadcount] = calloc(1, sizeof(struct fileDesc));
      fds[threadcount]->fd = newSocket;
      if (pthread_create(&tid[threadcount], NULL, serverThread, &newSocket) != 0)
      {
        printf("Failed to create thread\n");
      }
    }
  }
  return 0;
}
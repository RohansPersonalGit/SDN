#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
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
int r = 0;
pthread_t tid[10];
struct fileDesc **fds;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void *handleWrite(void *arg)

{
  int clientSocket = *((int *)arg);
  int retu = 0;
  int inputs;
  while ((inputs = (read(STDIN_FILENO, client_message, sizeof client_message)) > 0) &&
         connection==1)
  {
    if (inputs == -1)
    {
      perror("read stdin");
      memset(client_message, 0, sizeof(client_message));
      break;
    }
    (retu = send(clientSocket, client_message, sizeof client_message, 0));
    if (retu == -1)
    {
      perror("send");
      memset(client_message, 0, sizeof(client_message));
      break;
    }
    if (r)
    {
      for (int x = 0; x < threadcount; x++)
      {
        if (clientSocket != fds[x]->fd && fds[x]->inuse)
        {
          int output = send(fds[x]->fd, client_message, sizeof client_message, 0);
          if (output == -1)
          {
            perror("send");
          }
        }
      }
    }
    memset(client_message, 0, sizeof(client_message));
  }
  memset(client_message, 0, sizeof(client_message));
}

void *handleRead(void *arg)
{
  int clientSocket = *((int *)arg);
  int ret;

  while (ret = (read(clientSocket, buffer, sizeof buffer) > 0)&& connection ==1)
  {
    if (ret == -1)
    {
      perror("read");
      memset(buffer, 0, sizeof(buffer));
      connection = 0;
      close(clientSocket); 
    }
    fprintf(stdout, buffer);
    if (r)
    {
      for (int x = 0; x < threadcount; x++)
      {
        if (clientSocket != fds[x]->fd && fds[x]->inuse)
        {
          int output = send(fds[x]->fd, buffer, sizeof buffer, 0);
          if (output == -1)
          {
            perror("send");
          }
        }
      }
    }
    memset(buffer, 0, sizeof(buffer));
  }
  connection = 0;
}
void *serverThread(void *arg)
{
  int newSocket = *((int *)arg);
  int ret;
  pthread_t write;
  pthread_t readt;
  connection = 1;
  while (connection | k)
  {
    if (pthread_create(&write, NULL, handleWrite, &newSocket) != 0)
    {
      printf("failed to create write thread\n");
    }
    if (pthread_create(&readt, NULL, handleRead, &newSocket) != 0)
    {
      printf("failed to create read thread\n");
    }
    pthread_join(readt, NULL);
    pthread_join(write, NULL);
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
    char port[256];
    sprintf(port, "%d", cmdOps.port);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char remoteP[sizeof(int)];
    sprintf(remoteP, "%d", cmdOps.source_port);
    if ((rv = getaddrinfo("localhost", port, &hints, &servinfo)) != 0)
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
      r = validateInputs(cmdOps, &newSocket);
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
    pthread_create(&client, NULL, serverThread, &newSocket);
    pthread_join(client, NULL);
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

      if (threadcount > 10)
      {
        for (int i = 0; i < 10; i++)
        {
          pthread_join(tid[i], NULL);
          threadcount--;
        }
      }
      addrlen = sizeof clientAddr;
      newSocket = accept(listener, (struct sockaddr *)&clientAddr, &addrlen);
      r = validateInputs(cmdOps, &newSocket);
      if(k){
        
      }
      fds[threadcount] = calloc(1, sizeof(struct fileDesc));
      if (pthread_create(&tid[threadcount], NULL, serverThread, &newSocket) != 0)
      {
        printf("Failed to create thread\n");
      }
      else
      {
        fds[threadcount]->inuse = 1;
        fds[threadcount]->fd = newSocket;
        threadcount++;
        if(k==0){
          pthread_join(tid[k], NULL); 
          return 0; 
          exit(1);
        }
      }
    }
  }
  return 0;
}
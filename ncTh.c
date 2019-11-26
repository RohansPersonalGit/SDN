#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "commonProto.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PORT "9022" // Port we're listening on
#define STDIN 0
struct fileThread
{
  int inuse;
  int fd;
};

void sigchld_handler(int s)
{
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

int main(int argc, char **argv)
{
  //Me trying to use a listener;

  int listener;
  int newfd;
  char buf[256];
  struct sockaddr_storage clientAddr;
  socklen_t addrlen;
  //the bellow are for using select
  struct timeval tv;
  fd_set readfds;

  tv.tv_sec = 15;
  tv.tv_usec = 500000;

  char clientIp[INET6_ADDRSTRLEN];
  int fd_size = 10;
  struct fileThread *fds = malloc(sizeof *fds * fd_size);

  listener = get_listener_socket(PORT);

  if (listener == -1)
  {
    fprintf(stderr, "error gettin listening socket\n");
    exit(1);
  }

  int fd_count = 0;
  fds[0].inuse = 1;
  fds[0].fd = listener;
  FD_ZERO(&readfds);
  FD_SET(0, &readfds);
  while(1)
  {
    int retval;

    if ((retval = select(fd_count + 1, NULL, &readfds, NULL, &tv)) < 0)
    {
      fprintf(stderr, "issue with this shit for %i\n");
    }
    for (int i = 0; i < fd_count+1; ++i)
    {
      if (FD_ISSET(i, &readfds))
      {
        if (i == 0)
        {
          fprintf(stderr,"i does == 0");
          addrlen = sizeof clientAddr;
          newfd = accept(listener, (struct sockaddr *)&clientAddr, &addrlen);
          if (newfd == -1)
          {
            fprintf(stderr,"Accept error");
          }
          else
          {
                      fprintf(stdout, "new client"); 
            fd_count++;
            int ret = recv(i,buf,sizeof buf, 0|0|0);
            if(ret<0){
              printf("You entered: %d", fd_count); 
            } 
            fprintf(stdout,buf); 
            FD_SET(fd_count, &readfds);
          }
        }
      }else{
                  fprintf(stderr,"not a new client");
                    int ret = recv(i,buf,sizeof buf, 0|0|0);
                    if(ret<0){
                      continue; 
                    }
                    fprintf(stdout,buf); 

      }
    }
  }


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
}

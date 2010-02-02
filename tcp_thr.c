/* Measure throughput of IPC using tcp sockets */

/* Needs more work, only works for message size 1. 
 * TODO: implement message framing
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <netdb.h>
#include <netinet/tcp.h>

int main(int argc, char *argv[])
{
  int size;
  char *buf;
  int64_t count, i, delta;
  struct timespec start, stop;


  int yes = 1;
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints;
  struct addrinfo *res;
  int sockfd, new_fd;

  if (argc != 3) {
    printf ("usage: tcp_thr <message-size> <message-count>\n");
    return 1;
  }

  size = atoi(argv[1]);
  count = atol(argv[2]);

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  getaddrinfo("127.0.0.1", "3491", &hints, &res);

  printf("message size: %i octets\n", size);
  printf("message count: %lli\n", count);

  if (!fork()) {
    /* child */

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
      perror("socket");
      exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    } 
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
      perror("bind");
      exit(1);
    }
    
    if (listen(sockfd, 1) == -1) {
      perror("listen");
      exit(1);
    } 
    
    addr_size = sizeof their_addr;
    
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
      perror("accept");
      exit(1);
    } 

    for (i = 0; i < count; i++) {      
      if (read(new_fd, buf, size) != size) {
        perror("read");
        exit(1);
      }
    }
  } else { 
    /* parent */

    sleep(1);
    
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
      perror("socket");
      exit(1);
    }
    
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
      perror("connect");
      exit(1);
    }
    
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < count; i++) {
      if (write(sockfd, buf, size) != size) {
        perror("write");
        exit(1);
      }      
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    
    delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1e9 +
	     stop.tv_nsec - start.tv_nsec);
    
    printf("average throughput: %lli msg/s\n", (count * (int64_t) 1e9) / delta);
    printf("average throughput: %lli Mb/s\n", (((count * (int64_t) 1e9) / delta) * size * 8) / (int64_t) 1e6);
  }
  
  return 0;
}

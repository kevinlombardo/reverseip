/*
 * ** listener.c -- a datagram sockets "server" demo
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define MYPORT 5001    // the port users will be connecting to
#define MAXBUFLEN 300

struct Arecord{
	short name;
	short type;
	short class;
	short ttl;
	short datalength;
	int address;
};

void sigproc();
void sigalrm();
FILE *file;
int count;

int main(int argc, char *argv[]){

    signal(SIGINT, sigproc);
    signal(SIGALRM, sigalrm);
    int sockfd;
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    struct sockaddr_in raddr;      // resolved address
    socklen_t addr_len;
    int numbytes;
    char buf[MAXBUFLEN];
    char host[255];
    int offset, hostlength, ii, i;
    short type;
    struct Arecord *r;

    int port = atoi(argv[1]);
//    FILE *file;
    char *filename = argv[2];
    file = fopen(filename, "w");

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if (bind(sockfd, (struct sockaddr *)&my_addr,
        sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

    count = 0;

    // START LISTENING LOOP
    while(1){

      if ((numbytes=recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
      }

//printf("got packet from %s\n",inet_ntoa(their_addr.sin_addr));
//printf("packet is %d bytes long\n",numbytes);
//buf[numbytes] = '\0';
//printf("packet contains \"%s\"\n",buf);

      hostlength = buf[12];

      // get hostname
      for(i=0;i<hostlength+4;i++){
        if(buf[i+13] == 3){
          host[i] = '.';
        } else {
          host[i] = buf[i+13];
        }
      }
      host[i+1] = '\0';

// printf("host is %s, length is %d\n", host, hostlength);
//sprintf(sql, "INSERT INTO host2(hostname, ipaddress) VALUES(\'%s\',", host);
      type = 1;
      offset = 12+1+hostlength+9;

      while(type == 1){
        r = (struct Arecord *)(buf + offset);
        type = ntohs(r->type);
        if(type == 1){
          raddr.sin_addr.s_addr = r->address;
//printf("%s\n", inet_ntoa(raddr.sin_addr));
          fprintf(file, "INSERT INTO host2(hostname, ipaddress) VALUES(\'%s\',\'%s\');\n", host, inet_ntoa(raddr.sin_addr));
          //printf("INSERT INTO host2(hostname, ipaddress) VALUES(\'%s\',\'%s\');\n", host, inet_ntoa(raddr.sin_addr));
        }
        offset += 16;
      }
      fflush(file);
      /* count++;
      if(count % 1000 == 0)
        fprintf(stderr, "%d received\n", count);
      */
      memset(host, 0, 255);
      memset(buf, 0, 300);
    } // END LOOP

    close(sockfd);
    return 0;
} 

void sigalrm(){
	signal(SIGALRM, sigalrm);
	fflush(file);
	fprintf(stderr, "file flushed, count at %d\n", count);
}

void sigproc(){
	signal(SIGINT, sigproc);
	fflush(file);
	fclose(file);
	printf("exiting...\n");
	exit(0);
}

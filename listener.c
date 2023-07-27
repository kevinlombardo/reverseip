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
	unsigned short name;
	unsigned short type;
	unsigned short class;
	short ttl;
        short blah;
	unsigned short datalength;
	unsigned int address;
};

void sigproc();
void sigalrm();
FILE *file;
FILE *dfile;
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
    char host2[255];
    int offset, ii, i;
    unsigned short type;
    unsigned short datalength;
    short answers;
    struct Arecord *r;

    int port = atoi(argv[1]);
//    FILE *file;
    char *filename = argv[2];
    char *deletefile = argv[3];

    file = fopen(filename, "w");
    dfile = fopen(deletefile, "w");

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
     
      int dlength, hc; 

      //skip DNS header
      // get hostname
      sprintf(host2, "%s", buf+12);

      //parse hostname
      dlength = host2[0];
      int hostlength = 0;
 
      while(dlength > 0){
        //read thru current domain
        for(hc=0;hc<dlength;hc++,hostlength++){
          host[hostlength] = host2[hostlength+1];
        }
        //hostlength++;
        //printf("hl = %i\n", hostlength);
        dlength = host2[hostlength+1]; 
        if(dlength != 0){ 
          host[hostlength] = '.';
        }
        hostlength++;
      }
      host[hostlength] = '\0';
//printf("%s\n", host);

      type = 1;
      answers = 0;
      offset = 12+hostlength+5;

      while(type == 1 || type == 5){
        r = (struct Arecord *)(buf + offset);
//printf("%x %x %x %x %x %x %x\n", r->name, r->type, r->class, r->ttl, r->blah, r->datalength, r->address);

        type = ntohs(r->type);
//printf("type=%d\n", type);
        if(type == 1){
          raddr.sin_addr.s_addr = r->address;
          fprintf(file, "%s.%u\n", host, htonl(raddr.sin_addr.s_addr));
	  if(answers == 0){
            fprintf(dfile, "DELETE FROM host WHERE domain = \'%s\';\n", host);
	    answers++;
	  }
        }
        if(type == 5){
          datalength = ntohs(r->datalength);

//printf("%x %x %x %x %x %x\n", r->name, r->type, r->class, r->ttl, r->datalength, r->address);
          offset += 12 + datalength;
          //offset += 14;
        } else {
          offset += 16;
        }
      }
      fflush(file);
      fflush(dfile);
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
        fclose(dfile);
	printf("exiting...\n");
	exit(0);
}

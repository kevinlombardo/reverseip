#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#define __FAVOR_BSD
#include <netinet/udp.h>
#include <netinet/ip.h>

#define SRCA "192.168.1.10"
#define SRCP 5001
#define DSTP 53

struct query {
	char *name;
	short int type;
	short int class;
};

struct dnsquery {
	u_int16_t tid;
	u_int16_t flags;
	u_int16_t questions;
	u_int16_t answerRRs;
	u_int16_t authorityRRs;
	u_int16_t additionalRRs;
};

unsigned short csum (unsigned short *buf, int nwords)
{
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

int main(int argc, char *argv[]){
  
  char *filename = argv[1];
  char *ns = argv[2];
  char *source = argv[3];
  int daemons = atoi(argv[4]);
  int throttle = atoi(argv[5]);
  int sport = 5001;

  FILE *file;
  if((file = fopen(filename, "r")) == NULL){
    fprintf(stderr, "Cannot open %s\n", filename);
  }

  int s = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
  char datagram[256];
  struct ip *iph = (struct ip *) datagram;
  struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct ip));
  struct dnsquery *dns = (struct dnsquery *)(datagram + sizeof(struct udphdr) + sizeof(struct ip));
  char *query = (char *)(datagram + sizeof(struct dnsquery) + sizeof(struct udphdr) + sizeof(struct ip));
  struct sockaddr_in sin;

  char line[300];
  char host[255];
  char dhost[256];
  char ip[15];
  char len[2];

  //int sports[] = {5001, 5002, 5003, 5004, 5005};
  int sports[] = {5001, 5001, 5001, 5001, 5001};
  int sporti = 0;
  int count = 0;
  int errors = 0;

  while(fscanf(file, "%s %s", host, ip) != EOF){
    if(strncmp(ip, "\\N", 2) == 0){
      strcpy(ip, ns);
    }
    //parse host
    char tmphost[255];
    char check;
    int bb, bbb;
    int counter = 0;
    int dpos = 0;

    for(bb=0;bb<=strlen(host);bb++){
      if(host[bb] != '.' && host[bb] != '\0'){
        tmphost[counter] = host[bb];  
//printf("%c\n", host[bb]);
        counter++;
      } else {
        dhost[dpos] = counter;
//printf("%i %c\n", dpos, counter);
        for(bbb=0;bbb<counter;bbb++){
          dhost[dpos+bbb+1] = tmphost[bbb];
//printf("%i %c\n", dpos+bbb+1, dhost[dpos+bbb+1]);
        }
        dpos += counter + 1;
        counter = 0;
     }

    }
    dhost[strlen(host)+1] = '\0';
//printf("%s\n", dhost);
//exit(0); 
    //len[0] = strlen(host) - 4;
    //len[1] = '\0';
    //strcpy(dhost, len);
    //strcat(dhost,host);


    sin.sin_family = AF_INET;
    sin.sin_port = htons(sport);
    sin.sin_addr.s_addr = inet_addr(ip);
    memset(datagram, 0, 256);

    int udp_len = sizeof(struct udphdr) + sizeof(struct dnsquery) + strlen(dhost) + 5;
    int ip_len = sizeof(struct ip) + udp_len;

    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 0;
    iph->ip_len = ip_len;
    iph->ip_id = htonl(54321); /* value doesnt matter */
    iph->ip_off = 0;
    iph->ip_ttl = 255;
    iph->ip_p = 17;
    iph->ip_sum = 0;
    iph->ip_src.s_addr = inet_addr(source);
    iph->ip_dst.s_addr = sin.sin_addr.s_addr;

    udph->uh_sport = htons(sport);
    udph->uh_dport = htons(DSTP);
    udph->uh_ulen = htons(udp_len);
    udph->uh_sum = 0;

    dns->tid = htons(2);
    dns->flags = htons(256);
    dns->questions = htons(1);
    dns->answerRRs = 0;
    dns->authorityRRs = 0;
    dns->additionalRRs = 0;

    memcpy(query, dhost, strlen(dhost));
//printf("%s\n", dhost);
    short dlen = iph->ip_len;
    datagram[dlen-1] = 1;
    datagram[dlen-3] = 1;
//    datagram[dlen-9] = 3;

    iph->ip_sum = csum((unsigned short *) datagram, ip_len >> 1);

    int one = 1;
    const int *val = &one;
    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
      printf ("Warning: Cannot set HDRINCL!\n");

    if(sendto(s,
		datagram,
 		iph->ip_len,
		0,
		(struct sockaddr *) &sin,
		sizeof(sin)) < 0)
	errors++;
	//printf("error\n");
    else
	//printf("%d\n", count);
    count++;

    sporti++;
    sport++;
    if(sport == (5001 + daemons)){
      sport = 5001;
    }

    if(sporti == 5)
      sporti = 0;
    if(count % (throttle * daemons)  == 0){
      sleep(1);
    }
    if(count % 1000 == 0)
      printf("%d\n", count);

  }
  printf("%d errors.\n", errors);
  return 0;
}

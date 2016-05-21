/*
 * router1.c
 *
 *  Created on: Sep 17, 2014
 *      Author: Nguyen Tran
 */

#include "router1.h"
#include "proxy.h"
#include <linux/icmp.h>
#include <linux/ip.h>
#include <arpa/inet.h>

/*
 * communicate back to proxy for stage 1 assignment
 */
void communicateToServerStage1(int portNum, int numRouter)
{
	int sockfd, data, pid,serPort;
	socklen_t serlen;
	pid = getpid(); //get process pid

	char *buffer;

	struct payload {
		int router;
		int pid;
	};
	struct payload *p = (struct payload*) malloc(sizeof(struct payload));
	p->pid = pid;
	p->router = numRouter;


	struct sockaddr_in serv_addr, cli_addr;

	//create socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0); //use UDP
	//check for error
	if(sockfd < 0)
	{
		error ("ERROR creating socket in server...");
	}

	//initialize all fields in serv_addr to 0
	memset((char*) &serv_addr, 0 ,sizeof(serv_addr));
	//set fields in struct sockaddr_in
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(0); //Bind to any port
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind the socket to an addr
	if(bind(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR binding to socket in server...");
	}

	/*
	 * Set up cli addr
	 */
	//initialize all fields in serv_addr to 0
	memset((char*) &cli_addr, 0 ,sizeof(cli_addr));
	//set fields in struct sockaddr_in
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(portNum); //Bind to port provided by proxy
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	buffer = (char*)p;


	printf("ROUTER SENDS DATA TO PROXY...\n");
	data = sendto(sockfd,buffer,sizeof(struct payload),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
	if(data < 0)
	{
		printf ("ERROR sending message from router number %d\n",numRouter);
	}

	serlen = sizeof(serv_addr);
	getsockname(sockfd,(struct sockaddr*) &serv_addr, &serlen);
	serPort = ntohs(serv_addr.sin_port);

    writeInfoToFileRouterStage1(numRouter,pid,serPort);
    free(p);
}

/*
 * communicate back to proxy for stage 2 assignment
 */
void communicateToServerStage2(int portNum, int numRouter)
{
	FILE *fp = fopen("stage2.router.out","w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}

	struct timeval tv; //time interval wait for select()
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int maxfds1;

	int sockfd, data, pid,serPort;
	socklen_t serlen,clilen;
	pid = getpid(); //get process pid

	struct icmphdr* icmp;
	struct iphdr* ip;
	//struct icmp_filter* icmp_data;
	char saddr[16];
	char daddr[16];

	char *buffer;
	char icmp_buffer [2048];

	char reply_buf [2048];

	struct iphdr* ip_reply;
	struct icmphdr* icmp_reply;



	struct payload {
		int router;
		int pid;
	};
	struct payload *p = (struct payload*) malloc(sizeof(struct payload));
	p->pid = pid;
	p->router = numRouter;


	struct sockaddr_in serv_addr, cli_addr;

	//create socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0); //use UDP
	//check for error
	if(sockfd < 0)
	{
		error ("ERROR creating socket in server...");
	}

	//initialize all fields in serv_addr to 0
	memset((char*) &serv_addr, 0 ,sizeof(serv_addr));
	//set fields in struct sockaddr_in
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(0); //Bind to any port
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind the socket to an addr
	if(bind(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR binding to socket in server...");
	}

	/*
	 * Set up cli addr
	 */
	//initialize all fields in serv_addr to 0
	memset((char*) &cli_addr, 0 ,sizeof(cli_addr));
	//set fields in struct sockaddr_in
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(portNum); //Bind to port provided by proxy
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	buffer = (char*)p;


	printf("ROUTER SENDS DATA TO PROXY\n");
	data = sendto(sockfd,buffer,sizeof(struct payload),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
	if(data < 0)
	{
		printf ("ERROR sending message from router number %d\n",numRouter);
	}

	clilen = sizeof(cli_addr);
	serlen = sizeof(serv_addr);
	getsockname(sockfd,(struct sockaddr*) &serv_addr, &serlen);
	serPort = ntohs(serv_addr.sin_port);

	free(p);
	writeInfoToFileRouterStage2Part1(fp,numRouter,pid,serPort);

	maxfds1 = sockfd +1;
	while(1)
	{
	    int sel_return;
	    fd_set readfds;

	    FD_ZERO(&readfds); //initialize the set
	    FD_SET(sockfd, &readfds); //add sockfd to fd_set

	    /*
	     * Waiting for connection from proxy. If after 5 seconds with no communication, function exits
	     * and terminates this child process
	     */
		sel_return = select(maxfds1,&readfds,NULL,NULL,&tv);
		if(sel_return < 0)
		{
			printf("ERROR in selecting \n");
			exit(1);
		}
		else if(sel_return == 0)
		{
			printf("After 5 seconds without any communication. Function exits, router number %d is terminated\n",numRouter);
			break;
		}

		if(FD_ISSET(sockfd, &readfds))
		{
			/*
			 * reset timeout
			 */
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			data = recvfrom(sockfd,icmp_buffer,sizeof(icmp_buffer),0,(struct sockaddr*) &cli_addr, &clilen);
			if(data < 0)
			{
				printf ("ERROR receiving icmp echo message from proxy\n");
				exit(1);
			}

			/*
			 * ip header info
			 */
			ip = (struct iphdr*) icmp_buffer;
			inet_ntop(AF_INET,&ip->saddr,saddr,sizeof(saddr));
			inet_ntop(AF_INET,&ip->daddr,daddr,sizeof(daddr));

			/*
			 * icmp header info
			 */
			icmp = (struct icmphdr*) (icmp_buffer+sizeof(struct iphdr));

			/*
			 * write to file
			 */
			writeInfoToFileRouterStage2Part2(fp,portNum,saddr,daddr,icmp->type);

			/*
			 * Generate ICMP REPLY packet
			 */
			for(int i=0; i< (int) sizeof(icmp_buffer); i++)
			{
				reply_buf[i] = icmp_buffer[i];
			}

			ip_reply = (struct iphdr*)reply_buf;
			icmp_reply = (struct icmphdr*) (reply_buf + sizeof(struct iphdr));


			//calculate ID for IP header reply
			int id_reply = ntohs(ip->id);
			id_reply++;

			ip_reply->ihl = ip->ihl;
			ip_reply->version = ip->version;
			ip_reply->tos = ip->tos;
			ip_reply->tot_len = ip->tot_len;
			ip_reply->id = htons(id_reply);
			ip_reply->frag_off = 0;
			ip_reply->ttl = ip->ttl;
			ip_reply->protocol = ip->protocol;
			ip_reply->saddr = ip->daddr;
			ip_reply->daddr = ip->saddr;


			icmp_reply->type = 0;
			icmp_reply->code = 0;
			icmp_reply->un.echo.id = icmp->un.echo.id;
			icmp_reply->un.echo.sequence = icmp->un.echo.sequence;
			icmp_reply->checksum = 0;

			//in_cksum is a reused code from Mike Muuss. For full citation, please see the function definition at the end
			icmp_reply->checksum = in_cksum((unsigned short*)icmp_reply, sizeof(struct icmphdr) + icmp->un.frag.mtu);

			ip_reply->check = 0;
			//in_cksum is a reused code from Mike Muuss. For full citation, please see the function definition at the end
			ip_reply->check = in_cksum((unsigned short*)ip_reply,sizeof(struct iphdr));

			/*
			 * ready to send this reply_buf to proxy via UDP
			 */
			printf("SENDING ICMP REPLY PACKET TO PROXY from router number %d\n", numRouter);
			data = sendto(sockfd,reply_buf,ntohs(ip_reply->tot_len),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
			if(data < 0)
			{
				printf ("ERROR sending message from router number %d\n",numRouter);
			}
		}
	}
    	fclose(fp);
}

/*
 * write to file for stage 1
 */
void writeInfoToFileRouterStage1(int router,int pidNum,int serPort)
{
	FILE *fp = fopen("stage1.router.out","w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}
	fprintf(fp,"router: %d, pid: %d, port: %d\n",router,pidNum,serPort);
	fclose(fp);
}

/*
 * Write to file for stage 2 part 2
 */
void writeInfoToFileRouterStage2Part2(FILE *fp1,int cliPort,char* sourceAddr,char* destAddr,int type)
{
	fprintf(fp1,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",cliPort,sourceAddr,destAddr,type);
}

/*
 * write to file for stage 2 part 1
 */
void writeInfoToFileRouterStage2Part1(FILE *fp1,int router, int pidNum,int serPort)
{
	fprintf(fp1,"router: %d, pid: %d, port: %d\n",router,pidNum,serPort);
}

/*
 * in_cksum --
 * Checksum routine for Internet Protocol
 * family headers (C Version)
 * CODE RE-USE FROM MIKE MUUSS WHO IS THE AUTHOR OF PING.C
 * SOURCE: https://www.cs.utah.edu/~swalton/listings/sockets/programs/part4/chap18/ping.c
 */
unsigned short in_cksum(unsigned short *addr, int len)
{
    register int sum = 0;
    u_short answer = 0;
    register u_short *w = addr;
    register int nleft = len;
    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
      sum += *w++;
      nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1)
    {
      *(u_char *) (&answer) = *(u_char *) w;
      sum += answer;
    }
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);       /* add hi 16 to low 16 */
    sum += (sum >> 16);               /* add carry */
    answer = ~sum;              /* truncate to 16 bits */
    return (answer);
}

/*
 * router1.c
 *
 *  Created on: Sep 17, 2014
 *      Author: Nguyen Tran
 */

#include "router1.h"
#include "proxy.h"
#include "aes.h"
#include "controlMessage.h"
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
	char prependstr[] = "stage2.router";
	char postpendstr[] = ".out";
	char fileName[20];
	sprintf(fileName, "%s%d%s",prependstr,numRouter,postpendstr);

	FILE *fp = fopen(fileName,"w");
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
			writeInfoToFileRouterStage2Part2(fp,"port",portNum,saddr,daddr,icmp->type);

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
 * communicate to the outside world for stage 3
 */
void communicateToServerStage3(int portNum, int numRouter, char* eth_ip)
{
	char prependstr[] = "stage3.router";
	char postpendstr[] = ".out";
	char fileName[20];
	sprintf(fileName, "%s%d%s",prependstr,numRouter,postpendstr);

	FILE *fp = fopen(fileName,"w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}

	/*
	 * Necessary struct and constant declaration for sendmsg() - sending raw socket out to the world
	 */
	char orig_src_addr[128];
	struct iovec io;
	struct msghdr msgh;
	memset(&msgh,0,sizeof(msgh));
	memset(&io,0,sizeof(io));


	struct timeval tv; //time interval wait for select()
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int maxfds1;

	int sockfd, data, pid,serPort;
	socklen_t serlen,clilen,rawlen;
	pid = getpid(); //get process pid

	struct icmphdr* icmp;
	struct iphdr* ip;
	//struct icmp_filter* icmp_data;
	char saddr[16];
	char daddr[16];

	char *buffer;
	char icmp_buffer [2048];

	char reply_buf [2048];
	char send_raw_buf[2048];
	char rcv_raw_buf [2048];

	struct iphdr* ip_reply;
	struct icmphdr* icmp_reply;

	struct iphdr* ip_raw;
	struct icmphdr* icmp_raw;

	struct payload {
		int router;
		int pid;
	};
	struct payload *p = (struct payload*) malloc(sizeof(struct payload));
	p->pid = pid;
	p->router = numRouter;


	struct sockaddr_in serv_addr, cli_addr, eth_addr, dest_addr;

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

	/*
	 * Prepare raw socket to talk to outside world
	 */
	int raw_socket = socket(AF_INET, SOCK_RAW,IPPROTO_ICMP);
	//check for error
	if(raw_socket < 0)
	{
		error ("ERROR creating raw socket...\n");
	}

	//initialize all fields in eth_addr to 0
	memset((char*) &eth_addr, 0 ,sizeof(eth_addr));
	//set fields in struct sockaddr_in
	eth_addr.sin_family = AF_INET;
	eth_addr.sin_port = htons(0); //Bind to any port
	//eth_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int error_check;
	error_check = inet_pton(AF_INET,eth_ip, &(eth_addr.sin_addr));
	if(error_check < 0)
	{
		error ("Error in converting string IPv4 to network byte order using inet_pton()\n");
	}
	else if(error_check == 0)
	{
		error("Invalid IP addr is pased in inet_pton() function\n");
	}

	//bind the raw_socket to an addr
	if(bind(raw_socket,(struct sockaddr*) &eth_addr,sizeof(eth_addr)) < 0)
	{
		error("ERROR binding to raw socket...\n");
	}
	rawlen = sizeof(eth_addr);

	if(sockfd > raw_socket)
	{
		maxfds1 = sockfd +1;
	}
	else
	{
		maxfds1 = raw_socket + 1;
	}

	while(1)
	{
	    int sel_return;
	    fd_set readfds;

	    FD_ZERO(&readfds); //initialize the set
	    FD_SET(sockfd, &readfds); //add sockfd to fd_set
	    FD_SET(raw_socket, &readfds); //add raw_socket to fd_set
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
			printf("RECEIVING ICMP REQUEST PACKET FROM PROXY IN ROUTER NUMBER %d\n", numRouter);
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
			writeInfoToFileRouterStage2Part2(fp,"port",portNum,saddr,daddr,icmp->type);

			/*
			 * Check to see if the dest addr is router itself or else where
			 * If it is addressed to itself, it should response as in stage 2
			 * If it is for some where else, it should change src IP addr to itself and sent it out using raw_socket
			 */
			if(strncmp (daddr,eth_ip,sizeof(daddr)) == 0)
			{
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
			/*
			 * ICMP packet is addressed to some where else
			 */
			else
			{
				/*
				 * Need to change source IP addr to this router IP addr
				 */
				for(int i=0; i< (int) sizeof(icmp_buffer); i++)
				{
					send_raw_buf[i] = icmp_buffer[i];
				}

				ip_raw = (struct iphdr*)send_raw_buf;
				icmp_raw = (struct icmphdr*) (send_raw_buf + sizeof(struct iphdr));


				//Before change the src addr, store it
				if(inet_ntop(AF_INET, &ip_raw->saddr,orig_src_addr,sizeof(orig_src_addr)) == NULL)
				{
					error ("ERROR saving original src\n");
				}

				//change src addr to this router ip addr
				error_check = inet_pton(AF_INET,eth_ip, &ip_raw->saddr);

				if(error_check < 0)
				{
					error ("Error in converting string IPv4 to network byte order using inet_pton() before sending to outside world\n");
				}
				else if(error_check == 0)
				{
					error("Invalid IP addr is pased in inet_pton() function before sending to outside world\n");
				}


				//initialize all fields in serv_addr to 0
				memset(&dest_addr, 0 ,sizeof(dest_addr));
				//set fields in struct sockaddr_in
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_port = htons(0);
				dest_addr.sin_addr.s_addr = ip_raw->daddr;

				socklen_t destlen;
				destlen = sizeof(dest_addr);

				/*
				 * construct struct msghdr to pass in sendmsg()
				 */
				msgh.msg_name = &dest_addr;
				msgh.msg_namelen = destlen;

				msgh.msg_iovlen = 1;
				msgh.msg_iov = &io;
				msgh.msg_iov->iov_base = icmp_raw;
				msgh.msg_iov->iov_len = ntohs(ip_raw->tot_len) - sizeof(struct iphdr);
				msgh.msg_control = NULL;
				msgh.msg_controllen = 0;
				msgh.msg_flags = 0;


				printf("SENDING DATA TO OUTSIDE WORLD THROUGH RAW INTERFACE from router number %d\n", numRouter);
				data = sendmsg(raw_socket, &msgh,0);
				if(data < 0)
				{
					printf("ERROR sending raw packet to outside world from router number %d\n", numRouter);
					exit(-1);
				}
			}
		}
		if(FD_ISSET(raw_socket, &readfds))
		{
			/*
			 * reset timeout
			 */
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			/*
			 * Receive data from raw interface
			 */
			printf("RECEIVING DATA FROM RAW INTERFACE...\n");
			data = recvfrom(raw_socket,rcv_raw_buf,sizeof(rcv_raw_buf),0,(struct sockaddr*) &eth_addr, &rawlen);
			if(data < 0)
			{
				printf ("ERROR receiving icmp echo message from outside world\n");
				exit(1);
			}

			/*
			 * Re-send ICMP ECHO packet
			 * Need to change dest IP addr to the original src ip addr
			 */
			for(int i=0; i< (int) sizeof(rcv_raw_buf); i++)
			{
				send_raw_buf[i] = rcv_raw_buf[i];
			}
			ip_raw = (struct iphdr*)send_raw_buf;
			inet_ntop(AF_INET,&ip_raw->saddr,saddr,sizeof(saddr));
			inet_ntop(AF_INET,&ip_raw->daddr,daddr,sizeof(daddr));

			icmp_raw = (struct icmphdr*) (send_raw_buf + sizeof(struct iphdr));

			/*
			 * Check if dest addr is addressed to this router
			 */
			if(strncmp(daddr,eth_ip,sizeof(daddr)) == 0)
			{
				/*
				 * write to file
				 */
				writeInfoToFileRouterStage2Part2(fp,"raw",portNum,saddr,daddr,icmp_raw->type);

				//change dest addr to the original src addr
				error_check = inet_pton(AF_INET,orig_src_addr, &ip_raw->daddr);
				if(error_check < 0)
				{
					error ("Error in converting string IPv4 to network byte order using inet_pton() before sending back to proxy\n");
				}
				else if(error_check == 0)
				{
					error("Invalid IP addr is pased in inet_pton() function before sending back to proxy\n");
				}

				/*
				 * Send ICMP ECHO request back to proxy
				 */
				printf("SENDING ICMP REPLY PACKET TO PROXY from router number %d\n", numRouter);
				data = sendto(sockfd,send_raw_buf,ntohs(ip_raw->tot_len),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending message from router number %d\n",numRouter);
				}
			}
		}
	}
    	fclose(fp);
    	close(sockfd);
    	close(raw_socket);
}

/*
 * communicate to the outside world for stage 4
 */
void communicateToServerStage4(int portNum, int numRouter, char* eth_ip)
{
	char prependstr[] = "stage4.router";
	char postpendstr[] = ".out";
	char fileName[20];
	sprintf(fileName, "%s%d%s",prependstr,numRouter,postpendstr);

	FILE *fp = fopen(fileName,"w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}

	/*
	 * Necessary struct and constant declaration for sendmsg() - sending raw socket out to the world
	 */
	char orig_src_addr[128];
	struct iovec io;
	struct msghdr msgh;
	memset(&msgh,0,sizeof(msgh));
	memset(&io,0,sizeof(io));


	struct timeval tv; //time interval wait for select()
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int maxfds1;

	int sockfd, data, pid,serPort;
	socklen_t serlen,clilen,rawlen;
	pid = getpid(); //get process pid

	struct icmphdr* icmp;
	struct iphdr* ip;
	//struct icmp_filter* icmp_data;
	char saddr[16];
	char daddr[16];

	char *buffer;
	char icmp_buffer [2048];

	char reply_buf [2048];
	char send_raw_buf[2048];
	char rcv_raw_buf [2048];

	struct iphdr* ip_reply;
	struct icmphdr* icmp_reply;

	struct iphdr* ip_raw;
	struct icmphdr* icmp_raw;

	struct payload {
		int router;
		int pid;
	};
	struct payload *p = (struct payload*) malloc(sizeof(struct payload));
	p->pid = pid;
	p->router = numRouter;


	struct sockaddr_in serv_addr, cli_addr, eth_addr, dest_addr;

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
	printf ("IP ADDR of router serv_addr: %s\n",inet_ntoa(serv_addr.sin_addr));

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
	inet_pton(AF_INET,eth_ip, &(cli_addr.sin_addr));
	//cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	printf ("IP ADDR of router cli_eddr: %s\n",inet_ntoa(cli_addr.sin_addr));

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

	/*
	 * Prepare raw socket to talk to outside world
	 */
	int raw_socket = socket(AF_INET, SOCK_RAW,IPPROTO_ICMP);
	//check for error
	if(raw_socket < 0)
	{
		error ("ERROR creating raw socket...\n");
	}

	//initialize all fields in eth_addr to 0
	memset((char*) &eth_addr, 0 ,sizeof(eth_addr));
	//set fields in struct sockaddr_in
	eth_addr.sin_family = AF_INET;
	eth_addr.sin_port = htons(0); //Bind to any port
	//eth_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int error_check;
	error_check = inet_pton(AF_INET,eth_ip, &(eth_addr.sin_addr));
	if(error_check < 0)
	{
		error ("Error in converting string IPv4 to network byte order using inet_pton()\n");
	}
	else if(error_check == 0)
	{
		error("Invalid IP addr is pased in inet_pton() function\n");
	}

	//bind the raw_socket to an addr
	if(bind(raw_socket,(struct sockaddr*) &eth_addr,sizeof(eth_addr)) < 0)
	{
		error("ERROR binding to raw socket...\n");
	}
	rawlen = sizeof(eth_addr);

	if(sockfd > raw_socket)
	{
		maxfds1 = sockfd +1;
	}
	else
	{
		maxfds1 = raw_socket + 1;
	}

	while(1)
	{
	    int sel_return;
	    fd_set readfds;

	    FD_ZERO(&readfds); //initialize the set
	    FD_SET(sockfd, &readfds); //add sockfd to fd_set
	    FD_SET(raw_socket, &readfds); //add raw_socket to fd_set
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
			printf("RECEIVING ICMP REQUEST PACKET FROM PROXY IN ROUTER NUMBER %d\n", numRouter);
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
			writeInfoToFileRouterStage2Part2(fp,"port",portNum,saddr,daddr,icmp->type);

			/*
			 * Check to see if the dest addr is router itself or else where
			 * If it is addressed to itself, it should response as in stage 2
			 * If it is for some where else, it should change src IP addr to itself and sent it out using raw_socket
			 */
			if(strncmp (daddr,eth_ip,sizeof(daddr)) == 0)
			{
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
			/*
			 * ICMP packet is addressed to some where else
			 */
			else
			{
				/*
				 * Need to change source IP addr to this router IP addr
				 */
				for(int i=0; i< (int) sizeof(icmp_buffer); i++)
				{
					send_raw_buf[i] = icmp_buffer[i];
				}

				ip_raw = (struct iphdr*)send_raw_buf;
				icmp_raw = (struct icmphdr*) (send_raw_buf + sizeof(struct iphdr));


				//Before change the src addr, store it
				if(inet_ntop(AF_INET, &ip_raw->saddr,orig_src_addr,sizeof(orig_src_addr)) == NULL)
				{
					error ("ERROR saving original src\n");
				}

				//change src addr to this router ip addr
				error_check = inet_pton(AF_INET,eth_ip, &ip_raw->saddr);

				if(error_check < 0)
				{
					error ("Error in converting string IPv4 to network byte order using inet_pton() before sending to outside world\n");
				}
				else if(error_check == 0)
				{
					error("Invalid IP addr is pased in inet_pton() function before sending to outside world\n");
				}


				//initialize all fields in serv_addr to 0
				memset(&dest_addr, 0 ,sizeof(dest_addr));
				//set fields in struct sockaddr_in
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_port = htons(0);
				dest_addr.sin_addr.s_addr = ip_raw->daddr;

				socklen_t destlen;
				destlen = sizeof(dest_addr);

				/*
				 * construct struct msghdr to pass in sendmsg()
				 */
				msgh.msg_name = &dest_addr;
				msgh.msg_namelen = destlen;

				msgh.msg_iovlen = 1;
				msgh.msg_iov = &io;
				msgh.msg_iov->iov_base = icmp_raw;
				msgh.msg_iov->iov_len = ntohs(ip_raw->tot_len) - sizeof(struct iphdr);
				msgh.msg_control = NULL;
				msgh.msg_controllen = 0;
				msgh.msg_flags = 0;


				printf("SENDING DATA TO OUTSIDE WORLD THROUGH RAW INTERFACE from router number %d\n", numRouter);
				data = sendmsg(raw_socket, &msgh,0);
				if(data < 0)
				{
					printf("ERROR sending raw packet to outside world from router number %d\n", numRouter);
					exit(-1);
				}
			}
		}
		if(FD_ISSET(raw_socket, &readfds))
		{
			/*
			 * reset timeout
			 */
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			/*
			 * Receive data from raw interface
			 */
			printf("RECEIVING DATA FROM RAW INTERFACE...\n");
			data = recvfrom(raw_socket,rcv_raw_buf,sizeof(rcv_raw_buf),0,(struct sockaddr*) &eth_addr, &rawlen);
			if(data < 0)
			{
				printf ("ERROR receiving icmp echo message from outside world\n");
				exit(1);
			}

			/*
			 * Re-send ICMP ECHO packet
			 * Need to change dest IP addr to the original src ip addr
			 */
			for(int i=0; i< (int) sizeof(rcv_raw_buf); i++)
			{
				send_raw_buf[i] = rcv_raw_buf[i];
			}
			ip_raw = (struct iphdr*)send_raw_buf;
			inet_ntop(AF_INET,&ip_raw->saddr,saddr,sizeof(saddr));
			inet_ntop(AF_INET,&ip_raw->daddr,daddr,sizeof(daddr));

			icmp_raw = (struct icmphdr*) (send_raw_buf + sizeof(struct iphdr));

			/*
			 * Check if dest addr is addressed to this router
			 */
			if(strncmp(daddr,eth_ip,sizeof(daddr)) == 0)
			{
				/*
				 * write to file
				 */
				writeInfoToFileRouterStage2Part2(fp,"raw",portNum,saddr,daddr,icmp_raw->type);

				//change dest addr to the original src addr
				error_check = inet_pton(AF_INET,orig_src_addr, &ip_raw->daddr);
				if(error_check < 0)
				{
					error ("Error in converting string IPv4 to network byte order using inet_pton() before sending back to proxy\n");
				}
				else if(error_check == 0)
				{
					error("Invalid IP addr is passed in inet_pton() function before sending back to proxy\n");
				}

				/*
				 * Send ICMP ECHO request back to proxy
				 */
				printf("SENDING ICMP REPLY PACKET TO PROXY from router number %d\n", numRouter);
				data = sendto(sockfd,send_raw_buf,ntohs(ip_raw->tot_len),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending message from router number %d\n",numRouter);
				}
			}
		}
	}
    	fclose(fp);
    	close(sockfd);
    	close(raw_socket);
}

/*
 * Onion routing for stage 5
 */
void communicateToServerStage5(int portNum, int numRouter, char* eth_ip)
{
	char prependstr[] = "stage5.router";
	char postpendstr[] = ".out";
	char fileName[20];
	sprintf(fileName, "%s%d%s",prependstr,numRouter,postpendstr);

	FILE *fp = fopen(fileName,"w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}
	/*
	 * Necessary struct and constant declaration for sendmsg() - sending raw socket out to the world
	 */
	struct iovec io;
	struct msghdr msgh;
	memset(&msgh,0,sizeof(msgh));
	memset(&io,0,sizeof(io));


	struct timeval tv; //time interval wait for select()
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int maxfds1;

	int sockfd, data, pid,serPort;
	socklen_t serlen,clilen,rawlen;
	pid = getpid(); //get process pid
	struct icmphdr* icmp;
	struct iphdr* ip;

	char saddr[16];
	char daddr[16];
	char saved_daddr[16];

	char *buffer;

	char rcv_raw_buf [2048];


	struct iphdr* ip_raw;

	struct payload {
		int router;
		int pid;
		char* routerIP;
	};
	struct payload *p = (struct payload*) malloc(sizeof(struct payload));
	p->pid = pid;
	p->router = numRouter;
	p->routerIP = eth_ip;

	struct sockaddr_in serv_addr, cli_addr, eth_addr, dest_addr;

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
	printf ("IP ADDR of router serv_addr: %s\n",inet_ntoa(serv_addr.sin_addr));

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
	inet_pton(AF_INET,eth_ip, &(cli_addr.sin_addr));


	printf ("IP ADDR of router cli_eddr: %s\n",inet_ntoa(cli_addr.sin_addr));

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
	writeInfoToFileRouterStage5Part1(fp,numRouter,pid,serPort,eth_ip);


	/*
	 * Struct for payload
	 */
	struct controlMsg_payload {
		uint16_t rPort;
	}; //this struct is for CID in payload portion

	struct controlMsg_payload_long {
		char icmp_pkt[2048];
	}; //this struct is for ICMP packet payload


	char* buf_cm = malloc(sizeof(struct controlMessage));
	char* buf_payload = malloc(sizeof(struct controlMsg_payload));
	char* buf_payload_long = malloc (sizeof(struct controlMsg_payload_long));

	struct controlMsg_payload* payload;
	struct controlMsg_payload_long* payload_long = malloc (sizeof(struct controlMsg_payload_long));

	/*
	 * Getting info from received minitor control message
	 */
	struct iphdr* ip_hdr;
	struct ipPayload* ip_payload;


	int pktSize = 0;

	/*
	 * info that each router need to save
	 */
	uint16_t cidin = 0;
	uint16_t cidout = 0;
	int portin,portout;


	/*
	 * Prepare raw socket to talk to outside world
	 */
	int raw_socket = socket(AF_INET, SOCK_RAW,IPPROTO_ICMP);
	//check for error
	if(raw_socket < 0)
	{
		error ("ERROR creating raw socket...\n");
	}

	//initialize all fields in eth_addr to 0
	memset((char*) &eth_addr, 0 ,sizeof(eth_addr));
	//set fields in struct sockaddr_in
	eth_addr.sin_family = AF_INET;
	eth_addr.sin_port = htons(0); //Bind to any port
	int error_check;
	error_check = inet_pton(AF_INET,eth_ip, &(eth_addr.sin_addr));
	if(error_check < 0)
	{
		error ("Error in converting string IPv4 to network byte order using inet_pton()\n");
	}
	else if(error_check == 0)
	{
		error("Invalid IP addr is pased in inet_pton() function\n");
	}

	//bind the raw_socket to an addr
	if(bind(raw_socket,(struct sockaddr*) &eth_addr,sizeof(eth_addr)) < 0)
	{
		error("ERROR binding to raw socket...\n");
	}
	rawlen = sizeof(eth_addr);

	if(sockfd > raw_socket)
	{
		maxfds1 = sockfd +1;
	}
	else
	{
		maxfds1 = raw_socket + 1;
	}

	while(1)
	{
	    int sel_return;
	    fd_set readfds;

	    FD_ZERO(&readfds); //initialize the set
	    FD_SET(sockfd, &readfds); //add sockfd to fd_set
	    FD_SET(raw_socket, &readfds); //add raw_socket to fd_set
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

			/*
			 * Receive minitor control message from proxy
			 */
			printf ("Router number %d receiving data...\n",numRouter);
			printf ("RECEVING MINITOR CONTROL MESSAGE FROM PROXY or ROUTER\n");
			data = recvfrom(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr, &clilen);
			if(data < 0)
			{
				printf ("ERROR RECEVING MINITOR CONTROL MESSAGE FROM PROXY or ROUTER\n");
				exit(1);
			}
			printf ("Size of data received: %d\n",data);

			/*
			 * Getting info from received minitor control message
			 */
			ip_hdr = (struct iphdr*) buf_cm;
			ip_payload = (struct ipPayload*) (buf_cm+sizeof(struct iphdr));

			printf ("My new protocol = %d\n", ip_hdr->protocol);
			printf ("My new message type = %#04x\n", ip_payload->mType);
			printf ("My new cid = 0x%x\n",ntohs(ip_payload->cid));

			/*
			 * Now check if type is circuit-extend or relay data from proxy
			 */
			if(ip_payload->mType == 82) //circuit-extend
			{
				printf ("RECEVING MINITOR SMALL PAYLOAD FROM PROXY or ROUTER\n");
				data = recvfrom(sockfd,buf_payload,sizeof(struct controlMsg_payload),0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR RECEVING MINITOR SMALL PAYLOAD FROM PROXY or ROUTER\n");
					exit(1);
				}
				printf ("Size of data received: %d\n",data);

				/*
				 * Getting info from received minitor payload
				 */
				payload = (struct controlMsg_payload*) buf_payload;
				printf("My out port = %d\n",ntohs(payload->rPort));

				pktSize = sizeof(ip_payload->cid) + sizeof(ip_payload->mType) + sizeof(struct controlMsg_payload);

				/*
				 * Check to see if this router see the incoming cid before
				 */
				if(cidin == ntohs(ip_payload->cid)) //YES, seen before
				{
					printf("Router %d has seen this cid before, ready to forward to next hop\n", numRouter);
					//map this router cidout to cidin before send
					ip_payload->cid = htons(cidout);
					cli_addr.sin_port = htons(portout); //Set to next port before send

					printf("router %d forwards circuit-extend request down the chain to next port: %d\n",numRouter,portout);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwarding circuit-extend request down the chain from router number %d\n",numRouter);
					}

					printf ("router %d forwards payload for circuit-extend down the chain to next port: %d\n", numRouter,portout);
					data = sendto(sockfd,payload,sizeof(struct controlMsg_payload),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwards payload for circuit-extend down the chain\n");
					}
					/*
					 * log additional info on every received packet
					 */
					fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x%x\n",portin,pktSize,ip_payload->mType,cidin,ntohs(payload->rPort));

					fprintf(fp,"forwarding extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",cidin,cidout,portout);
				}
				else //NOT SEEN before
				{
					/*
					 * Store info for future use
					 */
					printf("Router number %d has not seen this cidin 0x%x before, storing new information...\n",numRouter,ntohs(ip_payload->cid));
					portin = ntohs(cli_addr.sin_port);
					portout = ntohs(payload->rPort);
					cidin = ntohs(ip_payload->cid);
					//calculate cinout
					cidout = numRouter * 256 + 1;
					/*
					 * log additional info on every received packet
					 */
					fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x%x\n",portin,pktSize,ip_payload->mType,cidin,portout);

					fprintf(fp,"new extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",cidin,cidout,portout);
					/*
					 * reply back to proxy with circuit-extend-done
					 */
					ip_payload->mType = 83; //0x53 in hex -> circuit-extend-done
					cli_addr.sin_port = htons(portin); //Set to previous port before send

					printf("Router %d sends data back the chain to proxy or previous router with circuit-extend-done\n",numRouter);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR sending data back the chain to proxy or previous router with circuit-extend-done from router number %d\n",numRouter);
					}
				}
			}
			else if(ip_payload->mType == 83)
			{

				cli_addr.sin_port = htons(portin); //Set to previous port before send
				int temp_cid = ntohs(ip_payload->cid); //temporarily store the incoming cid for write to file later
				//map back cid
				ip_payload->cid = htons(cidin);
				printf ("Map back original cidin: 0x%x\n",cidin);

				printf ("router forwards payload for circuit-extend up the chain\n");
				data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR forwards payload for circuit-extend up the chain\n");
				}
				/*
				 * log additional info
				 */
				pktSize = sizeof(ip_payload->cid) + sizeof(ip_payload->mType);
				fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x\n",portout,pktSize,ip_payload->mType,temp_cid);

				fprintf(fp,"forwarding extend-done circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",temp_cid,cidin,portin);
			}
			else if(ip_payload->mType == 81) //relay data
			{

				printf("RECEIVING ICMP REQUEST PACKET FROM PROXY IN ROUTER NUMBER %d\n", numRouter);
				data = recvfrom(sockfd,payload_long->icmp_pkt,84,0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR receiving icmp echo message from proxy\n");
					exit(1);
				}
				printf ("Processing relay data packet...\n");

				/*
				 * ip header info
				 */
				ip = (struct iphdr*) payload_long->icmp_pkt;
				inet_ntop(AF_INET,&ip->saddr,saddr,sizeof(saddr));
				inet_ntop(AF_INET,&ip->daddr,daddr,sizeof(daddr));

				//saved the original src to use later
				for(int m = 0; m < (int) sizeof(saddr); m++)
				{
					saved_daddr[m] = saddr[m];
				}

				/*
				 * icmp header info
				 */
				icmp = (struct icmphdr*) (payload_long->icmp_pkt+sizeof(struct iphdr));


				printf ("Incoming IP addr: %s\n", saddr);

				/*
				 * Check to see if router receive unknown circuit ID
				 */
				if(cidin != ntohs(ip_payload->cid))
				{
					printf ("FATAL ERROR: Router receive unknown circuit ID, logging and exiting...\n");
					fprintf (fp,"unknown incoming circuit: 0x%x, src: %s, dst: %s\n",ntohs(ip_payload->cid),saddr,daddr);
					exit(-1);
				}

				/*
				 * Change source IP addr to this router IP addr
				 */
				char outgoing_src[16];
				inet_pton(AF_INET,eth_ip,&ip->saddr);
				inet_ntop(AF_INET,&ip->saddr,outgoing_src,sizeof(outgoing_src));
				printf ("Incoming IP addr after changed: %s\n",outgoing_src);

				/*
				 * change cid
				 */
				ip_payload->cid = htons(cidout);

				/*
				 * Check to see if this router is the last one in the onion
				 * If it is, then send outside to the world through raw socket
				 * else, forward packet to next hop
				 */
				if(portout == 65535) //It's last hop, send out through raw
				{
					pktSize = data + sizeof(ip_payload->cid) + sizeof(ip_payload->mType);
					//initialize all fields in serv_addr to 0
					memset(&dest_addr, 0 ,sizeof(dest_addr));
					//set fields in struct sockaddr_in
					dest_addr.sin_family = AF_INET;
					dest_addr.sin_port = htons(0);
					dest_addr.sin_addr.s_addr = ip->daddr;

					socklen_t destlen;
					destlen = sizeof(dest_addr);

					/*
					 * construct struct msghdr to pass in sendmsg()
					 */
					msgh.msg_name = &dest_addr;
					msgh.msg_namelen = destlen;

					msgh.msg_iovlen = 1;
					msgh.msg_iov = &io;
					msgh.msg_iov->iov_base = icmp;
					msgh.msg_iov->iov_len = ntohs(ip->tot_len) - sizeof(struct iphdr);
					msgh.msg_control = NULL;
					msgh.msg_controllen = 0;
					msgh.msg_flags = 0;


					printf("SENDING DATA TO OUTSIDE WORLD THROUGH RAW INTERFACE from router number %d\n", numRouter);
					data = sendmsg(raw_socket, &msgh,0);
					if(data < 0)
					{
						printf("ERROR sending raw packet to outside world from router number %d\n", numRouter);
						exit(-1);
					}

					/*
					 * additional logging
					 */
					fprintf (fp, "pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2 (fp, (unsigned char *) payload_long->icmp_pkt, 84);

					/*
					 * logging
					 */
					fprintf(fp,"outgoing packet, circuit incoming: 0x%x, incoming src: %s, outgoing src: %s, dst: %s\n",cidin,saddr,outgoing_src,daddr);

				}
				else	//it's not last hop, forward it to next hop
				{
					ip_payload->mType = 81;
					cli_addr.sin_port = htons(portout); //Set to next port before send
					pktSize = data + sizeof(ip_payload->cid) + sizeof(ip_payload->mType);

					printf("router %d forwards relay-data request down the chain to next port: %d\n",numRouter,portout);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwarding relay-data request down the chain from router number %d\n",numRouter);
					}

					printf ("router %d forwards payload long for relay-data down the chain to next port: %d\n", numRouter,portout);
					data = sendto(sockfd,payload_long->icmp_pkt,84,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwards payload for relay-data down the chain\n");
					}

					/*
					 * additional logging
					 */
					fprintf (fp, "pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2 (fp, (unsigned char*) payload_long->icmp_pkt, 84);

					/*
					 * logging
					 */
					fprintf(fp,"relay packet, circuit incoming: 0x%x, outgoing: 0x%x, incoming src: %s, outgoing src: %s, dst: %s\n",cidin,cidout,saddr,outgoing_src,daddr);
				}

			}
			else if(ip_payload->mType == 84) //relay return data
			{
				printf("RECEIVING RELAY RETURN DATA IN ROUTER NUMBER %d\n", numRouter);
				data = recvfrom(sockfd,payload_long->icmp_pkt,84,0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR receiving icmp echo message from proxy\n");
					exit(1);
				}
				printf ("Processing relay return data...\n");

				ip_payload->mType = 84; //0x54 in hex -> relay-return-data
				cli_addr.sin_port = htons(portin); //Set to previous port before send
				//map back cid
				ip_payload->cid = htons(cidin);
				printf ("Map back original cidin: 0x%x\n",cidin);

				/*
				 * ip header info
				 */
				ip = (struct iphdr*) payload_long->icmp_pkt;
				inet_ntop(AF_INET,&ip->saddr,saddr,sizeof(saddr));
				inet_ntop(AF_INET,&ip->daddr,daddr,sizeof(daddr));

				/*
				 * Change dst addr to the previous hop addr
				 */
				inet_pton(AF_INET,saved_daddr, &ip->daddr);
				char temp1[16];
				inet_ntop(AF_INET,&ip->daddr,temp1,sizeof(temp1));
				printf ("Map back dst addr before forwarding: %s\n", temp1);

				/*
				 * Additional logging
				 */
				pktSize = data + sizeof(ip_payload->cid) + sizeof(ip_payload->mType);
				fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x",portout,pktSize,ip_payload->mType,cidout);
				writeInfoToFileRouterStage5Part2 (fp, (unsigned char*) payload_long->icmp_pkt, 84);

				/*
				 * Logging
				 */
				fprintf(fp,"relay reply packet, circuit incoming: 0x%x, outgoing: 0x%x, src: %s, incoming dst: %s, outgoing dst: %s\n",cidout,cidin,saddr,daddr,temp1);

				printf("SENDING RELAY-RETURN-DATA PACKET UP THE CHAIN...\n");

				printf("router %d forwards relay-return-data request back up the chain to next port: %d\n",numRouter,portin);
				data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR forwarding relay-return-data request back up the chain from router number %d\n",numRouter);
				}

				printf ("router %d forwards payload long for relay-return-data back up the chain to next port: %d\n", numRouter,portin);
				data = sendto(sockfd,payload_long->icmp_pkt,84,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR forwarding payload long for relay-return-data back up the chain\n");
				}
			}
		}


		if(FD_ISSET(raw_socket, &readfds))
		{
			/*
			 * reset timeout
			 */
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			/*
			 * Receive data from raw interface
			 */
			printf("RECEIVING DATA FROM RAW INTERFACE...\n");
			data = recvfrom(raw_socket,rcv_raw_buf,sizeof(rcv_raw_buf),0,(struct sockaddr*) &eth_addr, &rawlen);
			if(data < 0)
			{
				printf ("ERROR receiving icmp echo message from outside world\n");
				exit(1);
			}

			printf ("Processing data before sending back up the chain...\n");
			/*
			 * reply back relay-return-data type
			 */
			ip_payload->mType = 84; //0x54 in hex -> relay-return-data
			cli_addr.sin_port = htons(portin); //Set to previous port before send
			//map back cid
			ip_payload->cid = htons(cidin);
			printf ("Map back original cidin: 0x%x\n",cidin);

			/*
			 * Re-send ICMP ECHO packet
			 * Need to change dest IP addr to the original src ip addr
			 */
			for(int i=0; i< (int) sizeof(rcv_raw_buf); i++)
			{
				payload_long->icmp_pkt[i] = rcv_raw_buf[i];
			}
			ip_raw = (struct iphdr*)payload_long->icmp_pkt;
			inet_ntop(AF_INET,&ip_raw->saddr,saddr,sizeof(saddr));
			inet_ntop(AF_INET,&ip_raw->daddr,daddr,sizeof(daddr));

			/*
			 * Change dst addr to the previous hop addr
			 */
			inet_pton(AF_INET,saved_daddr, &ip_raw->daddr);
			char temp[16];
			inet_ntop(AF_INET,&ip_raw->daddr,temp,sizeof(temp));
			printf ("Map back dst addr: %s\n", temp);

			/*
			 * Check if dest addr is addressed to this router
			 */
			if(strncmp(daddr,eth_ip,sizeof(daddr)) == 0)
			{
				/*
				 * write to file
				 */
				fprintf(fp,"incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",saddr,daddr,cidin);

				printf("router %d sends relay-return-data request back up the chain to next port: %d\n",numRouter,portin);
				data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending relay-return-data request back up the chain from router number %d\n",numRouter);
				}

				printf ("router %d sends payload long for relay-return-data back up the chain to next port: %d\n", numRouter,portin);
				data = sendto(sockfd,payload_long->icmp_pkt,84,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending payload long for relay-return-data back up the chain\n");
				}
			}
		}
	}
    	fclose(fp);
    	close(sockfd);
    	close(raw_socket);
    	free(buf_cm);
    	free(buf_payload);
    	free(buf_payload_long);
    	free(payload_long);
}

/*
 * Encrypted Onion routing for stage 6
 */
void communicateToServerStage6(int portNum, int numRouter, char* eth_ip)
{
	char prependstr[] = "stage6.router";
	char postpendstr[] = ".out";
	char fileName[20];
	sprintf(fileName, "%s%d%s",prependstr,numRouter,postpendstr);

	FILE *fp = fopen(fileName,"w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}
	/*
	 * Necessary struct and constant declaration for sendmsg() - sending raw socket out to the world
	 */
	struct iovec io;
	struct msghdr msgh;
	memset(&msgh,0,sizeof(msgh));
	memset(&io,0,sizeof(io));


	struct timeval tv; //time interval wait for select()
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int maxfds1;

	int sockfd, data, pid,serPort;
	socklen_t serlen,clilen,rawlen;
	pid = getpid(); //get process pid
	struct icmphdr* icmp;
	struct iphdr* ip;

	char saddr[16];
	char daddr[16];

	char *buffer;


	struct iphdr* ip_raw;

	struct payload {
		int router;
		int pid;
		char* routerIP;
	};
	struct payload *p = (struct payload*) malloc(sizeof(struct payload));
	p->pid = pid;
	p->router = numRouter;
	p->routerIP = eth_ip;

	struct sockaddr_in serv_addr, cli_addr, eth_addr, dest_addr;

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
	printf ("IP ADDR of router serv_addr: %s\n",inet_ntoa(serv_addr.sin_addr));

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
	inet_pton(AF_INET,eth_ip, &(cli_addr.sin_addr));

	printf ("IP ADDR of router cli_eddr: %s\n",inet_ntoa(cli_addr.sin_addr));

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
	writeInfoToFileRouterStage5Part1(fp,numRouter,pid,serPort,eth_ip);


	/*
	 * Struct for payload
	 */
	struct controlMsg_payload {
		uint16_t rPort;
	}; //this struct is for CID in payload portion

	struct controlMsg_payload_long {
		char icmp_pkt[2048];
	}; //this struct is for ICMP packet payload


	char* buf_cm = malloc(sizeof(struct controlMessage));
	char* buf_payload = malloc(sizeof(struct controlMsg_payload));
	char* buf_payload_long = malloc (sizeof(struct controlMsg_payload_long));


	struct controlMsg_payload_long* payload_long = malloc (sizeof(struct controlMsg_payload_long));

	/*
	 * Getting info from received minitor control message
	 */
	struct iphdr* ip_hdr;
	struct ipPayload* ip_payload;


	int pktSize = 0;

	/*
	 * info that each router need to save
	 */
	uint16_t cidin = 0;
	uint16_t cidout = 0;
	int portin,portout;
	unsigned char sKey[16];
	unsigned char *buf_sKey = malloc (2048);
	int buf_sKey_size = 2048;
	int isSeen = 0;
	int saved_data = 0;
	/*
	 * variable needed for AES decryption
	 */
	AES_KEY enc_key;
	AES_KEY dec_key;
	unsigned char *clear_crypt_text;
	int clear_crypt_text_len;
	unsigned char *crypt_text;
	int crypt_text_len;
	unsigned char *crypt_buffer = malloc (2048);
	int crypt_buf_size = 2048;

	/*
	 * Prepare raw socket to talk to outside world
	 */
	int raw_socket = socket(AF_INET, SOCK_RAW,IPPROTO_ICMP);
	//check for error
	if(raw_socket < 0)
	{
		error ("ERROR creating raw socket...\n");
	}

	//initialize all fields in eth_addr to 0
	memset((char*) &eth_addr, 0 ,sizeof(eth_addr));
	//set fields in struct sockaddr_in
	eth_addr.sin_family = AF_INET;
	eth_addr.sin_port = htons(0); //Bind to any port
	int error_check;
	error_check = inet_pton(AF_INET,eth_ip, &(eth_addr.sin_addr));
	if(error_check < 0)
	{
		error ("Error in converting string IPv4 to network byte order using inet_pton()\n");
	}
	else if(error_check == 0)
	{
		error("Invalid IP addr is pased in inet_pton() function\n");
	}

	//bind the raw_socket to an addr
	if(bind(raw_socket,(struct sockaddr*) &eth_addr,sizeof(eth_addr)) < 0)
	{
		error("ERROR binding to raw socket...\n");
	}
	rawlen = sizeof(eth_addr);

	if(sockfd > raw_socket)
	{
		maxfds1 = sockfd +1;
	}
	else
	{
		maxfds1 = raw_socket + 1;
	}

	while(1)
	{
	    int sel_return;
	    fd_set readfds;

	    FD_ZERO(&readfds); //initialize the set
	    FD_SET(sockfd, &readfds); //add sockfd to fd_set
	    FD_SET(raw_socket, &readfds); //add raw_socket to fd_set
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

			/*
			 * Receive minitor control message from proxy
			 */
			printf ("Router number %d receiving data...\n",numRouter);
			printf ("RECEVING MINITOR CONTROL MESSAGE FROM PROXY or ROUTER\n");
			data = recvfrom(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr, &clilen);
			if(data < 0)
			{
				printf ("ERROR RECEVING MINITOR CONTROL MESSAGE FROM PROXY or ROUTER\n");
				exit(1);
			}
			//printf ("Size of data received: %d\n",data);

			/*
			 * Getting info from received minitor control message
			 */
			ip_hdr = (struct iphdr*) buf_cm;
			ip_payload = (struct ipPayload*) (buf_cm+sizeof(struct iphdr));

			printf ("My new protocol = %d\n", ip_hdr->protocol);
			printf ("My new message type = %#04x\n", ip_payload->mType);
			printf ("My new cid = 0x%x\n",ntohs(ip_payload->cid));

			/*
			 * Now check if type is fake-diffie-hellman
			 */
			if(ip_payload->mType == 101)
			{
				printf ("RECEVING FAKE-DIFFIE-HELLMAN REQUEST FROM PROXY or ROUTER\n");
				data = recvfrom(sockfd,buf_sKey,buf_sKey_size,0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR RECEVING FAKE-DIFFIE-HELLMAN REQUEST FROM PROXY or ROUTER\n");
					exit(1);
				}
				//printf ("Size of data received: %d\n",data);

				/*
				 * check to see if this router see this incoming cid before
				 * Re-use functions to encrypt/decrypt provided by Professor and TA
				 */
				if(cidin == ntohs(ip_payload->cid)) //YES, seen before
				{
					saved_data = data;
					printf("Before forwarding, need to decrypt its onion layer. Decrypting...\n");

					class_AES_set_decrypt_key(sKey, &dec_key);
					printf("MY SKEY = 0x");
					for(int m = 0; m < (int) sizeof(sKey); m++)
					{
						printf("%02x",sKey[m]);
					}
					printf ("\n");

					//printf("MY DATA SIZE = %d\n", data);
					printf("MY BUF_SKEY = 0x");
					for(int m = 0; m < data; m++)
					{
						printf("%02x",*(buf_sKey+m));
					}
					printf ("\n");

					class_AES_decrypt_with_padding(buf_sKey, data, &clear_crypt_text, &clear_crypt_text_len, &dec_key);
					if(clear_crypt_text_len > crypt_buf_size)
					{
						crypt_buf_size = clear_crypt_text_len*2;
						crypt_buffer = realloc(crypt_buffer,crypt_buf_size);
					}
					for(int m = 0; m < clear_crypt_text_len; m++)
					{
						*(crypt_buffer+ m) = *(clear_crypt_text+m);
					}
					free(clear_crypt_text);

					printf("My next hop session key = 0x");
					for(int m = 0; m < clear_crypt_text_len; m++)
					{
						printf("%02x",crypt_buffer[m]);
					}
					printf ("\n");

					printf ("Decrypt successfully. Now forward session key to next hop\n");

					//map this router cidout to cidin before send
					ip_payload->cid = htons(cidout);
					cli_addr.sin_port = htons(portout); //Set to next port before send

					printf("router %d forwards fake-diffie-hellman request down the chain to next port: %d\n",numRouter,portout);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwarding fake-diffie-hellman request down the chain from router number %d\n",numRouter);
					}

					printf ("router %d forwards fake-diffie-hellman session key down the chain to next port: %d\n", numRouter,portout);
					data = sendto(sockfd,crypt_buffer,clear_crypt_text_len,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwards fake-diffie-hellman session key down the chain to next port\n");
					}
					printf ("Size of data sent: %d\n",data);
					/*
					 * log additional info on every received packet
					 */
					pktSize = saved_data + sizeof(ip_payload->mType) + sizeof(ip_payload->cid);
					fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2(fp,buf_sKey,saved_data);

					fprintf(fp,"fake-diffie-hellman, forwarding, circuit incoming: 0x%x, circuit outgoing: 0x%x, key: 0x",cidin,cidout);
					writeInfoToFileRouterStage5Part2(fp,crypt_buffer,clear_crypt_text_len);
				}
				else //NOT SEEN before
				{
					/*
					 * Store info for future use
					 */
					printf("Router number %d has not seen this cidin 0x%x before, storing new information...\n",numRouter,ntohs(ip_payload->cid));
					portin = ntohs(cli_addr.sin_port);
					cidin = ntohs(ip_payload->cid);

					for(int m = 0; m < (int) sizeof(sKey); m++)
					{
						*(sKey+m) = *(buf_sKey+m);
					}

					printf("My number in router = 0x");
					for(int m = 0; m < (int) sizeof(sKey); m++)
					{
						printf("%02x",sKey[m]);
					}
					printf ("\n");

					//class_AES_set_encrypt_key(sKey, &enc_key);

					/*
					 * log additional info on every received packet
					 */
					pktSize = sizeof(ip_payload->mType) + sizeof(ip_payload->cid) + sizeof(sKey);
					fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2(fp,sKey,sizeof(sKey));

					fprintf(fp,"fake-diffie-hellman, new circuit incoming: 0x%x, key: 0x",cidin);
					writeInfoToFileRouterStage5Part2(fp,sKey,sizeof(sKey));
				}
			}

			/*
			 * Now check if type is encrypted-circuit-extend
			 */
			else if(ip_payload->mType == 98) //encrypted-circuit-extend
			{
				printf ("RECEVING ENCRYPTED CIRCUIT EXTEND FROM PROXY or ROUTER\n");
				data = recvfrom(sockfd,buf_sKey,buf_sKey_size,0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR RECEVING ENCRYPTED CIRCUIT EXTEND FROM PROXY or ROUTER\n");
					exit(1);
				}
				printf ("Size of data received: %d\n",data);
				saved_data = data;

				/*
				 * Re-use functions to encrypt/decrypt provided by Professor and TA
				 */
				class_AES_set_decrypt_key(sKey, &dec_key);
				class_AES_decrypt_with_padding(buf_sKey, data, &clear_crypt_text, &clear_crypt_text_len, &dec_key);
				if(clear_crypt_text_len > crypt_buf_size)
				{
					crypt_buf_size = clear_crypt_text_len*2;
					crypt_buffer = realloc(crypt_buffer,crypt_buf_size);
				}
				for(int m = 0; m < clear_crypt_text_len; m++)
				{
					*(crypt_buffer+ m) = *(clear_crypt_text+m);
				}
				free(clear_crypt_text);

				printf ("My clear text length after = %d\n", clear_crypt_text_len);
				/*
				 * check to see if this is a new circuit extend or router need to forward it
				 */
				if(isSeen == 0) //NOT SEEN, saved port NEXT NAME and return encrypted-circuit-extend done to proxy
				{
					/*
					 * Getting the port value
					 */
					isSeen = 1; //set isSeen = 1
					printf("Saving next port number and generate CID out\n");
					uint16_t port_val = crypt_buffer[0] | (crypt_buffer[1] << 8);
					printf ("My port after decrypt = %d\n", ntohs(port_val));

					portout = ntohs(port_val);
					//calculate cid out
					cidout = numRouter * 256 + 1;

					/*
					 * log additional info on every received packet
					 */
					pktSize = sizeof(ip_payload->mType) + sizeof(ip_payload->cid) + saved_data;
					fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2(fp,buf_sKey,saved_data);

					fprintf(fp,"new extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",cidin,cidout,portout);

					/*
					 * reply back to proxy with encrypted-circuit-extend-done
					 */
					ip_payload->mType = 99; //0x63 in hex -> encrypted-circuit-extend-done
					cli_addr.sin_port = htons(portin); //Set to previous port before send

					printf("Router %d sends data back the chain to proxy or previous router with encrypted-circuit-extend-done\n",numRouter);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR sending data back the chain to proxy or previous router with encrypted-circuit-extend-done from router number %d\n",numRouter);
					}
				}
				else //need to forward the packet to next port
				{
					//map this router cidout to cidin before send
					ip_payload->cid = htons(cidout);
					cli_addr.sin_port = htons(portout); //Set to next port before send

					printf("router %d forwards encrypted-circuit-extend request down the chain to next port: %d\n",numRouter,portout);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwarding encrypted-circuit-extend request down the chain from router number %d\n",numRouter);
					}

					printf ("router %d forwards encrypted-circuit-extend encrypted port number down the chain to next port: %d\n", numRouter,portout);
					data = sendto(sockfd,crypt_buffer,clear_crypt_text_len,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwards encrypted-circuit-extend port number down the chain to next port\n");
					}
					printf ("Size of data sent: %d\n",data);
					/*
					 * log additional info on every received packet
					 */
					pktSize = saved_data + sizeof(ip_payload->mType) + sizeof(ip_payload->cid);
					fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2(fp,buf_sKey,saved_data);

					fprintf(fp,"forwarding extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",cidin,cidout,portout);
				}
			}
			/*
			 * Check if message control is encrypted-circuit-extend-done from router
			 */
			else if(ip_payload->mType == 99) //encrypted-circuit-extend-done
			{
				cli_addr.sin_port = htons(portin); //Set to previous port before send
				int temp_cid = ntohs(ip_payload->cid); //temporarily store the incoming cid for write to file later
				//map back cid
				ip_payload->cid = htons(cidin);
				printf ("Map back original cidin: 0x%x\n",cidin);

				printf ("router forwards encrypted-circuit-extend-done up the chain\n");
				data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR forwarding encrypted-circuit-extend-done up the chain\n");
				}
				/*
				 * log additional info
				 */
				pktSize = sizeof(ip_payload->cid) + sizeof(ip_payload->mType);
				fprintf(fp,"pkt from port: %d, length: %d, contents: 0x%x%04x\n",portout,pktSize,ip_payload->mType,temp_cid);

				fprintf(fp,"forwarding extend-done circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",temp_cid,cidin,portin);
			}
			/*
			 * check to see if message control is relay-data with encryption
			 */
			else if(ip_payload->mType == 97) //Relay-data with encryption
			{
				printf ("RECEVING RELAY-DATA WITH ENCRYPTION FROM PROXY or ROUTER\n");
				data = recvfrom(sockfd,buf_sKey,buf_sKey_size,0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR RECEVING ENCRYPTED CIRCUIT EXTEND FROM PROXY or ROUTER\n");
					exit(1);
				}
				printf ("Size of data received: %d\n",data);
				saved_data = data;

				/*
				 * Re-use functions to encrypt/decrypt provided by Professor and TA
				 */
				class_AES_set_decrypt_key(sKey, &dec_key);
				class_AES_decrypt_with_padding(buf_sKey, data, &clear_crypt_text, &clear_crypt_text_len, &dec_key);
				if(clear_crypt_text_len > crypt_buf_size)
				{
					crypt_buf_size = clear_crypt_text_len*2;
					crypt_buffer = realloc(crypt_buffer,crypt_buf_size);
				}
				for(int m = 0; m < clear_crypt_text_len; m++)
				{
					*(crypt_buffer+ m) = *(clear_crypt_text+m);
				}
				free(clear_crypt_text);

				printf ("My clear text length after = %d\n", clear_crypt_text_len);

				/*
				 * change cid
				 */
				ip_payload->cid = htons(cidout);

				/*
				 * check if this is last hop.
				 * If it is, send out to the world
				 * else forward to next hop
				 */
				if(portout == 65535) //It's last hop, send out through raw
				{
					/*
					 * Decapsulate packet
					 */
					ip = (struct iphdr*) crypt_buffer;
					inet_ntop(AF_INET,&ip->saddr,saddr,sizeof(saddr));
					inet_ntop(AF_INET,&ip->daddr,daddr,sizeof(daddr));

					char store_saddr[16];
					for(int m=0; m < 16; m++)
					{
						store_saddr[m] = saddr[m];
					}

					/*
					 * icmp header info
					 */
					icmp = (struct icmphdr*) (crypt_buffer+sizeof(struct iphdr));

					/*
					 * change src IP to this router
					 */
					inet_pton(AF_INET,eth_ip, &ip->saddr);


					//initialize all fields in serv_addr to 0
					memset(&dest_addr, 0 ,sizeof(dest_addr));
					//set fields in struct sockaddr_in
					dest_addr.sin_family = AF_INET;
					dest_addr.sin_port = htons(0);
					dest_addr.sin_addr.s_addr = ip->daddr;

					socklen_t destlen;
					destlen = sizeof(dest_addr);

					/*
					 * construct struct msghdr to pass in sendmsg()
					 */
					msgh.msg_name = &dest_addr;
					msgh.msg_namelen = destlen;

					msgh.msg_iovlen = 1;
					msgh.msg_iov = &io;
					msgh.msg_iov->iov_base = icmp;
					msgh.msg_iov->iov_len = ntohs(ip->tot_len) - sizeof(struct iphdr);
					msgh.msg_control = NULL;
					msgh.msg_controllen = 0;
					msgh.msg_flags = 0;


					printf("SENDING DATA TO OUTSIDE WORLD THROUGH RAW INTERFACE from router number %d\n", numRouter);
					data = sendmsg(raw_socket, &msgh,0);
					if(data < 0)
					{
						printf("ERROR sending raw packet to outside world from router number %d\n", numRouter);
						exit(-1);
					}

					/*
					 * additional logging
					 */
					pktSize = saved_data + sizeof(ip_payload->cid) + sizeof(ip_payload->mType);
					fprintf (fp, "pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2(fp,buf_sKey,saved_data);
					/*
					 * logging
					 */
					fprintf(fp,"outgoing packet, circuit incoming: 0x%x, incoming src: %s, outgoing src: %s, dst: %s\n",cidin,store_saddr,eth_ip,daddr);
				}
				else	//it's not last hop, forward it to next hop
				{
					ip_payload->mType = 97;
					cli_addr.sin_port = htons(portout); //Set to next port before send
					pktSize = saved_data + sizeof(ip_payload->cid) + sizeof(ip_payload->mType);

					printf("router %d forwards relay-data request with encryption down the chain to next port: %d\n",numRouter,portout);
					data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwarding relay-data request with encryption down the chain from router number %d\n",numRouter);
					}

					printf ("router %d forwards payload long for relay-data with encryption down the chain to next port: %d\n", numRouter,portout);
					data = sendto(sockfd,crypt_buffer,clear_crypt_text_len,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
					if(data < 0)
					{
						printf ("ERROR forwards payload long for relay-data with encryption down the chain\n");
					}

					/*
					 * additional logging
					 */
					fprintf (fp, "pkt from port: %d, length: %d, contents: 0x%x%04x",portin,pktSize,ip_payload->mType,cidin);
					writeInfoToFileRouterStage5Part2(fp,buf_sKey,saved_data);


					/*
					 * logging
					 */
					fprintf(fp,"relay encrypted packet, circuit incoming: 0x%x, outgoing: 0x%x\n",cidin,cidout);
				}
			}
			/*
			 * Check to see if this is relay reply data request with encryption
			 */
			else if(ip_payload->mType == 100)
			{
				printf("RECEIVING RELAY REPLY DATA REQUEST WITH ENCRYPTION FROM ROUTER...\n");
				data = recvfrom(sockfd,buf_sKey,buf_sKey_size,0,(struct sockaddr*) &cli_addr, &clilen);
				if(data < 0)
				{
					printf ("ERROR RECEVING RELAY REPLY DATA REQUEST WITH ENCRYPTION FROM ROUTER\n");
					exit(1);
				}
				saved_data = data;
				//printf ("Size of data received: %d\n",data);

				ip_payload->mType = 100; //0x64 in hex -> relay-return-data with encryption
				cli_addr.sin_port = htons(portin); //Set to previous port before send

				//map back cid
				ip_payload->cid = htons(cidin);
				printf ("Map back original cidin: 0x%x\n",cidin);

				/*
				 * Re-use functions to encrypt/decrypt provided by Professor and TA
				 */
				//Add layer of encryption
				class_AES_set_encrypt_key(sKey, &enc_key);
				class_AES_encrypt_with_padding(buf_sKey, data, &crypt_text, &crypt_text_len, &enc_key);
				if(crypt_text_len > crypt_buf_size)
				{
					crypt_buf_size = crypt_text_len*2;
					crypt_buffer = realloc(crypt_buffer,crypt_buf_size);
				}
				for(int m = 0; m < crypt_text_len; m++)
				{
					*(crypt_buffer+ m) = *(crypt_text+m);
				}
				free(crypt_text);

				printf ("My crypt text length after = %d\n", crypt_text_len);

				printf("router %d sends relay-reply-return-data request with encryption back up the chain to next port: %d\n",numRouter,portin);
				data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending relay-reply-return-data request with encryption back up the chain from router number %d\n",numRouter);
				}

				printf ("router %d sends payload long for relay-reply-return-data with encryption back up the chain to next port: %d\n", numRouter,portin);
				data = sendto(sockfd,crypt_buffer,crypt_text_len,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending payload long for relay-reply-return-data with encryption back up the chain\n");
				}
				/*
				 * additional logging
				 */
				pktSize = saved_data + sizeof(ip_payload->cid) + sizeof(ip_payload->mType);
				fprintf (fp, "pkt from port: %d, length: %d, contents: 0x%x%04x",portout,pktSize,ip_payload->mType,cidout);
				writeInfoToFileRouterStage5Part2(fp,buf_sKey,saved_data);


				/*
				 * logging
				 */
				fprintf(fp,"relay reply encrypted packet, circuit incoming: 0x%x, outgoing: 0x%x\n",cidout,cidin);
			}
		}


		if(FD_ISSET(raw_socket, &readfds))
		{
			/*
			 * reset timeout
			 */
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			/*
			 * Receive data from raw interface
			 */
			printf("RECEIVING DATA FROM RAW INTERFACE...\n");
			data = recvfrom(raw_socket,buf_sKey,buf_sKey_size,0,(struct sockaddr*) &eth_addr, &rawlen);
			if(data < 0)
			{
				printf ("ERROR receiving icmp echo message from outside world\n");
				exit(1);
			}

			printf ("Processing data before sending back up the chain...\n");
			/*
			 * reply back relay-return-data type
			 */
			ip_payload->mType = 100; //0x64 in hex -> relay-return-data with encryption
			cli_addr.sin_port = htons(portin); //Set to previous port before send

			//map back cid
			ip_payload->cid = htons(cidin);
			printf ("Map back original cidin: 0x%x\n",cidin);


			ip_raw = (struct iphdr*)buf_sKey;
			inet_ntop(AF_INET,&ip_raw->saddr,saddr,sizeof(saddr));
			inet_ntop(AF_INET,&ip_raw->daddr,daddr,sizeof(daddr));

			/*
			 * Zero destination addr before send back the chain
			 */
			inet_pton(AF_INET,"0.0.0.0", &ip_raw->daddr);
			char temp[16];
			inet_ntop(AF_INET,&ip_raw->daddr,temp,sizeof(temp));
			printf ("Zero dst addr: %s\n", temp);


			/*
			 * Ecnrypt the packet before send
			 * Re-use functions to encrypt/decrypt provided by Professor and TA
			 */
			class_AES_set_encrypt_key(sKey, &enc_key);
			class_AES_encrypt_with_padding(buf_sKey, data, &crypt_text, &crypt_text_len, &enc_key);
			if(crypt_text_len > crypt_buf_size)
			{
				crypt_buf_size = crypt_text_len*2;
				crypt_buffer = realloc(crypt_buffer,crypt_buf_size);
			}
			for(int m = 0; m < crypt_text_len; m++)
			{
				*(crypt_buffer+ m) = *(crypt_text+m);
			}
			free(crypt_text);

			printf ("My crypt text length after = %d\n", crypt_text_len);

			/*
			 * Check if dest addr is addressed to this router
			 */
			if(strncmp(daddr,eth_ip,sizeof(daddr)) == 0)
			{
				fprintf(fp,"incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",saddr,daddr,cidin);

				printf("router %d sends relay-return-data request with encryption back up the chain to next port: %d\n",numRouter,portin);
				data = sendto(sockfd,buf_cm,sizeof(struct controlMessage),0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending relay-return-data request with encryption back up the chain from router number %d\n",numRouter);
				}

				printf ("router %d sends payload long for relay-return-data with encryption back up the chain to next port: %d\n", numRouter,portin);
				data = sendto(sockfd,crypt_buffer,crypt_text_len,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
				if(data < 0)
				{
					printf ("ERROR sending payload long for relay-return-data with encryption back up the chain\n");
				}
			}
		}
	}
    	fclose(fp);
    	close(sockfd);
    	close(raw_socket);
    	free(buf_cm);
    	free(buf_payload);
    	free(buf_payload_long);
    	free(payload_long);
    	free(crypt_buffer);
    	free(buf_sKey);
}

/*
 * Write to file for stage 5 part 1
 */
void writeInfoToFileRouterStage5Part1(FILE *fp1,int router, int pidNum,int serPort,char* rIP)
{
	fprintf(fp1,"router: %d, pid: %d, port: %d, IP: %s\n",router,pidNum,serPort, rIP);
}

/*
 * write to file for stage 1
 */
void writeInfoToFileRouterStage1(int router,int pidNum,int serPort)
{
	FILE *fp = fopen("stage1.router1.out","w");
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
void writeInfoToFileRouterStage2Part2(FILE *fp1,char *from,int cliPort,char* sourceAddr,char* destAddr,int type)
{
	if(strncmp (from, "port",4) == 0)
	{
		fprintf(fp1,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",cliPort,sourceAddr,destAddr,type);
	}
	else
	{
		fprintf(fp1,"ICMP from raw sock, src: %s, dst: %s, type: %d\n",sourceAddr,destAddr,type);
	}
}

/*
 * write to file for stage 2 part 1
 */
void writeInfoToFileRouterStage2Part1(FILE *fp1,int router, int pidNum,int serPort)
{
	fprintf(fp1,"router: %d, pid: %d, port: %d\n",router,pidNum,serPort);
}

/*
 * write to file for stage 5 part 2
 */
void writeInfoToFileRouterStage5Part2 (FILE *fp1, unsigned char *content_arr, int arr_size)
{
	for(int i = 0; i < arr_size; i++)
	{
		fprintf(fp1,"%02x",content_arr[i]);
	}
	fprintf(fp1,"\n");
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

/*
 * proxy.c
 *
 *  Created on: Sep 17, 2014
 *      Author: Nguyen Tran
 */

#include "proxy.h"

#include "router1.h"
#include <linux/icmp.h>
#include <linux/ip.h>
#include <arpa/inet.h>


#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>

/*
 * Print error message with error code
 */
void error(char* message)
{
	perror(message);
	exit(1);
}

/*
 * function to call in parent process for communication in Stage 1
 */
void createCommunicationStage1(int numRouter)
{
	int portno, data, noRouter,routerPort, pid;
	int sockfd;
	socklen_t servlen,clilen;
	noRouter = 0;

	struct sockaddr_in serv_addr, cli_addr; //UDP connection

	//Struct to get router info from router
	struct payload {
		int router;
		int pid;
	};
	struct payload *p;
	char* buffer = malloc(sizeof(struct payload));

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
	serv_addr.sin_port = htons(0); //Bind to port 0 and let the OS assigns the UDP port
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind the socket to an addr
	if(bind(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR binding to socket in server...");
	}
	else
	{
		servlen = sizeof(serv_addr);
		getsockname(sockfd,(struct sockaddr *) &serv_addr, &servlen);
		portno = ntohs(serv_addr.sin_port);
	}


	clilen = sizeof(cli_addr);
	for(int n = 0; n < numRouter; n++)
	{
		pid = fork(); //fork a router based on number of router
		noRouter++; //Update number of router after each forking
		if(pid < 0)
		{
			error ("ERROR on fork");
		}
		else if(pid == 0) //in CHILD process
		{
			communicateToServerStage1(portno,noRouter);
			close(sockfd);
		}
		else //in PARENT process
		{
			printf("Waiting on port %d\n", portno);
			data = recvfrom(sockfd,buffer,sizeof(struct payload),0,(struct sockaddr*) &cli_addr, &clilen);

			if(data < 0)
			{
				printf ("ERROR receiving message in port %d\n",portno);
			}
			else
			{
				p = (struct payload*)buffer;
			}
				routerPort = ntohs(cli_addr.sin_port);

				/*
				 * write to file
				 */
				writeInfoToFileStage1(portno,p->router,p->pid,routerPort);

				free(buffer);
		}
	}
}

/*
 * function to call in parent process for communication in Stage 2
 */
void createCommunicationStage2(int numRouter)
{
	FILE *fp = fopen("stage2.proxy.out","w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}

	int portno, data, noRouter, routerPort, pid;
	int sockfd;
	socklen_t servlen,clilen;
	noRouter = 0;

	struct timeval tv; //time interval wait for select()
	tv.tv_sec = 7;
	tv.tv_usec = 0;

	struct icmphdr* icmp;
	struct iphdr* ip;

	char saddr[16];
	char daddr[16];

	struct sockaddr_in serv_addr, cli_addr;

	//Getting router info from router
	struct payload {
		int router;
		int pid;
	};
	struct payload *p;

	char* buffer = malloc(sizeof(struct payload));


	char icmp_reply_buf[2048];

	/*
	 * Set up Tunnel
	 */
    char tun_name[IFNAMSIZ];
    char buffer_tun[2048];

    int maxfds1;


    /* Connect to the tunnel interface (make sure you create the tunnel interface first) */
    strcpy(tun_name, "tun1");
    int tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);

    if(tun_fd < 0)
    {
    	perror("Open tunnel interface");
    	exit(1);
    }



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
	serv_addr.sin_port = htons(0); //Bind to port 0 and let the OS assigns the UDP port
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind the socket to an addr
	if(bind(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR binding to socket in server...");
	}
	else
	{
		servlen = sizeof(serv_addr);
		getsockname(sockfd,(struct sockaddr *) &serv_addr, &servlen);
		portno = ntohs(serv_addr.sin_port);
	}

	/*
	 * Calculate max fd set
	 */
    if(tun_fd > sockfd)
    {
    	maxfds1 = tun_fd + 1;
    }
    else
    {
    	maxfds1 = sockfd + 1;
    }

	clilen = sizeof(cli_addr);
	/*
	 * FOR LOOP to fork as many routers as specified in the config file
	 */
	for(int n = 0; n < numRouter; n++)
	{
		pid = fork(); //fork a router based on number of router
		noRouter++; //Update number of router after each forking
		if(pid < 0)
		{
			error ("ERROR on fork");
		}
		else if(pid == 0) //in CHILD process
		{
			communicateToServerStage2(portno,noRouter);
			close(sockfd);
		}
		else //in PARENT process
		{
			printf("Waiting on port %d\n", portno);
			data = recvfrom(sockfd,buffer,sizeof(struct payload),0,(struct sockaddr*) &cli_addr, &clilen);

			if(data < 0)
			{
				printf ("ERROR receiving message in port %d\n",portno);
			}
			else
			{
				p = (struct payload*)buffer;
			}
			routerPort = ntohs(cli_addr.sin_port);
			writeInfoToFileStage2Part1(fp,portno,p->router,p->pid,routerPort);

			//WHILE LOOP to keep reading from tunnel
			while(1)
			{
			    int sel_return;
			    fd_set readfds;

			    FD_ZERO(&readfds); //initialize the set
			    FD_SET(tun_fd, &readfds); //add tunnel fd to fd_set
			    FD_SET(sockfd, &readfds); //add sockfd to fd_set

			    /*
			     * Waiting for connection from tunnel or router. If after 7 seconds with no communication, function exits
			     * and terminates program
			     */
				sel_return = select(maxfds1,&readfds,NULL,NULL,&tv);

				if(sel_return < 0)
				{
					printf("ERROR in selecting \n");
					exit(1);
				}
				else if(sel_return == 0)
				{
					printf("After 7 seconds without any communication. Function exits, program is terminated\n");
					kill(p->pid,SIGKILL);
					break;
				}
				if(FD_ISSET(tun_fd, &readfds))
				{
					/*
					 * reset timeout
					 */
					tv.tv_sec = 7;
					tv.tv_usec = 0;

					/* Now read data coming from the tunnel */
					int nread = read(tun_fd,buffer_tun,sizeof(buffer_tun));
					if(nread < 0)
					{
						perror("Reading from tunnel interface");
						close(tun_fd);
						exit(-1);
					}
					else
					{
						printf("\nRead a packet from tunnel, packet length:%d\n", nread);

						/*
						 * forward to router via proxy
						 */
						printf("NOW FORWARD ICMP PACKET TO ROUTER NUMBER: %d\n",p->router);
						//send icmp packet to router
						data = sendto(sockfd,buffer_tun,nread,0,(struct sockaddr*) &cli_addr,sizeof(cli_addr));
						if(data < 0)
						{
							printf ("ERROR sending icmp echo message from proxy to router\n");
						}

						/*
						 * ip header info
						 */
						ip = (struct iphdr*) buffer_tun;
						inet_ntop(AF_INET,&ip->saddr,saddr,sizeof(saddr));
						inet_ntop(AF_INET,&ip->daddr,daddr,sizeof(daddr));

						/*
						 * icmp header info
						 */
						icmp = (struct icmphdr*) (buffer_tun+sizeof(struct iphdr));
						/*
						 * write to file
						 */
						writeInfoToFileStage2Part2(fp, routerPort,"tunnel", saddr, daddr,icmp->type);
					}
				}
				if(FD_ISSET(sockfd, &readfds))
				{
					/*
					 * reset timeout
					 */
					tv.tv_sec = 7;
					tv.tv_usec = 0;

					/*
					 * data coming to proxy from router for ICMP reply packet
					 */
					printf("Ready to receive ICMP REPLY packet from router...\n");
					data = recvfrom(sockfd,icmp_reply_buf,160,0,(struct sockaddr*) &cli_addr, &clilen);

					if(data < 0)
					{
						printf ("ERROR receiving message in port %d\n",portno);
					}
					else
					{
						printf("RECEIVED ICMP REPLY PACKET FROM ROUTER NUMBER %d with pid %d\n",p->router,p->pid);
						/*
						 * ip header info
						 */
						ip = (struct iphdr*) icmp_reply_buf;
						inet_ntop(AF_INET,&ip->saddr,saddr,sizeof(saddr));
						inet_ntop(AF_INET,&ip->daddr,daddr,sizeof(daddr));

						/*
						 * icmp header info
						 */
						icmp = (struct icmphdr*) (icmp_reply_buf+sizeof(struct iphdr));

						/*
						 * write to file
						 */
						writeInfoToFileStage2Part2(fp, routerPort,"port", saddr, daddr,icmp->type);

						/*
						 * write back to tunnel
						 */
						printf("Writing ICMP REPLY packet back to tunnel interface from proxy\n");
						int nwrite = write(tun_fd,icmp_reply_buf,data);
						if(nwrite < 0)
						{
							error("Writing to tunnel interface");
							close(tun_fd);
							exit(-1);
						}
					}
				}
			}
		}
	}
	fclose(fp);
	free(buffer);
	close(tun_fd);
	close(sockfd);
}
/*
 * write info to config file for stage 1
 */
void writeInfoToFileStage1(int portNum, int routerNum,int rPid,int rPort)
{
	FILE *fp = fopen("stage1.proxy.out","w");
	if(fp == NULL)
	{
		error ("Can't open file for write");
	}
	fprintf(fp,"proxy port: %d\nrouter: %d, pid: %d, port: %d\n",portNum,routerNum,rPid,rPort);
	fclose(fp);
}

/*
 * write info to config file for stage 2 part 1
 */
void writeInfoToFileStage2Part1(FILE *fp1, int portNum, int routerNum,int rPid,int rPort)
{
	fprintf(fp1,"proxy port: %d\nrouter: %d, pid: %d, port: %d\n",portNum,routerNum,rPid,rPort);
}

/*
 * write info to config file for stage 2 part 2
 */
void writeInfoToFileStage2Part2(FILE *fp1, int rPort,char *from, char *src,char *dest,int type)
{
	if(strncmp (from, "port",4) == 0)
	{
		fprintf(fp1,"ICMP from %s: %d, src: %s, dst: %s, type: %d\n",from,rPort,src,dest,type);
	}
	else
	{
		fprintf(fp1,"ICMP from %s, src: %s, dst: %s, type: %d\n",from,src,dest,type);
	}
}

/*
 * Set up tunnel
 */
int tun_alloc(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;
    char *clonedev = (char*)"/dev/net/tun";

    if( (fd = open(clonedev , O_RDWR)) < 0 )
    {
    	perror("Opening /dev/net/tun");
    	return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if (*dev)
    {
    	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 )
    {
    	perror("ioctl(TUNSETIFF)");
    	close(fd);
    	return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}


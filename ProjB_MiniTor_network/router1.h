/*
 * router1.h
 *
 *  Created on: Sep 17, 2014
 *      Author: csci551
 */

#ifndef ROUTER1_H_
#define ROUTER1_H_

#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

void communicateToServerStage1(int portNum, int numRouter);
void communicateToServerStage2(int portNum, int numRouter);
void communicateToServerStage3(int portNum, int numRouter, char* eth_ip);
void communicateToServerStage4(int portNum, int numRouter, char* eth_ip);
void communicateToServerStage5(int portNum, int numRouter, char* eth_ip);
void communicateToServerStage6(int portNum, int numRouter, char* eth_ip);
void writeInfoToFileRouterStage1(int router, int pidNum,int serPort);
void writeInfoToFileRouterStage2Part1(FILE *fp1,int router, int pidNum,int serPort);
void writeInfoToFileRouterStage2Part2(FILE *fp1, char *from,int cliPort,char* sourceAddr,char* destAddr,int type);
void writeInfoToFileRouterStage5Part1(FILE *fp1,int router, int pidNum,int serPort,char* rIP);
void writeInfoToFileRouterStage5Part2 (FILE *fp1, unsigned char *content_arr, int arr_size);
/*
 * in_cksum --
 * Checksum routine for Internet Protocol
 * family headers (C Version)
 * CODE RE-USE FROM MIKE MUUSS WHO IS THE AUTHOR OF PING.C
 * SOURCE: https://www.cs.utah.edu/~swalton/listings/sockets/programs/part4/chap18/ping.c
 */
unsigned short in_cksum(unsigned short *addr, int len);
#endif /* ROUTER1_H_ */

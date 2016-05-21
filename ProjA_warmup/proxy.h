/*
 * proxy.h
 *
 *  Created on: Sep 17, 2014
 *      Author: csci551
 */

#ifndef PROXY_H_
#define PROXY_H_

#include <netinet/in.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Define functions
void error(char* message);
void createCommunicationStage1(int numRouter);
void createCommunicationStage2(int numRouter);
void writeInfoToFileStage1(int portNum, int routerNum,int rPid,int rPort);
void writeInfoToFileStage2Part1(FILE *fp1, int portNum, int routerNum,int rPid,int rPort);
void writeInfoToFileStage2Part2(FILE *fp1, int rPort,char *from, char *src,char *dest,int type);
int tun_alloc(char *dev, int flags);
#endif /* PROXY_H_ */

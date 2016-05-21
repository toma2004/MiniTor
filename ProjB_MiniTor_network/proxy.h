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
void createCommunicationStage2and3(int stageNum, int numRouter);
void createCommunicationStage4(int stageNum, int numRouter);
void createCommunicationStage5(int stageNum, int numRouter, int numHop);
void createCommunicationStage6(int stageNum, int numRouter, int numHop);
void writeInfoToFileStage1(int portNum, int routerNum,int rPid,int rPort);
void writeInfoToFileStage2Part1(FILE *fp1, int portNum, int routerNum,int rPid,int rPort);
void writeInfoToFileStage2Part2(FILE *fp1, int rPort,char *from, char *src,char *dest,int type);
void writeInfoToFileStage4Part1(FILE *fp1, int portNum);
void writeInfoToFileStage4Part2(FILE *fp1, int routerNum,int rPid,int rPort);
void writeInfoToFileStage5Part1(FILE *fp1, int routerNum,int rPid,int rPort, char* rIP);
void writeInfoToFileStage5Part2 (FILE *fp1, unsigned char *content_arr, int arr_size);
int tun_alloc(char *dev, int flags);
#endif /* PROXY_H_ */

/*
 * main.c
 *
 *  Created on: Sep 16, 2014
 *      Author: Nguyen Tran
 */

#include "proxy.h"


int main (int argc, char** argv)
{
	FILE *fp = NULL;

	if(argc < 1)
	{
		fprintf(stderr,"Please specify the config file");
	}
	else
	{
		fp = fopen(argv[1],"r");
		if(fp == NULL)
		{
			fprintf(stderr,"Cannot open %s for reading",argv[1]);
			return 0;
		}
		char myWord[30];
		char myLine [256];

		int myStageNum = 0;
		int myRouterNum = 0;
		int myHop = 0;
		int myPacketsTillDie = 0;
		int lastNum = 4; //neither stage number or router number or minihop or die_after

		int k = 0;
		while(fgets(myLine,sizeof(myLine),fp) != NULL) //read line by line
		{

			if(myLine[0] == '#') //ignore comments
			{
				continue;
			}
			else
			{
				for(int i=0; i < (int) sizeof(myLine);i++) //for each line read word by word
				{
					if(myLine[i] == '\n' || myLine[i] == '\0') //check last word in a line
					{
						if(lastNum == 0)
						{
							//assign myStageNum
							myStageNum = atoi(myWord);
							lastNum = 4;
						}
						else if(lastNum == 1)
						{
							//assign myRouterNum
							myRouterNum = atoi(myWord);
							lastNum = 4;
						}
						else if(lastNum == 2)
						{
							myHop = atoi (myWord);
							lastNum = 4;
						}
						else if(lastNum == 3)
						{
							myPacketsTillDie = atoi (myWord);
							lastNum = 4;
						}
						memset(myWord,0,sizeof(myWord)); //empty myWord char array to re-use
						k=0;	//reset index of myWord char array
						break;
					}
					if(myLine[i] != ' '&& myLine[i] != '\t')
					{
						myWord[k] = myLine[i];
						k++;
					}
					else
					{
						myWord[k] = '\0';
						if(strcmp(myWord,"stage")==0)
						{
							lastNum = 0;
						}
						else if(strcmp(myWord,"num_routers")==0)
						{
							lastNum = 1;
						}
						else if(strcmp(myWord,"minitor_hops") == 0)
						{
							lastNum = 2;
						}
						else if(strcmp(myWord,"die_after") == 0)
						{
							lastNum = 3;
						}
						memset(myWord,0,sizeof(myWord)); //empty myWord char array to re-use
						k=0; //reset index of myWord char array
					}
				}
			}
		}
		fclose(fp);
		//call function to pass in stage number and router number
		if(myStageNum == 1)
		{
			createCommunicationStage1(myRouterNum);
		}
		else if(myStageNum == 2 || myStageNum == 3)
		{
			createCommunicationStage2and3(myStageNum, myRouterNum);
		}
		else if(myStageNum == 4)
		{
			createCommunicationStage4(myStageNum, myRouterNum);
		}
		else if(myStageNum == 5)
		{
			createCommunicationStage5(myStageNum, myRouterNum, myHop);
		}
		else if(myStageNum == 6)
		{
			createCommunicationStage6(myStageNum, myRouterNum, myHop);
		}
		else if(myStageNum == 7)
		{
			createCommunicationStage7(myStageNum, myRouterNum, myHop);
		}
		else if(myStageNum == 8)
		{
			createCommunicationStage8(myStageNum, myRouterNum, myHop);
		}
		else if(myStageNum == 9)
		{
			createCommunicationStage9(myStageNum, myRouterNum, myHop, myPacketsTillDie);
		}
	}
	return 0;
}


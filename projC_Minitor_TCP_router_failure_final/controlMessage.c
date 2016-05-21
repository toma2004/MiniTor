/*
 * controlMessage.c
 *
 *  Created on: Nov 24, 2014
 *      Author: Nguyen Tran
 */
#include "controlMessage.h"

struct flowCheckReturn checkifmatch (char* src_addr,char* dst_addr,int src_port,int dst_port,int protocol, struct flowCheck** arr_flow, int arr_size)
{
	struct flowCheckReturn check_return;
	check_return.isMatch = 0;
	for (int i = 0; i < arr_size; i++)
	{
		if(strncmp (src_addr,arr_flow[i]->src_addr,16) == 0 && strncmp(dst_addr,arr_flow[i]->dest_addr,16) == 0 && src_port == arr_flow[i]->src_port && dst_port == arr_flow[i]->dest_port && protocol == arr_flow[i]->chk_protocol)
		{
			printf ("Found a match flow in cache!!!\n");
			check_return.isMatch = 1;
			check_return.f_cid = arr_flow[i]->flow_cid;
		}
	}
	return check_return;
}



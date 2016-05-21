/*
 * controlMessage.h
 *
 *  Created on: Oct 14, 2014
 *      Author: Nguyen Tran
 */

#ifndef CONTROLMESSAGE_H_
#define CONTROLMESSAGE_H_
#include <linux/ip.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

/*
 * Payload struct
 */
struct ipPayload
{
	uint8_t mType;
	uint16_t cid;
	char* payload;
};

/*
 * control message struct for Minitor
 */
struct controlMessage
{
	struct iphdr cmIphdr;
	struct ipPayload cmIpPayload;
};

/*
 * struct for router-worried message
 */
struct router_worried
{
	int self_port;
	int next_port;
	int circuit_cidout;
};

/*
 * Struct to define flow
 */
struct flowCheck
{
	char src_addr[16];
	char dest_addr[16];
	int src_port;
	int dest_port;
	int chk_protocol;
	int flow_cid;
};

struct flowCheckReturn
{
	int isMatch;
	int f_cid;
};
struct flowCheckReturn checkifmatch (char* src_addr,char* dst_addr,int src_port,int dst_port,int protocol, struct flowCheck** arr_flow, int arr_size);

#endif /* CONTROLMESSAGE_H_ */

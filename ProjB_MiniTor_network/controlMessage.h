/*
 * controlMessage.h
 *
 *  Created on: Oct 14, 2014
 *      Author: csci551
 */

#ifndef CONTROLMESSAGE_H_
#define CONTROLMESSAGE_H_
#include <linux/ip.h>

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

#endif /* CONTROLMESSAGE_H_ */

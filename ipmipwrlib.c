/*
  Library for quering IPMI power meter which can be found e.g. on HP Proliants.
  Copyright (C) 2014 Yarda <jskarvad@redhat.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the.
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
/*  License for the IPMI stuff that was taken from the ipmitool: */
/*
 * Copyright (C) 2008 Intel Corporation.
 * All rights reserved
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

/*
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#ifndef IPMIPWRLIB_H
#define IPMIPWRLIB_H
#include "ipmipwrlib.h"
#endif

#define IPMI_DCMI			0xDC
#define IPMI_DCMI_GETRED		0x02

#define IPMI_NETFN_APP			0x6
#define IPMI_NETFN_DCGRP		0x2C

#define IPMI_BMC_SLAVE_ADDR		0x20

#define IPMI_BUF_SIZE 1024

#define IPMI_MAX_ADDR_SIZE		0x20
#define IPMI_BMC_CHANNEL		0xf
#define IPMI_NUM_CHANNELS		0x10

#define IPMI_SYSTEM_INTERFACE_ADDR_TYPE	0x0c
#define IPMI_IPMB_ADDR_TYPE		0x01
#define IPMI_IPMB_BROADCAST_ADDR_TYPE	0x41

#define IPMI_RESPONSE_RECV_TYPE		1
#define IPMI_ASYNC_EVENT_RECV_TYPE	2
#define IPMI_CMD_RECV_TYPE		3

#define IPMI_IOC_MAGIC			'i'
#define IPMICTL_RECEIVE_MSG_TRUNC	_IOWR(IPMI_IOC_MAGIC, 11, struct ipmi_recv)
#define IPMICTL_RECEIVE_MSG		_IOWR(IPMI_IOC_MAGIC, 12, struct ipmi_recv)
#define IPMICTL_SEND_COMMAND		_IOR(IPMI_IOC_MAGIC, 13, struct ipmi_req)
#define IPMICTL_REGISTER_FOR_CMD	_IOR(IPMI_IOC_MAGIC, 14, struct ipmi_cmdspec)
#define IPMICTL_UNREGISTER_FOR_CMD	_IOR(IPMI_IOC_MAGIC, 15, struct ipmi_cmdspec)
#define IPMICTL_SET_GETS_EVENTS_CMD	_IOR(IPMI_IOC_MAGIC, 16, int)
#define IPMICTL_SET_MY_ADDRESS_CMD	_IOR(IPMI_IOC_MAGIC, 17, unsigned int)
#define IPMICTL_GET_MY_ADDRESS_CMD	_IOR(IPMI_IOC_MAGIC, 18, unsigned int)
#define IPMICTL_SET_MY_LUN_CMD		_IOR(IPMI_IOC_MAGIC, 19, unsigned int)
#define IPMICTL_GET_MY_LUN_CMD		_IOR(IPMI_IOC_MAGIC, 20, unsigned int)

struct ipmi_addr {
	int addr_type;
	short channel;
	char data[IPMI_MAX_ADDR_SIZE];
};

struct ipmi_msg {
	unsigned char netfn;
        unsigned char cmd;
        unsigned short data_len;
        unsigned char *data;
};

struct ipmi_req {
	unsigned char *addr;
	unsigned int addr_len;
	long msgid;
	struct ipmi_msg msg;
};

struct ipmi_recv {
	int recv_type;
	unsigned char *addr;
	unsigned int addr_len;
	long msgid;
	struct ipmi_msg msg;
};

struct ipmi_cmdspec {
	unsigned char netfn;
	unsigned char cmd;
};

struct ipmi_system_interface_addr {
	int addr_type;
	short channel;
	unsigned char lun;
};

struct ipmi_ipmb_addr {
	int addr_type;
	short channel;
	unsigned char slave_addr;
	unsigned char lun;
};

struct power_reading {
    uint8_t grp_id; /* first byte: Group Extension ID */
    uint16_t curr_pwr;
    uint16_t min_sample;
    uint16_t max_sample;
    uint16_t avg_pwr;
    uint32_t time_stamp; /* time since epoch */
    uint32_t sample;
    uint8_t state;
} __attribute__ ((packed));

struct ipmi_rq {
	struct {
		uint8_t netfn:6;
		uint8_t lun:2;
		uint8_t cmd;
		uint8_t target_cmd;
		uint16_t data_len;
		uint8_t *data;
	} msg;
};

struct ipmi_v2_payload {
	uint16_t payload_length;
	uint8_t payload_type;

	union {

		struct {
			uint8_t rq_seq;
			struct ipmi_rq *request;
		} ipmi_request;

		struct {
			uint8_t rs_seq;
			struct ipmi_rs *response;
		} ipmi_response;

		/* Only used internally by the lanplus interface */
		struct {
			uint8_t *request;
		} open_session_request;

		/* Only used internally by the lanplus interface */
		struct {
			uint8_t *message;
		} rakp_1_message;

		/* Only used internally by the lanplus interface */
		struct {
			uint8_t *message;
		} rakp_2_message;

		/* Only used internally by the lanplus interface */
		struct {
			uint8_t *message;
		} rakp_3_message;

		/* Only used internally by the lanplus interface */
		struct {
			uint8_t *message;
		} rakp_4_message;

		struct {
			uint8_t data[IPMI_BUF_SIZE];
			uint16_t character_count;
			uint8_t packet_sequence_number;
			uint8_t acked_packet_number;
			uint8_t accepted_character_count;
			uint8_t is_nack;	/* bool */
			uint8_t assert_ring_wor;	/* bool */
			uint8_t generate_break;	/* bool */
			uint8_t deassert_cts;	/* bool */
			uint8_t deassert_dcd_dsr;	/* bool */
			uint8_t flush_inbound;	/* bool */
			uint8_t flush_outbound;	/* bool */
		} sol_packet;

	} payload;
};

struct ipmi_rs {
	uint8_t ccode;
	uint8_t data[IPMI_BUF_SIZE];

	/*
	 * Looks like this is the length of the entire packet, including the RMCP
	 * stuff, then modified to be the length of the extra IPMI message data
	 */
	int data_len;

	struct {
		uint8_t netfn;
		uint8_t cmd;
		uint8_t seq;
		uint8_t lun;
	} msg;

	struct {
		uint8_t authtype;
		uint32_t seq;
		uint32_t id;
		uint8_t bEncrypted;	/* IPMI v2 only */
		uint8_t bAuthenticated;	/* IPMI v2 only */
		uint8_t payloadtype;	/* IPMI v2 only */
		/* This is the total length of the payload or
		   IPMI message.  IPMI v2.0 requires this to
		   be 2 bytes.  Not really used for much. */
		uint16_t msglen;
	} session;

	/*
	 * A union of the different possible payload meta-data
	 */
	union {
		struct {
			uint8_t rq_addr;
			uint8_t netfn;
			uint8_t rq_lun;
			uint8_t rs_addr;
			uint8_t rq_seq;
			uint8_t rs_lun;
			uint8_t cmd;
		} ipmi_response;
		struct {
			uint8_t message_tag;
			uint8_t rakp_return_code;
			uint8_t max_priv_level;
			uint32_t console_id;
			uint32_t bmc_id;
			uint8_t auth_alg;
			uint8_t integrity_alg;
			uint8_t crypt_alg;
		} open_session_response;
		struct {
			uint8_t message_tag;
			uint8_t rakp_return_code;
			uint32_t console_id;
			uint8_t bmc_rand[16];	/* Random number generated by the BMC */
			uint8_t bmc_guid[16];
			uint8_t key_exchange_auth_code[20];
		} rakp2_message;
		struct {
			uint8_t message_tag;
			uint8_t rakp_return_code;
			uint32_t console_id;
			uint8_t integrity_check_value[20];
		} rakp4_message;
		struct {
			uint8_t packet_sequence_number;
			uint8_t acked_packet_number;
			uint8_t accepted_character_count;
			uint8_t is_nack;	/* bool */
			uint8_t transfer_unavailable;	/* bool */
			uint8_t sol_inactive;	/* bool */
			uint8_t transmit_overrun;	/* bool */
			uint8_t break_detected;	/* bool */
		} sol_packet;

	} payload;
};

struct valstr {
	uint16_t val;
	const char * str;
};
const struct valstr dcmi_ccode_vals[] = {
	{ 0x80, "Parameter not supported" },
	{ 0x81, "Something else has already claimed these parameters" },
	{ 0x82, "Not supported or failed to write a read-only parameter" },
	{ 0x83, "Access mode is not supported" },
	{ 0x84, "Power/Thermal limit out of range" },
	{ 0x85, "Correction/Exception time out of range" },
	{ 0x89, "Sample/Statistics Reporting period out of range" },
	{ 0x8A, "Power limit already active" },
	{ 0xFF, NULL }
};

const struct valstr completion_code_vals[] = {
	{ 0x00, "Command completed normally" },
	{ 0xc0, "Node busy" },
	{ 0xc1, "Invalid command" },
	{ 0xc2, "Invalid command on LUN" },
	{ 0xc3, "Timeout" },
	{ 0xc4, "Out of space" },
	{ 0xc5, "Reservation cancelled or invalid" },
	{ 0xc6, "Request data truncated" },
	{ 0xc7, "Request data length invalid" },
	{ 0xc8, "Request data field length limit exceeded" },
	{ 0xc9, "Parameter out of range" },
	{ 0xca, "Cannot return number of requested data bytes" },
	{ 0xcb, "Requested sensor, data, or record not found" },
	{ 0xcc, "Invalid data field in request" },
	{ 0xcd, "Command illegal for specified sensor or record type" },
	{ 0xce, "Command response could not be provided" },
	{ 0xcf, "Cannot execute duplicated request" },
	{ 0xd0, "SDR Repository in update mode" },
	{ 0xd1, "Device firmeware in update mode" },
	{ 0xd2, "BMC initialization in progress" },
	{ 0xd3, "Destination unavailable" },
	{ 0xd4, "Insufficient privilege level" },
	{ 0xd5, "Command not supported in present state" },
	{ 0xd6, "Cannot execute command, command disabled" },
	{ 0xff, "Unspecified error" },
	{ 0x00, NULL }
};

static const char * val2str(uint16_t val, const struct valstr *vs)
{
	static char un_str[32];
	int i;

	for (i = 0; vs[i].str != NULL; i++) {
		if (vs[i].val == val)
			return vs[i].str;
	}

	memset(un_str, 0, 32);
	snprintf(un_str, 32, "Unknown (0x%02X)", val);

	return un_str;
}

static int chk_rsp(struct ipmi_rs * rsp)
{
	/* if the response from the ipmi is NULL then the BMC is experiencing
	 * some issue and cannot complete the command
	 */
	if (rsp == NULL) {
		    printf("\n    Unable to get DCMI information");
		return 1;
	}
	/* if the completion code is greater than zero there was an error.  We'll
	 * use val2str from helper.c to print the error from either the DCMI
	 * completion code struct or the generic IPMI completion_code_vals struct
	 */
	if ((rsp->ccode >= 0x80) && (rsp->ccode <= 0x8F)) {
		printf("\n    DCMI request failed because: %s (%x)",
				val2str(rsp->ccode, dcmi_ccode_vals), rsp->ccode);
		return 1;
	} else if (rsp->ccode > 0) {
		printf("\n    DCMI request failed because: %s (%x)",
				val2str(rsp->ccode, completion_code_vals), rsp->ccode);
		return 1;
	}
	/* check to make sure this is a DCMI firmware */
	if(rsp->data[0] != IPMI_DCMI) {
		printf("\n    A valid DCMI command was not returned! (%x)", rsp->data[0]);
		return 1;
	}
 	return 0;
}

static int openipmi_set_my_addr(tipmi *ipmi, uint8_t addr)
{
	unsigned int a = addr;

	if (ipmi == NULL)
		return -1;

	if (ioctl(ipmi->fd, IPMICTL_SET_MY_ADDRESS_CMD, &a) < 0) {
		printf("Could not set IPMB address");
		return -1;
	}
	ipmi->my_addr = addr;
	return 0;
}

int openipmi_open(tipmi *ipmi)
{
	int i = 0;
    
	char ipmi_dev[16];
	char ipmi_devfs[16];
	char ipmi_devfs2[16];
	int devnum = 0;

	if (ipmi == NULL)
		return -1;

	sprintf(ipmi_dev, "/dev/ipmi%d", devnum);
	sprintf(ipmi_devfs, "/dev/ipmi/%d", devnum);
	sprintf(ipmi_devfs2, "/dev/ipmidev/%d", devnum);

	if (!ipmi->my_addr)
		ipmi->my_addr = IPMI_BMC_SLAVE_ADDR;

	ipmi->fd = open(ipmi_dev, O_RDWR);

	if (ipmi->fd < 0) {
		ipmi->fd = open(ipmi_devfs, O_RDWR);
		if (ipmi->fd < 0) {
			ipmi->fd = open(ipmi_devfs2, O_RDWR);
		}
		if (ipmi->fd < 0) {
			printf("Could not open device at %s or %s or %s",
			ipmi_dev, ipmi_devfs , ipmi_devfs2);
			return -1;
		}
	}

	if (ioctl(ipmi->fd, IPMICTL_SET_GETS_EVENTS_CMD, &i) < 0) {
		printf("Could not enable event receiver");
		return -1;
	}
 
	ipmi->opened = 1;

   /* This is never set to 0, the default is IPMI_BMC_SLAVE_ADDR */
	if (ipmi->my_addr != 0) {
		if (openipmi_set_my_addr(ipmi, ipmi->my_addr) < 0) {
			printf("Could not set IPMB address");
			return -1;
		}
	}

	return ipmi->fd;
}

void openipmi_close(tipmi *ipmi)
{
	if (ipmi == NULL)
		return;

	if (ipmi->fd >= 0) {
		close(ipmi->fd);
		ipmi->fd = -1;
	}

	ipmi->opened = 0;
}

static struct ipmi_rs * openipmi_send_cmd(tipmi *ipmi, struct ipmi_rq * req)
{
	struct ipmi_recv recv;
	struct ipmi_addr addr;
	struct ipmi_system_interface_addr bmc_addr = {
		addr_type:	IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
		channel:	IPMI_BMC_CHANNEL,
	};
	struct ipmi_req _req;
	static struct ipmi_rs rsp;
	static int curr_seq = 0;
	fd_set rset;

	uint8_t * data = NULL;
	int data_len = 0;


	if (ipmi == NULL || req == NULL)
		return NULL;

	if (ipmi->opened == 0)
		if (openipmi_open(ipmi) < 0)
			return NULL;

	/*
	 * setup and send message
	 */

	memset(&_req, 0, sizeof(struct ipmi_req));

	/* use system interface */
	bmc_addr.lun = req->msg.lun;
	_req.addr = (unsigned char *) &bmc_addr;
	_req.addr_len = sizeof(bmc_addr);

	_req.msgid = curr_seq++;

	/* In case of a bridge request */
	if( data != NULL && data_len != 0 ) {
	   _req.msg.data = data;
	   _req.msg.data_len = data_len;
	   _req.msg.netfn = IPMI_NETFN_APP;
	   _req.msg.cmd   = 0x34;

	} else {
	   _req.msg.data = req->msg.data;
	   _req.msg.data_len = req->msg.data_len;
	   _req.msg.netfn = req->msg.netfn;
	   _req.msg.cmd = req->msg.cmd;
	}
   
	if (ioctl(ipmi->fd, IPMICTL_SEND_COMMAND, &_req) < 0) {
	   printf("Unable to send command");
	   if (data != NULL) {
	      free(data);
				data = NULL;
		 }
	   return NULL;
	}

	/*
	 * wait for and retrieve response
	 */

	if (ipmi->noanswer) {
	   if (data != NULL) {
	      free(data);
				data = NULL;
		 }
	   return NULL;
	}

	FD_ZERO(&rset);
	FD_SET(ipmi->fd, &rset);

	if (select(ipmi->fd+1, &rset, NULL, NULL, NULL) < 0) {
	   printf("I/O Error");
	   if (data != NULL) {
	      free(data);
				data = NULL;
		 }
	   return NULL;
	}
	if (FD_ISSET(ipmi->fd, &rset) == 0) {
	   printf("No data available");
	   if (data != NULL) {
	      free(data);
				data = NULL;
		 }
	   return NULL;
	}

	recv.addr = (unsigned char *) &addr;
	recv.addr_len = sizeof(addr);
	recv.msg.data = rsp.data;
	recv.msg.data_len = sizeof(rsp.data);

	/* get data */
	if (ioctl(ipmi->fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv) < 0) {
	   printf("Error receiving message");
	   if (errno != EMSGSIZE) {
	      if (data != NULL) {
					free(data);
					data = NULL;
				}
	      return NULL;
	   }
	}

	if(ipmi->transit_addr != 0 && ipmi->transit_addr != ipmi->my_addr) {
	   /* comp code */
	   /* Check data */

	   if( recv.msg.data[0] == 0 ) {
	      recv.msg.netfn = recv.msg.data[2] >> 2;
	      recv.msg.cmd   = recv.msg.data[6];

	      recv.msg.data = (unsigned char *) memmove(recv.msg.data ,recv.msg.data + 7 , recv.msg.data_len - 7);
	      recv.msg.data_len -= 8;

	   }
	}

	/* save completion code */
	rsp.ccode = recv.msg.data[0];
	rsp.data_len = recv.msg.data_len - 1;

	/* save response data for caller */
	if (rsp.ccode == 0 && rsp.data_len > 0) {
	   memmove(rsp.data, rsp.data + 1, rsp.data_len);
	   rsp.data[recv.msg.data_len] = 0;
	}

	if (data != NULL) {
	   free(data);
		 data = NULL;
	}

	return &rsp;
}

int openipmi_pwr_rd(tipmi *ipmi, tpwr *pwr)
{
	struct ipmi_rs * rsp;
	struct ipmi_rq req;
	struct power_reading val;
	struct tm tm_t;
	time_t t;
	uint8_t msg_data[4]; /* number of request data bytes */

	if (ipmi == NULL || pwr == NULL)
		return -1;
		
	memset(&tm_t, 0, sizeof(tm_t));
	memset(&t, 0, sizeof(t));

	msg_data[0] = IPMI_DCMI; /* Group Extension Identification */
	msg_data[1] = 0x01; /* Mode Power Status */
	msg_data[2] = 0x00; /* reserved */
	msg_data[3] = 0x00; /* reserved */

	memset(&req, 0, sizeof(req));
	req.msg.netfn = IPMI_NETFN_DCGRP;
	req.msg.cmd = IPMI_DCMI_GETRED; /* Get power reading */
	req.msg.data = msg_data; /* msg_data above */
	req.msg.data_len = 4; /* how many times does req.msg.data need to read */

	rsp = openipmi_send_cmd(ipmi, &req);

	if (chk_rsp(rsp)) {
		return -1;
	}
	/* rsp->data[0] is equal to response data byte 2 in spec */
	/* printf("Group Extension Identification: %02x\n", rsp->data[0]); */
	memcpy(&val, rsp->data, sizeof (val));
	t = val.time_stamp;
	gmtime_r(&t, &tm_t);

	pwr->curr = val.curr_pwr;
	pwr->min = val.min_sample;
	pwr->max = val.max_sample;
	pwr->avg = val.avg_pwr;
	memcpy((void *)&(pwr->timestamp), (void *)&tm_t, sizeof(tm_t));
	pwr->samp_period = val.sample;
	pwr->state = (val.state & 0x40) == 0x40;
	return 0;
}

/**
* @file
* Ping sender module
*
*/

/*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
* This file is part of the lwIP TCP/IP stack.
*
*/

/**
* This is an example of a "ping" sender (with raw API and socket API).
* It can be used as a start point to maintain opened a network connection, or
* like a network "watchdog" for your device.
*
*/

#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <lwipopts.h>
#include "lwip/opt.h"

#include "ping.h"

#include <arch/sys_arch.h>
#include <lwip/mem.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/netif.h>
#include <lwip/sys.h>
//#include <lwip/timers.h>
#include <lwip/inet_chksum.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

//#ifdef ESP_LWIP
#include "esp_ping.h"
#include <lwip/ip4_addr.h>
//#endif
#include <stdint.h>
/**
* PING_DEBUG: Enable debugging for PING.
*/
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_OFF
#endif

/** ping target - should be an "ip4_addr_t" */
#ifndef PING_TARGET
#define PING_TARGET   (netif_default ? *netif_ip4_gw(netif_default) : (*IP4_ADDR_ANY))
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1250
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

uint8_t CurrentIDPING=0;
static sys_thread_t pingtaskhandle;

bool dofreeping = false;
bool docheckping = false;
uint32_t statusdoping;

/* ping variables */
static u16_t ping_seq_num;
static u32_t ping_time;

static void IRAM_ATTR ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len)
{
	size_t i;
	size_t data_len = len - sizeof(struct icmp_echo_hdr);

	ICMPH_TYPE_SET(iecho, ICMP_ECHO);
	ICMPH_CODE_SET(iecho, 0);
	iecho->chksum = 0;
	iecho->id = PING_ID;
	iecho->seqno = lwip_htons(++ping_seq_num);

	/* fill the additional data buffer with some data */
	for (i = 0; i < data_len; i++) {
		((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
	}

	iecho->chksum = inet_chksum(iecho, len);
}

static err_t IRAM_ATTR ping_send(int s, ip_addr_t *addr)
{
	int err=0;
	struct icmp_echo_hdr *iecho=NULL;
	struct sockaddr_in to; memset(&to, 0, sizeof(struct sockaddr_in));
	size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

	//uint8_t buf[sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE];
	//memset(buf, 0, sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE);
	//iecho = buf;

	LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);
	LWIP_ASSERT("ping: expect IPv4 address", !IP_IS_V6(addr));


	iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
	if (!iecho) {
		return ERR_MEM;
	}

	ping_prepare_echo(iecho, (u16_t)ping_size);

	to.sin_len = sizeof(to);
	to.sin_family = AF_INET;
	inet_addr_from_ip4addr(&to.sin_addr, ip_2_ip4(addr));

	err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

	mem_free(iecho);

	return (err ? ERR_OK : ERR_VAL);
}

static void IRAM_ATTR ping_recv(int s)
{
	char buf[64];
	int len;
	struct sockaddr_in from;
	struct ip_hdr *iphdr;
	struct icmp_echo_hdr *iecho;
	int fromlen = sizeof(from);

	while ((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
		if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
			if (from.sin_family != AF_INET) {
				/* Ping is not IPv4 */
				LWIP_DEBUGF(PING_DEBUG, ("ping: invalid sin_family\n"));
			}
			else {
				ip4_addr_t fromaddr;
				inet_addr_to_ip4addr(&fromaddr, &from.sin_addr);
				LWIP_DEBUGF(PING_DEBUG, ("ping: recv "));
				ip4_addr_debug_print(PING_DEBUG, &fromaddr);
				LWIP_DEBUGF(PING_DEBUG, (" %"U32_F" ms\n", (sys_now() - ping_time)));

				iphdr = (struct ip_hdr *)buf;
				iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
				if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) 
				{
					esp_ping_result(CurrentIDPING,(ICMPH_TYPE(iecho) == ICMP_ER), len, (sys_now() - ping_time));
					bool pingok = true;
					esp_ping_set_target(CurrentIDPING, PING_TARGET_IS, &pingok, sizeof(bool));
					statusdoping = 0;
					return;
				}
				else {
					LWIP_DEBUGF(PING_DEBUG, ("ping: drop\n"));
				}
			}
		}
		fromlen = sizeof(from);
	}

	if (len == 0) {
		LWIP_DEBUGF(PING_DEBUG, ("ping: recv - %"U32_F" ms - timeout\n", (sys_now() - ping_time)));
	}

	esp_ping_result(CurrentIDPING,0, len, (sys_now() - ping_time));
	statusdoping = 0;
}


static void IRAM_ATTR ping_thread(void *arg)
{
	int s;
	int ret;
	ip_addr_t ping_target;
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
	int timeout = PING_RCV_TIMEO;
#else
	struct timeval timeout;
	timeout.tv_sec = PING_RCV_TIMEO / 1000;
	timeout.tv_usec = (PING_RCV_TIMEO % 1000) * 1000;
#endif
	LWIP_UNUSED_ARG(arg);

	if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
		return;
	}

	ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	LWIP_ASSERT("setting receive timeout failed", ret == 0);
	LWIP_UNUSED_ARG(ret);

	while (1) 
	{
		if (docheckping)
		{
			statusdoping++;
		
			ip4_addr_t ipaddr;
			bool pingok = false;

			esp_ping_get_target(CurrentIDPING, PING_TARGET_IP_ADDRESS, &ipaddr.addr, sizeof(uint32_t));
			esp_ping_set_target(CurrentIDPING, PING_TARGET_IS, &pingok, sizeof(bool));


			ip_addr_copy_from_ip4(ping_target, ipaddr);

			if (ping_send(s, &ping_target) == ERR_OK)
			{
				LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
				ip_addr_debug_print(PING_DEBUG, &ping_target);
				LWIP_DEBUGF(PING_DEBUG, ("\n"));

				ping_time = sys_now();
				ping_recv(s);
			}
			else
			{
				LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
				ip_addr_debug_print(PING_DEBUG, &ping_target);
				LWIP_DEBUGF(PING_DEBUG, (" - error\n"));
			}
			

			CurrentIDPING++; if (CurrentIDPING >= PING_COUNT) CurrentIDPING = 0;

			sys_msleep(PING_DELAY);
		}
		else
		{
			sys_msleep(1000);		
		}
	}
}

void ping_init(void)
{
	CurrentIDPING = 0;
	if (pingtaskhandle == NULL)
	{
		pingtaskhandle = sys_thread_new("ping_thread", ping_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
	}
	docheckping = true;
}

void ping_deinit(void)
{

}

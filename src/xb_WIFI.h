#ifndef _XB_WIFI_H
#define _XB_WIFI_H

#include <xb_board.h>
#include <WiFiClient.h>

typedef enum { wasDisconnect, wasConnect } TWiFiAPStatus;
typedef enum { wssDisconnect, wssConnect } TWiFiSTAStatus;
typedef enum { nsDisconnect, nsConnect } TNETStatus;

void TCPClientDestroy(WiFiClient **Awificlient);

extern void WIFI_RESET(void);

extern TWiFiSTAStatus WiFiSTAStatus;
extern TWiFiAPStatus WiFiAPStatus;
extern void NET_Connect();
extern void NET_Disconnect();
extern TNETStatus NETStatus;

extern uint8_t WIFI_mac[6];
extern IPAddress CFG_WIFI_StaticIP_IP;
extern IPAddress CFG_WIFI_MASK_IP;
extern IPAddress CFG_WIFI_GATEWAY_IP;

extern TTaskDef XB_WIFI_DefTask;
#endif

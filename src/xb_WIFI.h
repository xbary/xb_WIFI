#ifndef _XB_WIFI_H
#define _XB_WIFI_H

#include <xb_board.h>
#ifdef XB_PING
#include <xb_PING.h>
#endif
#include <WiFiClient.h>

typedef enum { wasDisconnect, wasConnect } TWiFiAPStatus;
typedef enum { wsDisconnect, wsConnect } TWiFiStatus;
typedef enum { isDisconnect, isConnect } TInternetStatus;

void TCPClientDestroy(WiFiClient **Awificlient);
void WIFI_DoAllTaskWifiDisconnect(void);
void WIFI_DoAllTaskInternetDisconnect(void);
void WIFI_DoAllTaskInternetConnect(void);
void WIFI_DoAllTaskWiFiConnect(void);
void WIFI_SetConnectWiFi(void);
void WIFI_SetConnectInternet(void);
void WIFI_SetDisconnectWiFi(void);
void WIFI_SetDisconnectInternet(void);
bool WIFI_CheckDisconnectWiFi(void);

void WIFI_Setup(void);
uint32_t WIFI_DoLoop(void);
bool WIFI_DoMessage(TMessageBoard *Am);

extern void WIFI_HardDisconnect(void);
extern TTaskDef XB_WIFI_DefTask;
extern TWiFiStatus WiFiStatus;
extern TInternetStatus WIFI_InternetStatus;
extern TWiFiAPStatus WiFiAPStatus;
extern uint8_t WIFI_mac[6];
extern IPAddress CFG_WIFI_StaticIP_IP;

#ifndef XB_PING
extern bool PING_GATEWAY_IS;
extern bool PING_8888_IS;
#endif

#endif

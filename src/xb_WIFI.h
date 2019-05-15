#ifndef _XB_WIFI_H
#define _XB_WIFI_H

#include <xb_board.h>
#include <WiFiClient.h>

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
extern uint8_t WIFI_mac[6];
extern uint32_t CFG_WIFI_StaticIP_IP;
#endif

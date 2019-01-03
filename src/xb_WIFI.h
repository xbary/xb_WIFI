#ifndef _XB_WIFI_H
#define _XB_WIFI_H

#include <xb_board.h>

#ifdef ESP8266
#define ARDUINO_OTA
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <WiFiClient.h>
#endif


typedef enum { wsDisconnect, wsConnect } TWiFiStatus;
typedef enum { isDisconnect, isConnect } TInternetStatus;

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
extern TTaskDef WIFI_DefTask;
extern TWiFiStatus WiFiStatus;
extern TInternetStatus WIFI_InternetStatus;
extern uint8_t WIFI_mac[6];

#endif

#include <xb_board.h>
#include <xb_WIFI.h>

#ifdef ESP32
#include <WiFi.h>
#ifdef XB_OTA
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoOTA.h>
#endif
extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include "lwip/ip_addr.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "esp_ipc.h"

} //extern "C"

#endif

#ifdef ESP8266
extern "C" {
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "cont.h"
};
#endif

typedef enum { 
	wfIDLE, 
	wfResetRadio,
	wfHandleOnlyAP,
	wfStartFindWiFiAP,
	wfFindingWiFiAP,
	wfEndFindWiFiAP,
	wfConnectWiFiAP,
	wfWaitConnectWiFiAP,
	wfHandleSta
} TWiFiFunction;
TWiFiFunction WiFiFunction = wfIDLE;

TWiFiSTAStatus WiFiSTAStatus;
TWiFiAPStatus WiFiAPStatus;
TNETStatus NETStatus;

#ifdef XB_OTA
#ifdef NO_GLOBAL_ARDUINOOTA
ArduinoOTAClass *ArduinoOTA;
#endif
#endif

void NET_Connect()
{
	if (NETStatus == nsConnect) return;
	NETStatus=nsConnect;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_NET_CONNECT;
	board.DoMessageOnAllTask(&mb, true, doFORWARD);
}

void NET_Disconnect()
{
	if (NETStatus == nsDisconnect) return;
	NETStatus = nsDisconnect;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_NET_DISCONNECT;
	board.DoMessageOnAllTask(&mb, true, doBACKWARD);
}

uint8_t WIFI_mac[6];

#ifdef XB_GUI
#include <xb_GUI.h>
#include <xb_GUI_Gadget.h>

TWindowClass *WIFI_winHandle0;
TGADGETMenu *WIFI_menuhandle1;
TGADGETInputDialog *WIFI_inputdialoghandle0_ssid;
TGADGETInputDialog *WIFI_inputdialoghandle1_psw;
TGADGETInputDialog *WIFI_inputdialoghandle2_sta_ip;
TGADGETInputDialog *WIFI_inputdialoghandle3_mask_ip;
TGADGETInputDialog *WIFI_inputdialoghandle4_gateway_ip;
TGADGETInputDialog* WIFI_inputdialoghandle5_dns1_ip;
TGADGETInputDialog* WIFI_inputdialoghandle6_dns2_ip;
TGADGETInputDialog* WIFI_inputdialoghandle7_appsw;
TGADGETInputDialog* WIFI_inputdialoghandle8_pswota;
TGADGETInputDialog* WIFI_inputdialoghandle9_portota;
TGADGETInputDialog* WIFI_inputdialoghandle10_channelap;
#endif

String WIFI_GetString_WiFiFunction(TWiFiFunction Awf)
{
	switch (Awf)
	{
		GET_ENUMSTRING(wfIDLE, 2);
		GET_ENUMSTRING(wfResetRadio, 2);
		GET_ENUMSTRING(wfConnectWiFiAP, 2);
		GET_ENUMSTRING(wfEndFindWiFiAP, 2);
		GET_ENUMSTRING(wfFindingWiFiAP, 2);
		GET_ENUMSTRING(wfHandleOnlyAP, 2);
		GET_ENUMSTRING(wfHandleSta, 2);
		GET_ENUMSTRING(wfStartFindWiFiAP, 2);
		GET_ENUMSTRING(wfWaitConnectWiFiAP, 2);
	}
}

// Konfiguracja -----------------------------------------------------------------------------------------------------
#pragma region KONFIGURACJA


#ifdef WIFI_SSID_DEFAULT
bool CFG_WIFI_UseStation = true;
#else
bool CFG_WIFI_UseStation = false;
#endif
bool CFG_WIFI_UseAp = true;
bool CFG_WIFI_HideAp = false;
uint32_t CFG_WIFI_IntervalTickStartFindWifiAp = 5000;


uint8_t CFG_WIFI_ChannelAp=11;
bool CFG_WIFI_DEBUG = false;
#ifdef XB_OTA
bool CFG_WIFI_USEOTA = true;

String CFG_WIFI_PSWOTA;
#ifndef WIFI_PORTOTA_DEFAULT
#define WIFI_PORTOTA_DEFAULT 8266
#endif
int16_t CFG_WIFI_PORTOTA = WIFI_PORTOTA_DEFAULT;
#endif


#ifndef WIFI_SSID_DEFAULT
#define WIFI_SSID_DEFAULT ""
#endif
String CFG_WIFI_SSID = WIFI_SSID_DEFAULT;

#ifndef WIFI_PASSWORD_DEFAULT
#define WIFI_PASSWORD_DEFAULT ""
#endif
String CFG_WIFI_PSW = WIFI_PASSWORD_DEFAULT;

#ifndef WIFI_STATICIP_DEFAULT
#define WIFI_STATICIP_DEFAULT "0.0.0.0"
#endif
String CFG_WIFI_StaticIP = WIFI_STATICIP_DEFAULT;
IPAddress CFG_WIFI_StaticIP_IP;

#ifndef WIFI_MASK_DEFAULT
#define WIFI_MASK_DEFAULT "0.0.0.0"
#endif
String CFG_WIFI_MASK = WIFI_MASK_DEFAULT;
IPAddress CFG_WIFI_MASK_IP;

#ifndef WIFI_GATEWAY_DEFAULT
#define WIFI_GATEWAY_DEFAULT "0.0.0.0"
#endif
String CFG_WIFI_GATEWAY = WIFI_GATEWAY_DEFAULT;
IPAddress CFG_WIFI_GATEWAY_IP;

#ifndef WIFI_DNS1_DEFAULT
#define WIFI_DNS1_DEFAULT "0.0.0.0"
#endif
String CFG_WIFI_DNS1 = WIFI_DNS1_DEFAULT;
IPAddress CFG_WIFI_DNS1_IP;

#ifndef WIFI_DNS2_DEFAULT
#define WIFI_DNS2_DEFAULT "0.0.0.0"
#endif
String CFG_WIFI_DNS2 = WIFI_DNS2_DEFAULT;
IPAddress CFG_WIFI_DNS2_IP;

#ifndef WIFI_AP_PASSWORD_DEFAULT
#define WIFI_AP_PASSWORD_DEFAULT ""
#endif
String CFG_WIFI_AP_PSW = WIFI_AP_PASSWORD_DEFAULT;


bool WIFI_LoadConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("WIFI"))
	{
		CFG_WIFI_DEBUG = board.PREFERENCES_GetBool("DEBUG", CFG_WIFI_DEBUG);
		
		CFG_WIFI_IntervalTickStartFindWifiAp = board.PREFERENCES_GetUINT32("ITSFWA", CFG_WIFI_IntervalTickStartFindWifiAp);

		CFG_WIFI_UseStation = board.PREFERENCES_GetBool("UseStation", CFG_WIFI_UseStation); 
		CFG_WIFI_UseAp = board.PREFERENCES_GetBool("UseAp", CFG_WIFI_UseAp);
		CFG_WIFI_ChannelAp = board.PREFERENCES_GetUINT8("ChannelAp", CFG_WIFI_ChannelAp);

		CFG_WIFI_SSID = board.PREFERENCES_GetString("SSID", CFG_WIFI_SSID); 
		CFG_WIFI_PSW = board.PREFERENCES_GetString("PSW", CFG_WIFI_PSW); 

		CFG_WIFI_StaticIP = board.PREFERENCES_GetString("StaticIP", CFG_WIFI_StaticIP); 
		{IPAddress ip; ip.fromString(CFG_WIFI_StaticIP); CFG_WIFI_StaticIP_IP = ip; }

		CFG_WIFI_MASK = board.PREFERENCES_GetString("Mask", CFG_WIFI_MASK); 
		{IPAddress ip; ip.fromString(CFG_WIFI_MASK); CFG_WIFI_MASK_IP = ip; }

		CFG_WIFI_GATEWAY = board.PREFERENCES_GetString("Gateway", CFG_WIFI_GATEWAY); 
		{IPAddress ip; ip.fromString(CFG_WIFI_GATEWAY); CFG_WIFI_GATEWAY_IP = ip; }

		CFG_WIFI_DNS1 = board.PREFERENCES_GetString("Dns1", CFG_WIFI_DNS1);
		{IPAddress ip; ip.fromString(CFG_WIFI_DNS1); CFG_WIFI_DNS1_IP = ip; }

		CFG_WIFI_DNS2 = board.PREFERENCES_GetString("Dns2", CFG_WIFI_DNS2);
		{IPAddress ip; ip.fromString(CFG_WIFI_DNS2); CFG_WIFI_DNS2_IP = ip; }

		CFG_WIFI_AP_PSW = board.PREFERENCES_GetString("APPSW", CFG_WIFI_AP_PSW);
		CFG_WIFI_HideAp = board.PREFERENCES_GetBool("HideAp", CFG_WIFI_HideAp);
#ifdef XB_OTA
		CFG_WIFI_USEOTA = board.PREFERENCES_GetBool("USEOTA", CFG_WIFI_USEOTA);
		CFG_WIFI_PSWOTA = board.PREFERENCES_GetString("PSWOTA", CFG_WIFI_PSWOTA);
		CFG_WIFI_PORTOTA = board.PREFERENCES_GetINT16("PORTOTA", CFG_WIFI_PORTOTA);
#endif
	}
	else
	{
		return false;
	}
	board.PREFERENCES_EndSection();
	
	return true;
#else
	return false;
#endif
}

bool WIFI_SaveConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("WIFI"))
	{
		board.PREFERENCES_PutUINT32("ITSFWA", CFG_WIFI_IntervalTickStartFindWifiAp);
		board.PREFERENCES_PutBool("UseStation", CFG_WIFI_UseStation); 
		board.PREFERENCES_PutBool("UseAp", CFG_WIFI_UseAp);
		board.PREFERENCES_PutUINT8("ChannelAp", CFG_WIFI_ChannelAp);
		board.PREFERENCES_PutBool("DEBUG", CFG_WIFI_DEBUG);
		board.PREFERENCES_PutString("SSID", CFG_WIFI_SSID); 
		board.PREFERENCES_PutString("PSW", CFG_WIFI_PSW); 
		board.PREFERENCES_PutString("StaticIP", CFG_WIFI_StaticIP); 
		board.PREFERENCES_PutString("Mask", CFG_WIFI_MASK); 
		board.PREFERENCES_PutString("Gateway", CFG_WIFI_GATEWAY); 
		board.PREFERENCES_PutString("Dns1", CFG_WIFI_DNS1);
		board.PREFERENCES_PutString("Dns2", CFG_WIFI_DNS2);
		board.PREFERENCES_PutString("APPSW", CFG_WIFI_AP_PSW);
		board.PREFERENCES_PutBool("HideAp", CFG_WIFI_HideAp);
#ifdef XB_OTA
		board.PREFERENCES_PutBool("USEOTA", CFG_WIFI_USEOTA);
		board.PREFERENCES_PutString("PSWOTA", CFG_WIFI_PSWOTA);
		board.PREFERENCES_PutINT16("PORTOTA", CFG_WIFI_PORTOTA);
#endif
	}
	else
	{
		return false;
	}
	board.PREFERENCES_EndSection();
	return true;
#else
	return false;
#endif
}

bool WIFI_ResetConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("WIFI"))
	{
		board.PREFERENCES_CLEAR();
	}
	else
	{
		return false;
	}
	board.PREFERENCES_EndSection();
	return true;
#else
	return false;
#endif
}



#pragma endregion

//-------------------------------------------------------------------------------------------------------------
#if defined(wificlient_h) || defined(_WIFICLIENT_H_)


//Insert Into WiFiClient Class
//
//void abort();
//
//void WiFiClient::abort()
//{
//	if (_client) _client->abort();
//}
//*/

void TCPClientDestroy(WiFiClient **Awificlient)
{
	if (Awificlient != NULL)
	{
		if (*Awificlient != NULL)
		{
			delay(50);
			(*Awificlient)->flush();
			#ifdef ESP8266
			delay(50);
			(*Awificlient)->abort();
			#endif
			delay(50);
			delete((*Awificlient));
			(*Awificlient) = NULL;
			delay(50);
		}
	}
}
#endif

#pragma region FUNKCJE_STEROWANIA

#ifdef XB_OTA
void WIFI_OTA_Init(void);
#endif


void WIFI_RESET(void)
{
#ifdef XB_OTA
	if (CFG_WIFI_USEOTA)
	{

#ifdef NO_GLOBAL_ARDUINOOTA
		if (ArduinoOTA != NULL)
		{
			ArduinoOTA->end();
			delete(ArduinoOTA);
			ArduinoOTA = NULL;
		}
#else
		ArduinoOTA.end();
#endif
	}
#endif

	NET_Disconnect();

	int i = 0;
	WiFi.scanDelete();
	delay(100);
	
	if (WiFi.status() == WL_CONNECTED)
	{
		board.Log("Disconnecting and reseting WIFI.", true, true);
		WiFi.disconnect();
		board.Log('.');	delay(10);
	}
	else
	{
		board.Log("Reseting WIFI.", true, true);
		WiFi.disconnect();
		board.Log('.');	delay(10);
	}
	
	board.Log('.'); delay(10);
	WiFi.persistent(false);
	
	WiFi.setAutoConnect(false);
	WiFi.setAutoReconnect(false);
	board.Log('.');	delay(10);
	
	if (CFG_WIFI_UseAp)
	{
		WiFi.softAPsetHostname(board.DeviceName.c_str());
		WiFi.softAP(board.DeviceName.c_str(), CFG_WIFI_AP_PSW.c_str(), CFG_WIFI_ChannelAp,(CFG_WIFI_HideAp?1:0));
		WiFi.enableAP(true);
		board.Log('.');	delay(500);
		WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
		board.Log('.');	delay(100);
		WiFiAPStatus = wasConnect;
	}
	else
	{
		WiFiAPStatus = wasDisconnect;
		WiFi.enableAP(false);
		board.Log('.');	delay(500);
	}
	

	WiFiSTAStatus = wssDisconnect;
	if (CFG_WIFI_UseStation)
	{
		board.Log('.');
		delay(100);
		WiFi.enableSTA(true);
		board.Log('.');
		delay(100);
		WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));
		board.Log('.');
		delay(100);
		WiFi.begin("", "", 0, NULL, false);
	}
	else
	{
		WiFi.enableSTA(false);
	}
    board.Log("OK");

	if ((CFG_WIFI_UseAp) || (CFG_WIFI_UseStation))
	{
#ifdef XB_OTA
		if (CFG_WIFI_USEOTA)
		{
			WIFI_OTA_Init();
		}
#endif
	}

}

#ifdef XB_OTA
void WIFI_OTA_Init(void)
{
	if (CFG_WIFI_USEOTA)
	{
#ifdef NO_GLOBAL_ARDUINOOTA
		if (ArduinoOTA == NULL)
		{
			ArduinoOTA = new ArduinoOTAClass();
		}
#endif

		board.Log("OTA Init", true, true);
		board.Log('.');
#ifdef NO_GLOBAL_ARDUINOOTA
		ArduinoOTA->setPort(CFG_WIFI_PORTOTA);
#else
		ArduinoOTA.setPort(CFG_WIFI_PORTOTA);
#endif
		board.Log('.');
#ifdef NO_GLOBAL_ARDUINOOTA
		ArduinoOTA->setHostname(board.DeviceName.c_str());
#else
		ArduinoOTA.setHostname(board.DeviceName.c_str());
#endif
		board.Log('.');
		if (CFG_WIFI_PSWOTA != "")
		{
#ifdef NO_GLOBAL_ARDUINOOTA
			ArduinoOTA->setPassword(CFG_WIFI_PSWOTA.c_str());
#else
			ArduinoOTA.setPassword(CFG_WIFI_PSWOTA.c_str());
#endif
		}
		board.Log('.');


//#if !defined(_VMICRO_INTELLISENSE)
#ifdef NO_GLOBAL_ARDUINOOTA
		ArduinoOTA->onStart([](void) {
			board.SendMessage_OTAUpdateStarted();
			String type;
			if (ArduinoOTA->getCommand() == U_FLASH)
				type = "sketch";
			else
				type = "filesystem";

			board.Log(String("\n\n[WIFI] Start updating " + type).c_str());
			});

		board.Log('.');
		ArduinoOTA->onEnd([](void) {
			board.Log("\n\rEnd");
			});

		board.Log('.');
		ArduinoOTA->onProgress([](unsigned int progress, unsigned int total) {
			board.Blink_RX();
			board.Blink_TX();
			board.Blink_Life();
			board.Log('.');
			});

		board.Log('.');
		ArduinoOTA->onError([](ota_error_t error) {
			Serial.printf(("Error[%u]: "), error);
			if (error == OTA_AUTH_ERROR) Serial.println(("Auth Failed"));
			else if (error == OTA_BEGIN_ERROR) Serial.println(("Begin Failed"));
			else if (error == OTA_CONNECT_ERROR) Serial.println(("Connect Failed"));
			else if (error == OTA_RECEIVE_ERROR) Serial.println(("Receive Failed"));
			else if (error == OTA_END_ERROR) Serial.println(("End Failed"));
			});
#else
		ArduinoOTA.onStart([](void) {
			board.SendMessage_OTAUpdateStarted();
			String type;
			if (ArduinoOTA.getCommand() == U_FLASH)
				type = "sketch";
			else
				type = "filesystem";

			board.Log(String("\n\n[WIFI] Start updating " + type).c_str());
			});

		board.Log('.');
		ArduinoOTA.onEnd([](void) {
			board.Log("\n\rEnd");
			});

		board.Log('.');
		ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			board.Blink_RX();
			board.Blink_TX();
			board.Blink_Life();
			board.Log('.');
			});

		board.Log('.');
		ArduinoOTA.onError([](ota_error_t error) {
			Serial.printf(("Error[%u]: "), error);
			if (error == OTA_AUTH_ERROR) Serial.println(("Auth Failed"));
			else if (error == OTA_BEGIN_ERROR) Serial.println(("Begin Failed"));
			else if (error == OTA_CONNECT_ERROR) Serial.println(("Connect Failed"));
			else if (error == OTA_RECEIVE_ERROR) Serial.println(("Receive Failed"));
			else if (error == OTA_END_ERROR) Serial.println(("End Failed"));
	});
#endif
		board.Log('.');
#ifdef NO_GLOBAL_ARDUINOOTA
		ArduinoOTA->begin();
#else
		ArduinoOTA.begin();
#endif
		board.Log('.');
		board.Log(("OK"));
	}
	else
	{
#ifdef NO_GLOBAL_ARDUINOOTA
	if (ArduinoOTA!=NULL) ArduinoOTA->end();
#else
		ArduinoOTA.end();
#endif
	}
}
#endif

#pragma endregion


void WIFI_Setup(void)
{
	board.Log("Init.", true, true);
	board.LoadConfiguration(&XB_WIFI_DefTask);

	WiFiSTAStatus = wssDisconnect;
	WiFiAPStatus = wasDisconnect;
	NETStatus = nsDisconnect;

	#ifdef ESP8266 
	wifi_set_sleep_type(NONE_SLEEP_T);
	#endif

	WiFi.macAddress(WIFI_mac);

	board.Log('.');
	WiFiFunction = wfResetRadio;
	board.Log(".OK");
}

void WIFI_GUI_Repaint()
{
#ifdef XB_GUI
	if (WIFI_winHandle0 != NULL) WIFI_winHandle0->RepaintCounter++;
#endif
	
}

void kropka()
{
	DEF_WAITMS_VAR(krop);
	BEGIN_WAITMS(krop, 500);
	board.Log('.');
	END_WAITMS(krop);
}

uint32_t WIFI_DoLoop(void)
{
	static uint32_t LastTickStartFindWiFiAP;

	switch (WiFiFunction)
	{
	case wfIDLE:
	{
		if ((CFG_WIFI_UseAp == true) && (CFG_WIFI_UseStation == false))
		{

			if (WiFiAPStatus == wasConnect)
			{
				WiFiFunction = wfHandleOnlyAP;
			}
			else
			{
				WiFiFunction = wfResetRadio;
			}
			WIFI_GUI_Repaint();
			LastTickStartFindWiFiAP = 0;
			return 0;
		}
		if (CFG_WIFI_UseStation == true)
		{
			WIFI_GUI_Repaint();
			WiFiFunction = wfStartFindWiFiAP;
			return 0;
		}
		return 0;
	}
	case wfResetRadio:
	{
		WIFI_GUI_Repaint();
		WIFI_RESET();
		LastTickStartFindWiFiAP = 0;
		WiFiFunction = wfIDLE;
		return 1000;
	}
	case wfHandleOnlyAP:
	{
		if ((CFG_WIFI_UseAp == false) || (CFG_WIFI_UseStation == true))
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
		}
#ifdef XB_OTA
		if (CFG_WIFI_USEOTA)
		{
#ifdef NO_GLOBAL_ARDUINOOTA
			if(ArduinoOTA!=NULL) ArduinoOTA->handle();
#else
			ArduinoOTA.handle();
#endif
		}
#endif
		return 0;
	}
	case wfStartFindWiFiAP:
	{
		if (CFG_WIFI_UseStation == false)
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			return 0;
		}

		if (LastTickStartFindWiFiAP != 0)
		{
			if (SysTickCount - LastTickStartFindWiFiAP < CFG_WIFI_IntervalTickStartFindWifiAp) return 0;
		}
		LastTickStartFindWiFiAP = SysTickCount;


		int16_t n = WiFi.scanNetworks(true);
		if (n == 0)
		{
			board.Log(("No network found."), true, true);
		}
		else
		{
			board.Log(("Scan networks."), true, true);
			WiFiFunction = wfFindingWiFiAP;
		}

		WIFI_GUI_Repaint();
		return 0;
	}
	case wfFindingWiFiAP:
	{
		if (CFG_WIFI_UseStation == false)
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			return 0;
		}

		kropka();

		int16_t n = WiFi.scanComplete();
		if (n != WIFI_SCAN_RUNNING)
		{
			if (n == WIFI_SCAN_FAILED)
			{
				board.Log(("ERROR"));
				board.Log(("Scan network failed."), true, true,tlError);
				WiFiFunction = wfStartFindWiFiAP;
			}
			else if (n == 0)
			{
				board.Log(("OK"));
				board.Log(("No network found."), true, true,tlWarn);
				WiFiFunction = wfStartFindWiFiAP;
			}
			else
			{
				board.Log(("OK"));
				WiFiFunction = wfEndFindWiFiAP;
			}
			LastTickStartFindWiFiAP = SysTickCount;
			WIFI_GUI_Repaint();
		}

		return 0;
	}
	case wfEndFindWiFiAP:
	{
		if (CFG_WIFI_UseStation == false)
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			return 0;
		}

		WiFiFunction = wfStartFindWiFiAP;
		LastTickStartFindWiFiAP = SysTickCount;

		int16_t n = WiFi.scanComplete();
		board.Log(("List WIFI:"), true, true);
		for (int i = 0; i < n; ++i)
		{
			board.Log(String(i + 1).c_str(), true, true);
			board.Log((": "));
			board.Log(WiFi.SSID(i).c_str());
			board.Log((" ("));
			board.Log(String(WiFi.RSSI(i)).c_str());
			board.Log(')');
#ifdef ESP32
			board.Log((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? ' ' : '*');
#else
			board.Log((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? ' ' : '*');
#endif
			if (WiFi.SSID(i) == String(CFG_WIFI_SSID))
			{
				board.Log((" <<< Connect"));
				WiFiFunction = wfConnectWiFiAP;
				WIFI_GUI_Repaint();
			}

		}
		WiFi.scanDelete();

		if (WiFiFunction == wfConnectWiFiAP)
		{
			board.Log(("Connecting to "), true, true);
			board.Log(CFG_WIFI_SSID.c_str());
			kropka();
			return 1000;
		}

		return 0;
	}
	case wfConnectWiFiAP:
	{
		if (CFG_WIFI_UseStation == false)
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			return 0;
		}

		kropka();
#ifdef ESP32
		WiFi.setHostname(board.DeviceName.c_str());
#else
		WiFi.hostname(board.DeviceName.c_str());
#endif
		kropka();
		{IPAddress ip; ip.fromString(CFG_WIFI_StaticIP); CFG_WIFI_StaticIP_IP = ip; }
		{IPAddress ip; ip.fromString(CFG_WIFI_MASK); CFG_WIFI_MASK_IP = ip; }
		{IPAddress ip; ip.fromString(CFG_WIFI_GATEWAY); CFG_WIFI_GATEWAY_IP = ip; }
		{IPAddress ip; ip.fromString(CFG_WIFI_DNS1); CFG_WIFI_DNS1_IP = ip; }
		{IPAddress ip; ip.fromString(CFG_WIFI_DNS2); CFG_WIFI_DNS2_IP = ip; }

		WiFi.config(CFG_WIFI_StaticIP_IP, CFG_WIFI_GATEWAY_IP, CFG_WIFI_MASK_IP, CFG_WIFI_DNS1_IP, CFG_WIFI_DNS2_IP);

		kropka();
		WiFi.setSleep(false);
		esp_wifi_set_ps(WIFI_PS_NONE);
		kropka();
		WiFi.begin(CFG_WIFI_SSID.c_str(), CFG_WIFI_PSW.c_str());

		WiFiFunction = wfWaitConnectWiFiAP;
		LastTickStartFindWiFiAP = SysTickCount;
		WIFI_GUI_Repaint();

		return 0;
	}
	case wfWaitConnectWiFiAP:
	{
		if (CFG_WIFI_UseStation == false)
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			return 0;
		}

		kropka();

		if ((WiFi.status() == WL_CONNECTED))
		{
			board.Log(("OK"));
			WiFiFunction = wfHandleSta;
			NET_Connect();
			WiFiSTAStatus = wssConnect;
			WIFI_GUI_Repaint();
		}
		else
		{
			if (SysTickCount - LastTickStartFindWiFiAP  > 10000)
			{
				switch (WiFi.status())
				{
				case WL_CONNECTION_LOST: board.Log("(WL_CONNECTION_LOST)", true, true, tlError); break;
				case WL_CONNECT_FAILED: board.Log("(WL_CONNECT_FAILED)", true, true, tlError); break;
				case WL_DISCONNECTED: board.Log("(WL_DISCONNECTED)", true, true, tlError); break;
				case WL_IDLE_STATUS: board.Log("(WL_IDLE_STATUS)", true, true, tlError); break;
				case WL_NO_SHIELD: board.Log("(WL_NO_SHIELD)", true, true, tlError); break;
				case WL_NO_SSID_AVAIL: board.Log("(WL_NO_SSID_AVAIL)", true, true, tlError); break;
				case WL_SCAN_COMPLETED: board.Log("(WL_SCAN_COMPLETED)", true, true, tlError); break;
				default: board.Log(String("(ERROR STATUS "+String((int)WiFi.status())+")").c_str(), true, true, tlError); break;
				}
				WiFiFunction = wfResetRadio;
				LastTickStartFindWiFiAP = SysTickCount;
				WIFI_GUI_Repaint();
			}
		}

		return 0;
	}
	case wfHandleSta:
	{
		if (CFG_WIFI_UseStation == false)
		{
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			return 0;
		}

		if ((WiFi.status() != WL_CONNECTED))
		{
			switch (WiFi.status())
			{
			case WL_CONNECTION_LOST: board.Log("Error (WL_CONNECTION_LOST)", true, true, tlError); break;
			case WL_CONNECT_FAILED: board.Log("Error (WL_CONNECT_FAILED)", true, true, tlError); break;
			case WL_DISCONNECTED: board.Log("Error (WL_DISCONNECTED)", true, true, tlError); break;
			case WL_IDLE_STATUS: board.Log("Error (WL_IDLE_STATUS)", true, true, tlError); break;
			case WL_NO_SHIELD: board.Log("Error (WL_NO_SHIELD)", true, true, tlError); break;
			case WL_NO_SSID_AVAIL: board.Log("Error (WL_NO_SSID_AVAIL)", true, true, tlError); break;
			case WL_SCAN_COMPLETED: board.Log("Error (WL_SCAN_COMPLETED)", true, true, tlError); break;
			default: board.Log(String("Error (STATUS " + String((int)WiFi.status()) + ")").c_str(), true, true, tlError); break;
			}
			WiFiFunction = wfResetRadio;
			LastTickStartFindWiFiAP = SysTickCount;
			NET_Disconnect();
		}
#ifdef XB_OTA
		if (CFG_WIFI_USEOTA)
		{
#ifdef NO_GLOBAL_ARDUINOOTA
			if (ArduinoOTA != NULL) ArduinoOTA->handle();
#else
			ArduinoOTA.handle();
#endif
		}
#endif
		return 0;
	}
	default: WiFiFunction = wfIDLE; return 10000;

	}
	return 0;
}

bool WIFI_DoMessage(TMessageBoard* Am)
{
	switch (Am->IDMessage)
	{
	case IM_LOAD_CONFIGURATION:
	{
		return WIFI_LoadConfig();
	}
	case IM_SAVE_CONFIGURATION:
	{
		return WIFI_SaveConfig();
	}
	case IM_RESET_CONFIGURATION:
	{
		return WIFI_ResetConfig();
	}
	case IM_HANDLEPTR:
	{
#ifdef XB_GUI
		HANDLEPTR(WIFI_winHandle0);
		HANDLEPTR(WIFI_menuhandle1);
		HANDLEPTR(WIFI_inputdialoghandle0_ssid);
		HANDLEPTR(WIFI_inputdialoghandle1_psw);
		HANDLEPTR(WIFI_inputdialoghandle2_sta_ip);
		HANDLEPTR(WIFI_inputdialoghandle3_mask_ip);
		HANDLEPTR(WIFI_inputdialoghandle4_gateway_ip);
		HANDLEPTR(WIFI_inputdialoghandle5_dns1_ip);
		HANDLEPTR(WIFI_inputdialoghandle6_dns2_ip);
		HANDLEPTR(WIFI_inputdialoghandle7_appsw);
		HANDLEPTR(WIFI_inputdialoghandle8_pswota);
		HANDLEPTR(WIFI_inputdialoghandle9_portota);
		HANDLEPTR(WIFI_inputdialoghandle10_channelap);
#endif
		return true;
	}
	case IM_GET_TASKNAME_STRING:
	{
		GET_TASKNAME("WIFI");
		return true;
	}
	case IM_GET_TASKSTATUS_STRING:
	{
		GET_TASKSTATUS_ADDSTR(WIFI_GetString_WiFiFunction(WiFiFunction));

		if (WiFiFunction == wfHandleSta)
		{
				GET_TASKSTATUS_ADDSTR(" " + WiFi.localIP().toString());
		}

		return true;
	}
	case IM_INTERNET_CONNECT:
	{
		WIFI_GUI_Repaint();
		break;
	}
	case IM_INTERNET_DISCONNECT:
	{
		WIFI_GUI_Repaint();
		break;
	}
#ifdef XB_GUI		
	case IM_MENU:
	{
		OPEN_MAINMENU()
		{
			WIFI_menuhandle1 = GUIGADGET_CreateMenu(&XB_WIFI_DefTask, 1, false, X, Y );
		}

		BEGIN_MENU(1, "WIFI Configuration", WINDOW_POS_X_DEF, WINDOW_POS_Y_DEF, 40, MENU_AUTOCOUNT, 0, true)
		{
			BEGIN_MENUITEM_CHECKED("Debug", taLeft, CFG_WIFI_DEBUG)
			{
				CLICK_MENUITEM()
				{
					CFG_WIFI_DEBUG = !CFG_WIFI_DEBUG;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Reset WIFI", taLeft)
			{
				CLICK_MENUITEM()
				{
					WiFiFunction = wfResetRadio;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Show WIFI Status...", taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_winHandle0 = GUI_WindowCreate(&XB_WIFI_DefTask, 0, false);
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			BEGIN_MENUITEM_CHECKED("USE WiFi STATION", taLeft, CFG_WIFI_UseStation)
			{
				CLICK_MENUITEM()
				{
					CFG_WIFI_UseStation = !CFG_WIFI_UseStation;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set SSID [" + CFG_WIFI_SSID + "]"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle0_ssid = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 0);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set PASSWORD "+(CFG_WIFI_PSW == "" ? String("") : String("[****]"))), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle1_psw = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 1);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set STA IP      (" + CFG_WIFI_StaticIP + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle2_sta_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 2);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set STA Mask    (" + CFG_WIFI_MASK + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle3_mask_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 3);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set STA GateWay (" + CFG_WIFI_GATEWAY + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle4_gateway_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 4);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("USE DHCP", taLeft)
			{
				CLICK_MENUITEM_REPAINT()
				{
					CFG_WIFI_StaticIP_IP = IPAddress(0, 0, 0, 0); CFG_WIFI_StaticIP = CFG_WIFI_StaticIP_IP.toString();
					CFG_WIFI_GATEWAY_IP = IPAddress(0, 0, 0, 0); CFG_WIFI_GATEWAY = CFG_WIFI_GATEWAY_IP.toString();
					CFG_WIFI_MASK_IP = IPAddress(0, 0, 0, 0); CFG_WIFI_MASK = CFG_WIFI_MASK_IP.toString();
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set DNS1 (" + CFG_WIFI_DNS1 + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle5_dns1_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 5);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set DNS2 (" + CFG_WIFI_DNS2 + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle6_dns2_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 6);
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			BEGIN_MENUITEM_CHECKED("USE WiFi AP", taLeft, CFG_WIFI_UseAp)
			{
				CLICK_MENUITEM()
				{
					CFG_WIFI_UseAp = !CFG_WIFI_UseAp;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set AP PASSWORD " + (CFG_WIFI_AP_PSW == "" ? String("") : String("[****]"))), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle7_appsw = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 7);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set AP Channel [" + String(CFG_WIFI_ChannelAp)+"]"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle10_channelap = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 10);
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM_CHECKED("HIDE AP", taLeft, CFG_WIFI_HideAp)
			{
				CLICK_MENUITEM()
				{
					CFG_WIFI_HideAp = !CFG_WIFI_HideAp;
				}
			}
			END_MENUITEM()
#ifdef XB_OTA
			SEPARATOR_MENUITEM()
			BEGIN_MENUITEM_CHECKED("USE OTA Update", taLeft, CFG_WIFI_USEOTA)
			{
				CLICK_MENUITEM()
				{
					CFG_WIFI_USEOTA = !CFG_WIFI_USEOTA;
					WIFI_RESET();
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set OTA PASSWORD " + (CFG_WIFI_PSWOTA == "" ? String("") : String("[****]"))), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle8_pswota = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 8);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM(String("Set OTA PORT [" + String(CFG_WIFI_PORTOTA)+"]"), taLeft)
			{
				CLICK_MENUITEM()
				{
					WIFI_inputdialoghandle9_portota = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 9);
				}
			}
			END_MENUITEM()

#endif
			SEPARATOR_MENUITEM()
			CONFIGURATION_MENUITEMS()

		}
		END_MENU()

			return true;
	}
	case IM_INPUTDIALOG:
	{
		BEGIN_DIALOG(0, "Edit WIFI SSID", "Please input WIFI SSID: ", tivString, 16, &CFG_WIFI_SSID)
		{
		}
		END_DIALOG()

		BEGIN_DIALOG(1, "Edit WIFI PSW", "Please input WIFI PSW: ", tivString, 16, &CFG_WIFI_PSW)
		{
		}
		END_DIALOG()

		BEGIN_DIALOG(2, "Edit STATIC IP", "Please input static ip: ", tivIP, 15, &CFG_WIFI_StaticIP_IP)
		{
			EVENT_ENTER()
			{
				CFG_WIFI_StaticIP = CFG_WIFI_StaticIP_IP.toString();
			}
		}
		END_DIALOG()

		BEGIN_DIALOG(3, "Edit MASK IP", "Please input mask ip: ", tivIP, 15, &CFG_WIFI_MASK_IP)
		{
			EVENT_ENTER()
			{
				CFG_WIFI_MASK = CFG_WIFI_MASK_IP.toString();
			}
		}
		END_DIALOG()

		BEGIN_DIALOG(4, "Edit GATEWAY IP", "Please input gateway ip: ", tivIP, 15, &CFG_WIFI_GATEWAY_IP)
		{
			EVENT_ENTER()
			{
				CFG_WIFI_GATEWAY = CFG_WIFI_GATEWAY_IP.toString();
			}
		}
		END_DIALOG()

		BEGIN_DIALOG(5, "Edit DNS1 IP", "Please input dns1 ip: ", tivIP, 15, &CFG_WIFI_DNS1_IP)
		{
			EVENT_ENTER()
			{
				CFG_WIFI_DNS1 = CFG_WIFI_DNS1_IP.toString();
			}
		}
		END_DIALOG()

		BEGIN_DIALOG(6, "Edit DNS2 IP", "Please input dns2 ip: ", tivIP, 15, &CFG_WIFI_DNS2_IP)
		{
			EVENT_ENTER()
			{
				CFG_WIFI_DNS2 = CFG_WIFI_DNS2_IP.toString();
			}
		}
		END_DIALOG()

		BEGIN_DIALOG(7, "Edit WIFI AP PSW", "Please input WIFI AP PSW: ", tivString, 16, &CFG_WIFI_AP_PSW)
		{
		}
		END_DIALOG()

#ifdef XB_OTA
		BEGIN_DIALOG(8, "Edit OTA PASSWORD", "Please input WIFI OTA PASSWORD: ", tivString, 16, &CFG_WIFI_PSWOTA)
		{
		}
		END_DIALOG()

		BEGIN_DIALOG_MINMAX(9, "Edit OTA PORT", "Please input SERVER OTA PORT: ", tivInt16, 5, &CFG_WIFI_PORTOTA,1,32767)
		{
		}
		END_DIALOG()
#endif
		BEGIN_DIALOG_MINMAX(10, "Edit channel AP", "Please input channel AP: ", tivUInt8, 2, &CFG_WIFI_ChannelAp, 1, 12)
		{
		}
		END_DIALOG()


		return true;
	}
	case IM_WINDOW:
	{
		BEGIN_WINDOW_DEF(0, "STATUS", WINDOW_POS_LAST_RIGHT_ACTIVE, WINDOW_POS_Y_DEF, 32, 9, WIFI_winHandle0)
		{
			REPAINT_WINDOW()
			{
				WH->BeginDraw();
				WH->SetNormalChar();
				WH->SetTextColor(tfcWhite);

				int x = 0;

				WH->PutStr(x, 0, ("WIFI SSID:"));
				WH->PutStr(x, 1, ("WIFI IP:"));
				WH->PutStr(x, 2, ("STATUS:"));
				WH->PutStr(x, 3, ("RSSI:"));

				WH->PutStr(x, 5, ("WIFI STA:"));
				WH->PutStr(x, 6, ("WIFI AP:"));

				WH->EndDraw();
			}
			REPAINTDATA_WINDOW()
			{
				WH->BeginDraw();
				WH->SetNormalChar();
				WH->SetBoldChar();
				WH->SetTextColor(tfcYellow);

				int x = 0;

				WH->PutStr(x + 10, 0, WiFi.SSID().c_str());
				WH->PutStr(x + 8, 1, WiFi.localIP().toString().c_str());

				switch (WiFi.status())
				{
				case WL_NO_SHIELD:		WH->PutStr(x + 7, 2, ("NO SHIELD      ")); break;
				case WL_IDLE_STATUS:	WH->PutStr(x + 7, 2, ("IDLE STATUS    ")); break;
				case WL_NO_SSID_AVAIL:	WH->PutStr(x + 7, 2, ("NO SSID AVAIL  ")); break;
				case WL_SCAN_COMPLETED:	WH->PutStr(x + 7, 2, ("SCAN COMPLETED ")); break;
				case WL_CONNECTED:		WH->PutStr(x + 7, 2, ("CONNECTED      ")); break;
				case WL_CONNECT_FAILED:	WH->PutStr(x + 7, 2, ("CONNECT FAILED ")); break;
				case WL_CONNECTION_LOST:WH->PutStr(x + 7, 2, ("CONNECTION LOST")); break;
				case WL_DISCONNECTED:	WH->PutStr(x + 7, 2, ("DISCONNECTED   ")); break;
				default:				WH->PutStr(x + 7, 2, String(WiFi.status()).c_str()); break;
				}
				WH->PutStr(x + 5, 3, String(WiFi.RSSI()).c_str());
				WH->PutChar(' ');
				WH->PutChar(' ');

				switch (WiFiSTAStatus)
				{
				case wssDisconnect: WH->PutStr(x + 9, 5, ("Disconnect")); break;
				case wssConnect:WH->PutStr(x + 9, 5, ("Connect   ")); break;
				}

				switch (WiFiAPStatus)
				{
				case wasDisconnect: WH->PutStr(x + 8, 6, ("Disconnect")); break;
				case wasConnect:WH->PutStr(x + 8, 6, ("Connect   ")); break;
				}

				WH->EndDraw();
			}
		}
		END_WINDOW_DEF()
#endif
			return true;
	}
	default: break;
	}
	return false;
}

TTaskDef XB_WIFI_DefTask = { 1, &WIFI_Setup,&WIFI_DoLoop,&WIFI_DoMessage };

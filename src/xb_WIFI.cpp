#include <xb_board.h>
#include <xb_WIFI.h>
#include <xb_PING.h>

#ifdef ESP32
#include <ESPmDNS.h>
#include <WiFi.h>

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

#ifdef WIFI_ARDUINO_OTA
#include <ArduinoOTA.h>
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

typedef enum { wfDoFindtWiFi, wfDoWaitFindtWiFi, wfHandle, wfWaitConnectWiFi, wfDoConnectWiFi, wfCheckInternetAvaliable , wfWaitForPingGateway } TWiFiFunction;
TWiFiFunction WiFiFunction = wfHandle;

TWiFiStatus WiFiStatus;
TInternetStatus WIFI_InternetStatus;
TWiFiAPStatus WiFiAPStatus;

TTaskDef XB_WIFI_DefTask = {1, &WIFI_Setup,&WIFI_DoLoop,&WIFI_DoMessage};

IPAddress WIFI_dnsip1(8, 8, 8, 8);
IPAddress WIFI_dnsip2(8, 8, 4, 4);
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
#endif

// Konfiguracja -----------------------------------------------------------------------------------------------------
#pragma region KONFIGURACJA


bool CFG_WIFI_AutoConnect = false;
bool CFG_WIFI_DEBUG = true;

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


String CFG_WIFI_AdministratorSSID;
String CFG_WIFI_AdministratorPSW;
uint32_t CFG_WIFI_AdministratorStaticIP = 0;
uint32_t CFG_WIFI_AdministratorMASK = 0;
uint32_t CFG_WIFI_AdministratorGATEWAY = 0;

void WIFI_PutDot_Debug()
{
	if (CFG_WIFI_DEBUG) board.Log('.');
}

bool WIFI_LoadConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("WIFI"))
	{
		CFG_WIFI_DEBUG = board.PREFERENCES_GetBool("DEBUG", CFG_WIFI_DEBUG);
		
		CFG_WIFI_AutoConnect = board.PREFERENCES_GetBool("AUTOCONNECT", CFG_WIFI_AutoConnect); 
		CFG_WIFI_SSID = board.PREFERENCES_GetString("SSID", CFG_WIFI_SSID); 
		CFG_WIFI_PSW = board.PREFERENCES_GetString("PSW", CFG_WIFI_PSW); 
		CFG_WIFI_StaticIP = board.PREFERENCES_GetString("StaticIP", CFG_WIFI_StaticIP); 
		{IPAddress ip; ip.fromString(CFG_WIFI_StaticIP); CFG_WIFI_StaticIP_IP = ip; }

		CFG_WIFI_MASK = board.PREFERENCES_GetString("Mask", CFG_WIFI_MASK); 
		CFG_WIFI_GATEWAY = board.PREFERENCES_GetString("Gateway", CFG_WIFI_GATEWAY); 

		CFG_WIFI_AdministratorSSID = board.PREFERENCES_GetString("ASSID", CFG_WIFI_AdministratorSSID); 
		CFG_WIFI_AdministratorPSW = board.PREFERENCES_GetString("APSW", CFG_WIFI_AdministratorPSW); 
		CFG_WIFI_AdministratorStaticIP = board.PREFERENCES_GetUINT32("AStaticIP", CFG_WIFI_AdministratorStaticIP); 
		CFG_WIFI_AdministratorMASK = board.PREFERENCES_GetUINT32("AMask", CFG_WIFI_AdministratorMASK); 
		CFG_WIFI_AdministratorGATEWAY = board.PREFERENCES_GetUINT32("AGateway", CFG_WIFI_AdministratorGATEWAY); 
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
		board.PREFERENCES_PutBool("AUTOCONNECT", CFG_WIFI_AutoConnect); 
		board.PREFERENCES_PutBool("DEBUG", CFG_WIFI_DEBUG); 
		board.PREFERENCES_PutString("SSID", CFG_WIFI_SSID); 
		board.PREFERENCES_PutString("PSW", CFG_WIFI_PSW); 
		board.PREFERENCES_PutString("StaticIP", CFG_WIFI_StaticIP); 
		board.PREFERENCES_PutString("Mask", CFG_WIFI_MASK); 
		board.PREFERENCES_PutString("Gateway", CFG_WIFI_GATEWAY); 

		board.PREFERENCES_PutString("ASSID", CFG_WIFI_AdministratorSSID); 
		board.PREFERENCES_PutString("APSW", CFG_WIFI_AdministratorPSW); 
		board.PREFERENCES_PutUINT32("AStaticIP", CFG_WIFI_AdministratorStaticIP); 
		board.PREFERENCES_PutUINT32("AMask", CFG_WIFI_AdministratorMASK); 
		board.PREFERENCES_PutUINT32("AGateway", CFG_WIFI_AdministratorGATEWAY); 
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



void WIFI_DoAllTaskWifiDisconnect(void)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_WIFI_DISCONNECT;

	board.DoMessageOnAllTask(&mb, true, doBACKWARD); 
}

void WIFI_DoAllTaskInternetDisconnect(void)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_INTERNET_DISCONNECT;

	board.DoMessageOnAllTask(&mb,true,doBACKWARD); 
}

void WIFI_DoAllTaskInternetConnect(void)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_INTERNET_CONNECT;

	board.DoMessageOnAllTask(&mb, true, doFORWARD); 
}

void WIFI_DoAllTaskWiFiConnect(void)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_WIFI_CONNECT;

	board.DoMessageOnAllTask(&mb, true, doFORWARD); 
}

void WIFI_SetConnectWiFi(void)
{
	WiFiStatus = wsConnect;
	WIFI_DoAllTaskWiFiConnect();
}

void WIFI_SetConnectInternet(void)
{
	WIFI_InternetStatus = isConnect;
	WIFI_DoAllTaskInternetConnect();
}

void WIFI_SetDisconnectWiFi(void)
{
	WiFiStatus = wsDisconnect;
	WIFI_DoAllTaskWifiDisconnect();
}

void WIFI_SetDisconnectInternet(void)
{
	WIFI_InternetStatus = isDisconnect;
	WIFI_DoAllTaskInternetDisconnect();
}

void WIFI_HardDisconnect(void)
{
	int i = 0;
		
	WIFI_SetDisconnectInternet();
	WIFI_SetDisconnectWiFi();
	WiFi.scanDelete();
	delay(100);
		
	if (WiFi.status() == WL_CONNECTED)
	{
		board.Log("Disconnecting WIFI.", true, true);
		board.Log('.');
		WiFi.disconnect(true);
		delay(100);
	}
	else
	{
		board.Log("Reseting WIFI.", true, true);
	}
	board.Log('.');
	delay(100);
	board.Log('.');
	WiFi.persistent(false);
	delay(100);
	WiFi.setAutoConnect(false);
	board.Log('.');
	delay(100);
	WiFi.setAutoReconnect(false);
	board.Log('.');
	delay(100);
	WiFiFunction = wfHandle;

	WiFi.softAPsetHostname(board.DeviceName.c_str());
	WiFi.softAP(board.DeviceName.c_str(), "0987654321");

	WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));
	WiFi.enableAP(true);
	WiFi.mode(WIFI_AP_STA);
	WiFi.begin("", "", 0, {0},true);
	delay(1000);

    board.Log("OK");
}

bool WIFI_CheckDisconnectWiFi(void)
{
	if ((WiFi.status() != WL_CONNECTED))
	{
		if (WiFiStatus == wsConnect)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

#ifdef WIFI_ARDUINO_OTA
void WIFI_OTA_Init(void)
{
	board.Log(FSS("ArduinoOTA Init"), true, true);
	board.Log('.');
	ArduinoOTA.setPort(8266);
	board.Log('.');
	ArduinoOTA.setHostname(board.DeviceName.c_str());
	board.Log('.');
	//ArduinoOTA.setPassword("0987654321");

	board.Log('.');

	
	#if !defined(_VMICRO_INTELLISENSE)
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
		board.Blink_RX(1);
		board.Blink_Life();
		board.Log('.');
	});

	board.Log('.');
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf(FSS("Error[%u]: "), error);
		if (error == OTA_AUTH_ERROR) Serial.println(FSS("Auth Failed"));
		else if (error == OTA_BEGIN_ERROR) Serial.println(FSS("Begin Failed"));
		else if (error == OTA_CONNECT_ERROR) Serial.println(FSS("Connect Failed"));
		else if (error == OTA_RECEIVE_ERROR) Serial.println(FSS("Receive Failed"));
		else if (error == OTA_END_ERROR) Serial.println(FSS("End Failed"));
	});
	#endif
	board.Log('.');
	ArduinoOTA.begin();
	board.Log('.');
	board.Log(FSS("OK"));
}
#endif

#pragma endregion


void WIFI_Setup(void)
{
	board.LoadConfiguration(&XB_WIFI_DefTask);

	board.Log("Init.", true, true);

	WiFiStatus = wsDisconnect;
	WIFI_InternetStatus = isDisconnect;

	#ifdef ESP8266 
	wifi_set_sleep_type(NONE_SLEEP_T);
	#endif

	WiFi.macAddress(WIFI_mac);

	{
		WiFiAPStatus = wasConnect;

		bool lastshowloginfo = XB_WIFI_DefTask.Task->ShowLogInfo;
		XB_WIFI_DefTask.Task->ShowLogInfo = false;
		WIFI_HardDisconnect();
		XB_WIFI_DefTask.Task->ShowLogInfo = lastshowloginfo;
	}

	board.Log('.');
	WiFiFunction = wfHandle;
	board.Log(".OK");

	board.AddTask(&XB_PING_DefTask);
}
void WIFI_GUI_Repaint()
{
#ifdef XB_GUI
	if (WIFI_winHandle0 != NULL) WIFI_winHandle0->RepaintCounter++;
	#endif
	
}

uint32_t WIFI_DoLoop(void)
{
	switch (WiFiFunction)
	{
	case wfHandle: // Normalna praca 
		{
			if (CFG_WIFI_AutoConnect == false) return 1000;

			// Sprawdzenie czy jest ustanowione po³¹cznie WIFI
			if((WiFi.status() == WL_CONNECTED) && (WiFi.localIP() != 0))
			{

				// Sprawdzenie czy jeœli ustanowione jest po³¹czenie to czy status jest zmieniony na istniej¹ce po¹³czenie
				if(WiFiStatus == wsConnect)
				{

					if (PING_GATEWAY_IS == false)
					{
						board.Log(FSS("Router ping error."), true, true);
						WIFI_HardDisconnect();
						WIFI_GUI_Repaint();
					}
					else
					// Jeœli jeszcze nie sprawdzone czy jest internet dostepny
					if((WIFI_InternetStatus == isConnect) && (WiFiStatus == wsConnect))
					{
						if (!PING_8888_IS)
						{
							board.Log(FSS("Internet ping error."), true, true);
							WiFiFunction = wfCheckInternetAvaliable;
							WIFI_SetDisconnectInternet();
							WIFI_GUI_Repaint();
						}
					}
					else
					{
						if (!PING_8888_IS)
						{
							WiFiFunction = wfCheckInternetAvaliable;
							WIFI_SetDisconnectInternet();
						}
						else
						{
							WIFI_SetConnectInternet();
						}
						WIFI_GUI_Repaint();

					}
					#ifdef ESP32
					if (WiFiStatus == wsConnect)
					{
					#ifdef WIFI_ARDUINO_OTA
						ArduinoOTA.handle();
						#endif
					}
					#else
					if (ArduinoOTA != NULL) // System aktualizacji OTA
						{
							ArduinoOTA->handle();
						}
						#endif

				}
				else
				{
					WIFI_SetDisconnectInternet();
					WIFI_SetConnectWiFi();
					WIFI_GUI_Repaint();
				}
			}
			else
			{
				DEF_WAITMS_VAR(hhh1);
				if (WiFiStatus == wsConnect)
				{
					BEGIN_WAITMS(hhh1, 1000)
					{
						WIFI_HardDisconnect();
							
						WiFiFunction = wfDoFindtWiFi;
						WIFI_GUI_Repaint();
						RESET_WAITMS(hhh1);
					}
					END_WAITMS(hhh1)
				}
				else
				{
					WiFiFunction = wfDoFindtWiFi;

					WIFI_GUI_Repaint();
					RESET_WAITMS(hhh1);
				}

			}
			break;
		}
	case wfWaitForPingGateway:
		{
			WiFiFunction = wfHandle;
			break;
		}
	case wfDoFindtWiFi:
		{
			DEF_WAITMS_VAR(dfw);
			BEGIN_WAITMS(dfw, 10000);
			{
				int16_t n = WiFi.scanNetworks(true);

				if (n == 0)
				{
					board.Log(FSS("No network found."), true, true);
				}
				else
				{
					WiFiFunction = wfDoWaitFindtWiFi;
					WIFI_GUI_Repaint();
					board.Log(FSS("Scan networks."), true, true);

				}

				RESET_WAITMS(dfw);
			}
			END_WAITMS(dfw);

			break;
		}
	case wfDoWaitFindtWiFi:
		{
			DEF_WAITMS_VAR(krop);

			BEGIN_WAITMS(krop, 500);
			board.Log('.');
			END_WAITMS(krop);

			int16_t n = WIFI_SCAN_RUNNING;
			DEF_WAITMS_VAR(scancompletewait);
			BEGIN_WAITMS(scancompletewait, 100);
			{
				RESET_WAITMS(scancompletewait);
				n = WiFi.scanComplete();
			}
			END_WAITMS(scancompletewait);

			if (n != WIFI_SCAN_RUNNING)
			{
				if (n == WIFI_SCAN_FAILED)
				{
					board.Log(FSS("Scan network failed."), true, true);
					WiFiFunction = wfDoFindtWiFi;
					WIFI_GUI_Repaint();
				}
				else if (n == 0)
				{
					board.Log(FSS("OK"));
					board.Log(FSS("No network found."), true, true);
					WiFiFunction = wfDoFindtWiFi;
					WIFI_GUI_Repaint();
				}
				else
				{
					WiFiFunction = wfDoFindtWiFi;
					board.Log(FSS("OK"));
					board.Log(FSS("List WIFI:"), true, true);
					for (int i = 0; i < n; ++i)
					{
						board.Log(String(i + 1).c_str(), true, true);
						board.Log(FSS(": "));
						board.Log(WiFi.SSID(i).c_str());
						board.Log(FSS(" ("));
						board.Log(String(WiFi.RSSI(i)).c_str());
						board.Log(')');
						#ifdef ESP32
						board.Log((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? ' ' : '*');
						#else
						board.Log((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? ' ' : '*');
						#endif
						if (WiFi.SSID(i) == String(CFG_WIFI_SSID))
						{
							board.Log(FSS(" <<< Connect"));
							WiFiFunction = wfDoConnectWiFi;
							WIFI_GUI_Repaint();
						}
					
					}
				
					WiFi.scanDelete();

					if (WiFiFunction == wfDoConnectWiFi)
					{
						board.Log(FSS("Connecting to "), true, true);
						board.Log(CFG_WIFI_SSID.c_str());
						board.Log('.');
					}
				}
			}
			break;
		}
	case wfDoConnectWiFi:
		{
			board.Log('.');
			#ifdef ESP32
			WiFi.setHostname(board.DeviceName.c_str());
			#else
			WiFi.hostname(board.DeviceName.c_str());
			#endif
			board.Log('.');
			//PING_8888_addr = WIFI_dnsip2;
			{IPAddress ip; ip.fromString(CFG_WIFI_StaticIP); CFG_WIFI_StaticIP_IP = ip; }
			{IPAddress ip; ip.fromString(CFG_WIFI_MASK); CFG_WIFI_MASK_IP = ip; }
			{IPAddress ip; ip.fromString(CFG_WIFI_GATEWAY); CFG_WIFI_GATEWAY_IP = ip; }
			//PING_GATEWAY_addr = CFG_WIFI_GATEWAY_IP;
			WiFi.config(CFG_WIFI_StaticIP_IP, CFG_WIFI_GATEWAY_IP, CFG_WIFI_MASK_IP, WIFI_dnsip1, WIFI_dnsip2);

			board.Log('.');
			WiFi.mode(WIFI_AP_STA);
			WiFi.setSleep(false);
			esp_wifi_set_ps(WIFI_PS_NONE);
			board.Log('.');
			WiFi.begin(CFG_WIFI_SSID.c_str(), CFG_WIFI_PSW.c_str());
		
			WiFiFunction = wfWaitConnectWiFi;
			WIFI_GUI_Repaint();
			break;
		}
	case wfWaitConnectWiFi:
		{
			DEF_WAITMS_VAR(hhh4);
			if ((WiFi.status() == WL_CONNECTED))
			{
				RESET_WAITMS(hhh4);
				board.Log(FSS("OK"));
				WIFI_SetConnectWiFi();
				WiFiFunction = wfHandle;
				WIFI_GUI_Repaint();
			}
			else
			{
				DEF_WAITMS_VAR(krop2);

				BEGIN_WAITMS(krop2, 1000);
				board.Log('.');
				END_WAITMS(krop2);

				BEGIN_WAITMS(hhh4, 30000)
				{
					RESET_WAITMS(hhh4);
					board.Log(FSS("TIME OUT"));
					board.Log(FSS("Refind network."), true, true);
					WIFI_HardDisconnect();
					//				WIFI_SetDisconnectInternet();
					//				WIFI_SetDisconnectWiFi();
									WiFiFunction = wfDoFindtWiFi;
					WIFI_GUI_Repaint();
				}
				END_WAITMS(hhh4)
			}
			break;
		}
	case wfCheckInternetAvaliable:
		{
		#ifdef WIFI_ARDUINO_OTA
			ArduinoOTA.handle();
			#endif

			DEF_WAITMS_VAR(hhh3);
			BEGIN_WAITMS(hhh3, 2000)
			{
			#ifdef ESP32
				if (PING_8888_IS)
				#else
					if (ESP8266Ping.ping(IPAddress(8, 8, 8, 8), 1))
					#endif
					{
						WIFI_SetConnectInternet();
						RESET_WAITMS(hhh3);
					}
					else
					{
						WIFI_SetDisconnectInternet();
					}
				WiFiFunction = wfHandle;
				WIFI_GUI_Repaint();
			}
			END_WAITMS(hhh3)
			break;
		}
	default: WiFiFunction = wfHandle; break;
	}
	return 0;
}

bool WIFI_DoMessage(TMessageBoard *Am)
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
	case IM_FREEPTR:
	{
#ifdef XB_GUI
		
		if (Am->Data.FreePTR == WIFI_winHandle0) WIFI_winHandle0 = NULL;
		if (Am->Data.FreePTR == WIFI_menuhandle1) WIFI_menuhandle1 = NULL;
		if (Am->Data.FreePTR == WIFI_inputdialoghandle0_ssid) WIFI_inputdialoghandle0_ssid = NULL;
		if (Am->Data.FreePTR == WIFI_inputdialoghandle1_psw) WIFI_inputdialoghandle1_psw = NULL;
		if (Am->Data.FreePTR == WIFI_inputdialoghandle2_sta_ip) WIFI_inputdialoghandle2_sta_ip = NULL;
		if (Am->Data.FreePTR == WIFI_inputdialoghandle3_mask_ip) WIFI_inputdialoghandle3_mask_ip = NULL;
		if (Am->Data.FreePTR == WIFI_inputdialoghandle4_gateway_ip) WIFI_inputdialoghandle4_gateway_ip = NULL;
#endif
		return true;
	}
	case IM_GET_TASKNAME_STRING:
		{
			*(Am->Data.PointerString) = FSS("WIFI");
			return true;
		}
	case IM_GET_TASKSTATUS_STRING:
		{
			switch (WiFiFunction)
			{

			case wfCheckInternetAvaliable: *(Am->Data.PointerString) = FSS("wfCheckInternetAvaliable"); break;
			case wfDoConnectWiFi:          *(Am->Data.PointerString) = FSS("wfDoConnectWiFi         "); break;
			case wfDoFindtWiFi:            *(Am->Data.PointerString) = FSS("wfDoFindtWiFi           "); break;
			case wfDoWaitFindtWiFi:        *(Am->Data.PointerString) = FSS("wfDoWaitFindtWiFi       "); break;
			case wfHandle:                 *(Am->Data.PointerString) = FSS("wfHandle                "); break;
			case wfWaitConnectWiFi:        *(Am->Data.PointerString) = FSS("wfWaitConnectWiFi       "); break;
			case wfWaitForPingGateway:     *(Am->Data.PointerString) = FSS("wfWaitForPingGateway    "); break;
			default:                       *(Am->Data.PointerString) = FSS("..."); break;
			}
			return true;
		}
	case IM_WIFI_CONNECT:
		{
			WIFI_GUI_Repaint();
			PING_GATEWAY_IS = true;
			#ifdef WIFI_ARDUINO_OTA
			WIFI_OTA_Init();
			#endif
			WIFI_GUI_Repaint();
			break;
		}
	case IM_WIFI_DISCONNECT:
	case IM_INTERNET_CONNECT:
	case IM_INTERNET_DISCONNECT:
		{
			WIFI_GUI_Repaint();
			break;
		}
/*	case IM_CONFIG_SAVE:
		{
			WIFI_SaveConfig();
			return true;
		}*/
		#ifdef XB_GUI		
	case IM_MENU:
		{
			OPEN_MAINMENU()
			{
				WIFI_menuhandle1 = GUIGADGET_CreateMenu(&XB_WIFI_DefTask, 1,false,X,Y);
			}

			BEGIN_MENU(1, "WIFI Configuration",WINDOW_POS_X_DEF,WINDOW_POS_Y_DEF,40,MENU_AUTOCOUNT,0,true)
			{
				BEGIN_MENUITEM_CHECKED("Auto Connect", taLeft, CFG_WIFI_AutoConnect)
				{
					CLICK_MENUITEM()
					{
						CFG_WIFI_AutoConnect = !CFG_WIFI_AutoConnect;

						if (CFG_WIFI_AutoConnect == false)
						{
							WIFI_HardDisconnect();
						}
					}
				}
				END_MENUITEM()
				BEGIN_MENUITEM_CHECKED("Debug", taLeft, CFG_WIFI_DEBUG)
				{
					CLICK_MENUITEM()
					{
						CFG_WIFI_DEBUG = !CFG_WIFI_DEBUG;
					}
				}
				END_MENUITEM()
				BEGIN_MENUITEM("Disconnect WIFI", taLeft)
				{
					CLICK_MENUITEM()
					{
						WIFI_HardDisconnect();
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
				CONFIGURATION_MENUITEMS()
				BEGIN_MENUITEM(String("Set SSID [" + CFG_WIFI_SSID + "]"), taLeft)
				{
					CLICK_MENUITEM()
					{
						WIFI_inputdialoghandle0_ssid = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 0);
					}
				}
				END_MENUITEM()
				BEGIN_MENUITEM("Set PASSWORD", taLeft)
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
			}
			END_MENU()

			return true;
		}
	case IM_INPUTDIALOG:
	{
		BEGIN_INPUTDIALOGINIT(0)
			DEF_INPUTDIALOGINIT(tivString, 16, &CFG_WIFI_SSID)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOGINIT(1)
			DEF_INPUTDIALOGINIT(tivDynArrayChar1, 16, &CFG_WIFI_PSW)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOGINIT(2)
			{IPAddress ip; ip.fromString(CFG_WIFI_StaticIP); CFG_WIFI_StaticIP_IP = ip; }
			DEF_INPUTDIALOGINIT(tivIP, 15, &CFG_WIFI_StaticIP_IP)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOGINIT(3)
			{IPAddress ip; ip.fromString(CFG_WIFI_MASK); CFG_WIFI_MASK_IP = ip; }
			DEF_INPUTDIALOGINIT(tivIP, 15, &CFG_WIFI_MASK_IP)
		END_INPUTDIALOGINIT()
				
		BEGIN_INPUTDIALOGINIT(4)
			{IPAddress ip; ip.fromString(CFG_WIFI_GATEWAY); CFG_WIFI_GATEWAY_IP = ip; }
			DEF_INPUTDIALOGINIT(tivIP, 15, &CFG_WIFI_GATEWAY_IP)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOG_ENTER(2)
			CFG_WIFI_StaticIP = IPAddress(CFG_WIFI_StaticIP_IP).toString();
		END_INPUTDIALOG_ENTER()
		BEGIN_INPUTDIALOG_ENTER(3)
			CFG_WIFI_MASK = IPAddress(CFG_WIFI_MASK_IP).toString() ;
		END_INPUTDIALOG_ENTER()
		BEGIN_INPUTDIALOG_ENTER(4)
			CFG_WIFI_GATEWAY = IPAddress(CFG_WIFI_GATEWAY_IP).toString() ;
		END_INPUTDIALOG_ENTER()
			
		BEGIN_INPUTDIALOGCAPTION(0)
			DEF_INPUTDIALOGCAPTION("Edit WIFI SSID")
		END_INPUTDIALOGCAPTION()
		BEGIN_INPUTDIALOGCAPTION(1)
			DEF_INPUTDIALOGCAPTION("Edit WIFI PASSWORD")
		END_INPUTDIALOGCAPTION()
		BEGIN_INPUTDIALOGCAPTION(2)
			DEF_INPUTDIALOGCAPTION("Edit STA IP")
		END_INPUTDIALOGCAPTION()
		BEGIN_INPUTDIALOGCAPTION(3)
			DEF_INPUTDIALOGCAPTION("Edit MASK IP")
		END_INPUTDIALOGCAPTION()
		BEGIN_INPUTDIALOGCAPTION(4)
			DEF_INPUTDIALOGCAPTION("Edit GATEWAY IP")
		END_INPUTDIALOGCAPTION()

		BEGIN_INPUTDIALOGDESCRIPTION(0)
			DEF_INPUTDIALOGDESCRIPTION("Please input WIFI SSID: ")
		END_INPUTDIALOGDESCRIPTION()
		BEGIN_INPUTDIALOGDESCRIPTION(1)
			DEF_INPUTDIALOGDESCRIPTION("Please input WIFI PASSWORD: ")
		END_INPUTDIALOGDESCRIPTION()
		BEGIN_INPUTDIALOGDESCRIPTION(2)
			DEF_INPUTDIALOGDESCRIPTION("Please input IPv4: ")
		END_INPUTDIALOGDESCRIPTION()
		BEGIN_INPUTDIALOGDESCRIPTION(3)
			DEF_INPUTDIALOGDESCRIPTION("Please input IPv4: ")
		END_INPUTDIALOGDESCRIPTION()
		BEGIN_INPUTDIALOGDESCRIPTION(4)
			DEF_INPUTDIALOGDESCRIPTION("Please input IPv4: ")
		END_INPUTDIALOGDESCRIPTION()
			
		return true;
	}
	case IM_WINDOW:
	{
		BEGIN_WINDOW_DEF(0, "STATUS", WINDOW_POS_LAST_RIGHT_ACTIVE, WINDOW_POS_Y_DEF, 24, 9, WIFI_winHandle0)
		{
			REPAINT_WINDOW()
			{
				WH->BeginDraw();
				WH->SetNormalChar();
				WH->SetTextColor(tfcWhite);

				int x = 0;

				WH->PutStr(x, 0, FSS("WIFI SSID:"));
				WH->PutStr(x, 1, FSS("WIFI IP:"));
				WH->PutStr(x, 2, FSS("STATUS:"));
				WH->PutStr(x, 3, FSS("RSSI:"));

				WH->PutStr(x, 5, FSS("WIFI:"));
				WH->PutStr(x, 6, FSS("INTERNET:"));

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
				case WL_NO_SHIELD:		WH->PutStr(x + 7, 2, FSS("NO SHIELD      ")); break;
				case WL_IDLE_STATUS:	WH->PutStr(x + 7, 2, FSS("IDLE STATUS    ")); break;
				case WL_NO_SSID_AVAIL:	WH->PutStr(x + 7, 2, FSS("NO SSID AVAIL  ")); break;
				case WL_SCAN_COMPLETED:	WH->PutStr(x + 7, 2, FSS("SCAN COMPLETED ")); break;
				case WL_CONNECTED:		WH->PutStr(x + 7, 2, FSS("CONNECTED      ")); break;
				case WL_CONNECT_FAILED:	WH->PutStr(x + 7, 2, FSS("CONNECT FAILED ")); break;
				case WL_CONNECTION_LOST:WH->PutStr(x + 7, 2, FSS("CONNECTION LOST")); break;
				case WL_DISCONNECTED:	WH->PutStr(x + 7, 2, FSS("DISCONNECTED   ")); break;
				default:				WH->PutStr(x + 7, 2, String(WiFi.status()).c_str()); break;
				}
				WH->PutStr(x + 5, 3, String(WiFi.RSSI()).c_str());
				WH->PutChar(' ');
				WH->PutChar(' ');

				switch (WiFiStatus)
				{
				case wsDisconnect: WH->PutStr(x + 5, 5, FSS("Disconnect")); break;
				case wsConnect:WH->PutStr(x + 5, 5, FSS("Connect   ")); break;
				}

				switch (WIFI_InternetStatus)
				{
				case isDisconnect: WH->PutStr(x + 9, 6, FSS("Disconnect")); break;
				case isConnect:WH->PutStr(x + 9, 6, FSS("Connect   ")); break;
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
#include <xb_board.h>
#include <xb_WIFI.h>
#include <xb_PING.h>
#include <xb_GUI.h>
#include <xb_GUI_Gadget.h>
#include <xb_PREFERENCES.h>
#ifdef ESP32
#include <ESPmDNS.h>
#include <WiFi.h>
#endif

#ifdef ARDUINO_OTA
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

TTaskDef XB_WIFI_DefTask = {1, &WIFI_Setup,&WIFI_DoLoop,&WIFI_DoMessage};

IPAddress WIFI_dnsip1(8, 8, 8, 8);
IPAddress WIFI_dnsip2(8, 8, 4, 4);
uint8_t WIFI_mac[6];

#ifdef XB_GUI
TWindowClass *WIFI_winHandle0;
TGADGETMenu *WIFI_menuhandle1;
TGADGETInputDialog *WIFI_inputdialoghandle0_ssid;
TGADGETInputDialog *WIFI_inputdialoghandle1_psw;
TGADGETInputDialog *WIFI_inputdialoghandle2_sta_ip;
TGADGETInputDialog *WIFI_inputdialoghandle3_mask_ip;
TGADGETInputDialog *WIFI_inputdialoghandle4_gateway_ip;
#endif

// Konfiguracja -----------------------------------------------------------------------------------------------------
bool CFG_WIFI_AutoConnect = false;
bool CFG_WIFI_DEBUG = true;

String CFG_WIFI_SSID = WIFI_SSID;
String CFG_WIFI_PSW = WIFI_PASSWORD;

uint32_t CFG_WIFI_StaticIP = 0;
uint32_t CFG_WIFI_MASK = 0;
uint32_t CFG_WIFI_GATEWAY = 0;


String CFG_WIFI_AdministratorSSID;
String CFG_WIFI_AdministratorPSW;
uint32_t CFG_WIFI_AdministratorStaticIP = 0;
uint32_t CFG_WIFI_AdministratorMASK = 0;
uint32_t CFG_WIFI_AdministratorGATEWAY = 0;

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

void WIFI_DoAllTaskWifiDisconnect(void)
{
	board.SendMessageToAllTask(IM_WIFI_DISCONNECT, doBACKWARD);
}

void WIFI_DoAllTaskInternetDisconnect(void)
{
	board.SendMessageToAllTask(IM_INTERNET_DISCONNECT, doBACKWARD);
}

	void WIFI_DoAllTaskInternetConnect(void)
	{
		board.SendMessageToAllTask(IM_INTERNET_CONNECT, doFORWARD);
	}

	void WIFI_DoAllTaskWiFiConnect(void)
	{
		board.SendMessageToAllTask(IM_WIFI_CONNECT, doFORWARD);
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
		WIFI_SetDisconnectInternet();
		WIFI_SetDisconnectWiFi();

		if (WiFi.status() == WL_CONNECTED)
		{
			board.Log(FSS("Disconnecting WIFI."), true, true);
		}
		else
		{
			board.Log(FSS("Reseting WIFI."), true, true);
		}
		WiFi.scanDelete();
		delay(100);
		board.Log('.');
		WiFi.disconnect(true);
		delay(100);
		board.Log('.');
		WiFi.mode(WIFI_OFF);
		delay(100);
		board.Log('.');

		WiFi.persistent(false);
		board.Log('.');
		delay(100);
		WiFi.setAutoConnect(false);
		board.Log('.');
		delay(100);
		WiFi.setAutoReconnect(false);
		board.Log('.');
		delay(100);
		WiFiFunction = wfHandle;
		board.Log(FSS("OK"));
	}

	bool WIFI_SetDefaultConfig(void)
	{
		bool result = true;
		IPAddress ip;

		CFG_WIFI_SSID = WIFI_SSID;
		CFG_WIFI_PSW = WIFI_PASSWORD;

		if (ip.fromString(WIFI_STATICIP))
		{
			CFG_WIFI_StaticIP = ip;

		}
		else
		{
			result = false;
		}
	
		if (ip.fromString(WIFI_MASK))
		{
			CFG_WIFI_MASK = ip;

		}
		else
		{
			result = false;
		}

		if (ip.fromString(WIFI_GATEWAY))
		{
			CFG_WIFI_GATEWAY = ip;

		}
		else
		{
			result = false;
		}

		//----

		CFG_WIFI_AdministratorSSID = "xbary-wm";
		CFG_WIFI_AdministratorPSW = "0987654321";

		if (ip.fromString(WIFI_ADMINISTRATORSTATICIP))
		{
			CFG_WIFI_AdministratorStaticIP = ip;

		}
		else
		{
			result = false;
		}
		CFG_WIFI_AdministratorMASK = IPAddress(255, 255, 255, 0);
		CFG_WIFI_AdministratorGATEWAY = IPAddress(192, 168, 137, 1);

		return result;
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

	#ifdef ARDUINO_OTA
	void WIFI_OTA_Init(void)
	{
		board.Log(FSS("ArduinoOTA Init"), true, true);
		board.Log('.');
		ArduinoOTA.setPort(8266);
		board.Log('.');
		ArduinoOTA.setHostname(FSS(DEVICE_NAME));
		board.Log('.');
		//ArduinoOTA.setPassword("0987654321");

		board.Log('.');
		#if !defined(_VMICRO_INTELLISENSE)
		ArduinoOTA.onStart([](void) {
			board.SendMessageOTAUpdateStarted();
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

void WIFI_PutDot_Debug()
{
	if (CFG_WIFI_DEBUG) board.Log('.');		
}

bool WIFI_LoadConfig()
{
	PREFERENCES_BeginSection("WIFI");
	{
		CFG_WIFI_DEBUG = PREFERENCES_GetBool("DEBUG", CFG_WIFI_DEBUG);  
		if (CFG_WIFI_DEBUG) board.Log(FSS("Load config"), true, true);
		CFG_WIFI_AutoConnect = PREFERENCES_GetBool("AUTOCONNECT", CFG_WIFI_AutoConnect); WIFI_PutDot_Debug();
		CFG_WIFI_SSID = PREFERENCES_GetString("SSID", CFG_WIFI_SSID); WIFI_PutDot_Debug();
		CFG_WIFI_PSW = PREFERENCES_GetString("PSW", CFG_WIFI_PSW); WIFI_PutDot_Debug();
		CFG_WIFI_StaticIP = PREFERENCES_GetUINT32("StaticIP", CFG_WIFI_StaticIP); WIFI_PutDot_Debug();
		CFG_WIFI_MASK = PREFERENCES_GetUINT32("Mask", CFG_WIFI_MASK); WIFI_PutDot_Debug();
		CFG_WIFI_GATEWAY = PREFERENCES_GetUINT32("Gateway", CFG_WIFI_GATEWAY); WIFI_PutDot_Debug();

		CFG_WIFI_AdministratorSSID = PREFERENCES_GetString("ASSID", CFG_WIFI_AdministratorSSID); WIFI_PutDot_Debug();
		CFG_WIFI_AdministratorPSW = PREFERENCES_GetString("APSW", CFG_WIFI_AdministratorPSW); WIFI_PutDot_Debug();
		CFG_WIFI_AdministratorStaticIP = PREFERENCES_GetUINT32("AStaticIP", CFG_WIFI_AdministratorStaticIP); WIFI_PutDot_Debug();
		CFG_WIFI_AdministratorMASK = PREFERENCES_GetUINT32("AMask", CFG_WIFI_AdministratorMASK); WIFI_PutDot_Debug();
		CFG_WIFI_AdministratorGATEWAY = PREFERENCES_GetUINT32("AGateway", CFG_WIFI_AdministratorGATEWAY); WIFI_PutDot_Debug();
	}
	PREFERENCES_EndSection();
	if (CFG_WIFI_DEBUG) board.Log(FSS("OK"));
	return true;
}

bool WIFI_SaveConfig()
{
	PREFERENCES_BeginSection("WIFI");
	{

		if (CFG_WIFI_DEBUG) board.Log(FSS("Save config"), true, true);

		PREFERENCES_PutBool("AUTOCONNECT", CFG_WIFI_AutoConnect); WIFI_PutDot_Debug();
		PREFERENCES_PutBool("DEBUG", CFG_WIFI_DEBUG); WIFI_PutDot_Debug();
		PREFERENCES_PutString("SSID", CFG_WIFI_SSID); WIFI_PutDot_Debug(); 
		PREFERENCES_PutString("PSW", CFG_WIFI_PSW); WIFI_PutDot_Debug(); 
		PREFERENCES_PutUINT32("StaticIP", CFG_WIFI_StaticIP); WIFI_PutDot_Debug();
		PREFERENCES_PutUINT32("Mask", CFG_WIFI_MASK); WIFI_PutDot_Debug();
		PREFERENCES_PutUINT32("Gateway", CFG_WIFI_GATEWAY); WIFI_PutDot_Debug();

		PREFERENCES_PutString("ASSID", CFG_WIFI_AdministratorSSID); WIFI_PutDot_Debug();
		PREFERENCES_PutString("APSW", CFG_WIFI_AdministratorPSW); WIFI_PutDot_Debug();
		PREFERENCES_PutUINT32("AStaticIP", CFG_WIFI_AdministratorStaticIP); WIFI_PutDot_Debug();
		PREFERENCES_PutUINT32("AMask", CFG_WIFI_AdministratorMASK); WIFI_PutDot_Debug();
		PREFERENCES_PutUINT32("AGateway", CFG_WIFI_AdministratorGATEWAY); WIFI_PutDot_Debug();
	}
	PREFERENCES_EndSection();

	if (CFG_WIFI_DEBUG) board.Log(FSS("OK"));
	return true;
}

void WIFI_Setup(void)
{
	board.Log(FSS("Init."), true, true);

	WiFiStatus = wsDisconnect;
	WIFI_InternetStatus = isDisconnect;

	#ifdef ESP8266 
	wifi_set_sleep_type(NONE_SLEEP_T);
	#endif
	WiFi.macAddress(WIFI_mac);

	{
		bool lastshowloginfo = XB_WIFI_DefTask.Task->ShowLogInfo;
		XB_WIFI_DefTask.Task->ShowLogInfo = false;
		WIFI_HardDisconnect();
		XB_WIFI_DefTask.Task->ShowLogInfo = lastshowloginfo;
	}
	board.Log('.');
	WiFiFunction = wfHandle;
	board.Log(FSS(".OK"));

	WIFI_LoadConfig();
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
					#ifdef ARDUINO_OTA
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
					BEGIN_WAITMS(hhh1, 20000)
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
			WiFi.setHostname(FSS(DEVICE_NAME));
			#else
			WiFi.hostname(FSS(DEVICE_NAME));
			#endif
			board.Log('.');
			PING_8888_addr = WIFI_dnsip1;
			PING_GATEWAY_addr = CFG_WIFI_GATEWAY;
			WiFi.config(IPAddress(CFG_WIFI_StaticIP), IPAddress(CFG_WIFI_GATEWAY), IPAddress(CFG_WIFI_MASK), WIFI_dnsip1, WIFI_dnsip2);
			board.Log('.');
			WiFi.setSleep(false);
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
		#ifdef ARDUINO_OTA
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
			#ifdef ARDUINO_OTA
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
	case IM_CONFIG_SAVE:
		{
			WIFI_SaveConfig();
			return true;
		}
		#ifdef XB_GUI		
	case IM_MENU:
		{
			switch (Am->Data.MenuData.TypeMenuAction)
			{
			case tmaOPEN_MAINMENU:
				{
					WIFI_menuhandle1 = GUIGADGET_CreateMenu(&XB_WIFI_DefTask, 1);
					break;
				}
			case tmaGET_INIT_MENU:
				{
					BEGIN_MENUINIT(1)
					{
						DEF_MENUINIT(11, 0, 40, -1, 0,true);
					}
					END_MENUINIT()
					return true;
				}
			case tmaGET_CAPTION_MENU_STRING:
				{
					BEGIN_MENUCAPTION(1)
					{
						DEF_MENUCAPTION("WIFI Configuration");
					}
					END_MENUCAPTION()
					return true;
				}
			case tmaGET_ITEM_MENU_STRING:
				{
					BEGIN_MENUITEMNAME(1)
					{
						DEF_MENUITEMNAME_CHECKED(0, String(FSS("Auto Connect")), CFG_WIFI_AutoConnect);
						DEF_MENUITEMNAME_CHECKED(1, String(FSS("Debug")), CFG_WIFI_DEBUG);
						DEF_MENUITEMNAME(2, String(FSS("Disconnect WIFI")));
						DEF_MENUITEMNAME(3, String(FSS("Show WIFI Status...")));
						DEF_MENUITEMNAME(4, String(FSS("Load Config")));
						DEF_MENUITEMNAME(5, String(FSS("Save Config")));
						DEF_MENUITEMNAME(6, "Set SSID [" + CFG_WIFI_SSID + "]");
						DEF_MENUITEMNAME(7, String(FSS("Set PASSWORD")));
						DEF_MENUITEMNAME(8, "Set STA IP      (" + IPAddress(CFG_WIFI_StaticIP).toString() + ")");
						DEF_MENUITEMNAME(9, "Set STA Mask    (" + IPAddress(CFG_WIFI_MASK).toString() + ")");
						DEF_MENUITEMNAME(10, "Set STA GateWay (" + IPAddress(CFG_WIFI_GATEWAY).toString() + ")");
					}
					END_MENUITEMNAME()
					return true;
				}
			case tmaCLICK_ITEM_MENU:
				{
					BEGIN_MENUCLICK(1)
					{
						EVENT_MENUCLICK(0)
						{
							CFG_WIFI_AutoConnect = !CFG_WIFI_AutoConnect;

							if (CFG_WIFI_AutoConnect == false)
							{
								WIFI_HardDisconnect();
							}

						}
						EVENT_MENUCLICK(1)
						{
							CFG_WIFI_DEBUG = !CFG_WIFI_DEBUG;
						}
						EVENT_MENUCLICK(2)
						{
							WIFI_HardDisconnect();
						}
						EVENT_MENUCLICK(3)
						{
							WIFI_winHandle0 = GUI_WindowCreate(&XB_WIFI_DefTask, 0, false);
						}
						EVENT_MENUCLICK(4)
						{
							WIFI_LoadConfig();
						}
						EVENT_MENUCLICK(5)
						{
							WIFI_SaveConfig();
						}
						EVENT_MENUCLICK(6)
						{
							WIFI_inputdialoghandle0_ssid = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 0);
						}
						EVENT_MENUCLICK(7)
						{
							WIFI_inputdialoghandle1_psw = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 1);
						}
						EVENT_MENUCLICK(8)
						{
							WIFI_inputdialoghandle2_sta_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 2);
						}
						EVENT_MENUCLICK(9)
						{
							WIFI_inputdialoghandle3_mask_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 3);
						}
						EVENT_MENUCLICK(10)
						{
							WIFI_inputdialoghandle4_gateway_ip = GUIGADGET_CreateInputDialog(&XB_WIFI_DefTask, 4);
						}

					}
					END_MENUCLICK()
					return true;
				}
			}
		}
	case IM_INPUTDIALOG:
	{
		BEGIN_INPUTDIALOGINIT(0)
			DEF_INPUTDIALOGINIT(tivDynArrayChar1, 16, &CFG_WIFI_SSID)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOGINIT(1)
			DEF_INPUTDIALOGINIT(tivDynArrayChar1, 16, &CFG_WIFI_PSW)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOGINIT(2)
			DEF_INPUTDIALOGINIT(tivIP, 15, &CFG_WIFI_StaticIP)
		END_INPUTDIALOGINIT()

		BEGIN_INPUTDIALOGINIT(3)
			DEF_INPUTDIALOGINIT(tivIP, 15, &CFG_WIFI_MASK)
		END_INPUTDIALOGINIT()
				
		BEGIN_INPUTDIALOGINIT(4)
			DEF_INPUTDIALOGINIT(tivIP, 15, &CFG_WIFI_GATEWAY)
		END_INPUTDIALOGINIT()

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
			switch (Am->Data.WindowData.WindowAction)
			{
			case waCreate:
				{
					if (Am->Data.WindowData.ID == 0)
					{
						Am->Data.WindowData.ActionData.Create.X = -1;
						Am->Data.WindowData.ActionData.Create.Y = 0;
						Am->Data.WindowData.ActionData.Create.Width = 24;
						Am->Data.WindowData.ActionData.Create.Height = 9;
						return true;
					}
					break;
				}
			case waDestroy:
				{
					if (Am->Data.WindowData.ID == 0)
					{
						//WIFI_winHandle0 = NULL;
					}
					return true;
				}
			case waGetCaptionWindow:
				{
					if (Am->Data.WindowData.ID == 0)
					{
						*(Am->Data.WindowData.ActionData.GetCaption.PointerString) = FSS("WIFI");
						return true;
					}
					break;
				}
			case waRepaint:
				{
					if (Am->Data.WindowData.ID == 0)
					{
						if (WIFI_winHandle0 != NULL)
						{
							WIFI_winHandle0->BeginDraw();
							WIFI_winHandle0->SetNormalChar();
							WIFI_winHandle0->SetTextColor(tfcWhite);

							int x = 0;

							WIFI_winHandle0->PutStr(x, 0, FSS("WIFI SSID:"));
							WIFI_winHandle0->PutStr(x, 1, FSS("WIFI IP:"));
							WIFI_winHandle0->PutStr(x, 2, FSS("STATUS:"));
							WIFI_winHandle0->PutStr(x, 3, FSS("RSSI:"));

							WIFI_winHandle0->PutStr(x, 5, FSS("WIFI:"));
							WIFI_winHandle0->PutStr(x, 6, FSS("INTERNET:"));

							WIFI_winHandle0->EndDraw();
						}
						return true;
					}
					break;
				}
			case waRepaintData:
				{
					if (Am->Data.WindowData.ID == 0)
					{
						if (WIFI_winHandle0 != NULL)
						{
							WIFI_winHandle0->BeginDraw();
							WIFI_winHandle0->SetNormalChar();
							WIFI_winHandle0->SetBoldChar();
							WIFI_winHandle0->SetTextColor(tfcYellow);

							int x = 0;

							WIFI_winHandle0->PutStr(x + 10, 0, WiFi.SSID().c_str());
							WIFI_winHandle0->PutStr(x + 8, 1, WiFi.localIP().toString().c_str());

							switch (WiFi.status())
							{
							case WL_NO_SHIELD:		WIFI_winHandle0->PutStr(x + 7, 2, FSS("NO SHIELD      ")); break;
							case WL_IDLE_STATUS:	WIFI_winHandle0->PutStr(x + 7, 2, FSS("IDLE STATUS    ")); break;
							case WL_NO_SSID_AVAIL:	WIFI_winHandle0->PutStr(x + 7, 2, FSS("NO SSID AVAIL  ")); break;
							case WL_SCAN_COMPLETED:	WIFI_winHandle0->PutStr(x + 7, 2, FSS("SCAN COMPLETED ")); break;
							case WL_CONNECTED:		WIFI_winHandle0->PutStr(x + 7, 2, FSS("CONNECTED      ")); break;
							case WL_CONNECT_FAILED:	WIFI_winHandle0->PutStr(x + 7, 2, FSS("CONNECT FAILED ")); break;
							case WL_CONNECTION_LOST:WIFI_winHandle0->PutStr(x + 7, 2, FSS("CONNECTION LOST")); break;
							case WL_DISCONNECTED:	WIFI_winHandle0->PutStr(x + 7, 2, FSS("DISCONNECTED   ")); break;
							default:				WIFI_winHandle0->PutStr(x + 7, 2, String(WiFi.status()).c_str()); break;
							}
							WIFI_winHandle0->PutStr(x + 5, 3, String(WiFi.RSSI()).c_str());
							WIFI_winHandle0->PutChar(' ');
							WIFI_winHandle0->PutChar(' ');

							switch (WiFiStatus)
							{
							case wsDisconnect: WIFI_winHandle0->PutStr(x + 5, 5, FSS("Disconnect")); break;
							case wsConnect:WIFI_winHandle0->PutStr(x + 5, 5, FSS("Connect   ")); break;
							}

							switch (WIFI_InternetStatus)
							{
							case isDisconnect: WIFI_winHandle0->PutStr(x + 9, 6, FSS("Disconnect")); break;
							case isConnect:WIFI_winHandle0->PutStr(x + 9, 6, FSS("Connect   ")); break;
							}

							WIFI_winHandle0->EndDraw();
						}
						return true;
					}
					break;
				}


			default: break;
			}
			break;
		}
		#endif
	}
	return false;
}
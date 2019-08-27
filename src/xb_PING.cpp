#include <xb_board.h>
#include <xb_PING.h>
#include <WiFi.h>
#include <lwip/dns.h>


#ifdef XB_GUI
#include <xb_GUI_Gadget.h>
TGADGETMenu* XB_PING_menuhandle;
#endif

void XB_PING_Setup();
uint32_t XB_PING_DoLoop();
bool XB_PING_DoMessage(TMessageBoard *Am);

TTaskDef XB_PING_DefTask = { 1, &XB_PING_Setup,&XB_PING_DoLoop,&XB_PING_DoMessage };
TPING_FunctionStep PING_FunctionStep;
bool Show_Debug = true;


bool PING_8888_IS = true;
int PING_8888_TC = 0;

bool PING_GATEWAY_IS = true;
uint32_t PING_8888_addr = IPAddress(8, 8, 4, 4);
uint32_t PING_GATEWAY_addr = IPAddress(192, 168, 1, 1);

void XB_PING_Setup()
{
	board.Log(FSS("Init"), true, true,tlInfo);
	board.LoadConfiguration();
	board.Log(FSS("...OK"));

	PING_FunctionStep = pfsIDLE;
}

WiFiClient pingoclient;

uint32_t XB_PING_DoLoop()
{
	switch (PING_FunctionStep)
	{
	case pfsIDLE:
	{
		return 0;
	}
	case pfsCheckPingOK:
	{
		if (!pingoclient.connected())
		{
			if (Show_Debug) board.Log("Do check connect to IP: 172.217.20.164:80 ...",true,true);
			int c = pingoclient.connect(IPAddress(172,217,20,164), 80, 500);
			//if (Show_Debug) board.Log("Do check connect to URL: google.com:80 ...", true, true);
			//int c = pingoclient.connect("www.google.com", 80, 1000);
			if ( c == 1)
			{
				delay(0);
				if (pingoclient.connected())
				{
					if (Show_Debug) board.Log(".OK");
					PING_8888_IS = true;
					PING_8888_TC = 0;
					delay(0);
					pingoclient.stop();
					return 7500;
				}
				else
				{
					PING_8888_TC++;
					if (Show_Debug) board.Log(String(".ERROR("+String(PING_8888_TC)+").RESULT("+String(c)+")").c_str());
					if (PING_8888_TC >= 2)
					{
						if (Show_Debug) board.Log(".Internet disconnect");
						PING_8888_IS = false;
						return 5000;
					}
				}
			}
			else
			{
				PING_8888_TC++;
				if (Show_Debug) board.Log(String(".ERROR(" + String(PING_8888_TC) + ").RESULT(" + String(c) + ")").c_str());
				if (PING_8888_TC >= 1)
				{
					if (Show_Debug) board.Log(".Internet disconnect");
					delay(0);
					pingoclient.stop();
					PING_8888_IS = false;
					return 5000;
				}
			}
		}
		else
		{
			PING_8888_IS = true;
			PING_8888_TC = 0;
		}

		return 7500;
	}
	default: PING_FunctionStep = pfsIDLE; break;
	}
	return 0;
}




bool XB_PING_DoMessage(TMessageBoard *Am)
{
	switch (Am->IDMessage)
	{
	case IM_WIFI_CONNECT:
	{
		PING_8888_IS = false;
		PING_GATEWAY_IS = true;
		PING_FunctionStep = pfsCheckPingOK;
		return true;
	}
	case IM_WIFI_DISCONNECT:
	{
		PING_FunctionStep = pfsIDLE;
		PING_8888_IS = false;
		PING_GATEWAY_IS = false;
		return true;
	}
	case IM_OTA_UPDATE_STARTED:
	{
		PING_FunctionStep = pfsIDLE;
		PING_8888_IS = false;
		PING_GATEWAY_IS = false;
		return true;
	}
	case IM_GET_TASKNAME_STRING:
	{
		*(Am->Data.PointerString) = FSS("PING");
		return true;
	}
	case IM_GET_TASKSTATUS_STRING:
	{
		switch (PING_FunctionStep)
		{
		case pfsIDLE: *(Am->Data.PointerString) = FSS("IDLE           "); break;
		case pfsCheckPingOK: 
		{
			*(Am->Data.PointerString) = FSS("C.P. 8888:");
			*(Am->Data.PointerString) += String((int)PING_8888_IS)+ " GW:";
			*(Am->Data.PointerString) += String((int)PING_GATEWAY_IS);

			break;
		}
		default: *(Am->Data.PointerString) = FSS("..."); break;
		}
	
		return true;
	}
#ifdef XB_GUI

	case IM_FREEPTR:
	{
		FREEPTR(XB_PING_menuhandle);
		return true;
	}
	case IM_MENU:
	{
		OPEN_MAINMENU()
		{
			XB_PING_menuhandle = GUIGADGET_CreateMenu(&XB_PING_DefTask, 0,false,X,Y);
		}

		BEGIN_MENU(0, "OPTION", WINDOW_POS_LAST_RIGHT_ACTIVE, WINDOW_POS_Y_DEF, 32, MENU_AUTOCOUNT, 0, true);
		{
			BEGIN_MENUITEM_CHECKED("Show debug", taLeft, Show_Debug)
			{
				CLICK_MENUITEM()
				{
					Show_Debug = !Show_Debug;
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			CONFIGURATION_MENUITEMS()
		}
		END_MENU()

		return true;
	}
#endif
	case IM_LOAD_CONFIGURATION:
	{
		if (board.PREFERENCES_BeginSection("XB_PING"))
		{
			Show_Debug = board.PREFERENCES_GetBool("Show_Debug", Show_Debug);
			board.PREFERENCES_EndSection();
		}
		return true;
	}
	case IM_SAVE_CONFIGURATION:
	{
		if (board.PREFERENCES_BeginSection("XB_PING"))
		{
			board.PREFERENCES_PutBool("Show_Debug", Show_Debug);
			board.PREFERENCES_EndSection();
		}
		return true;
	}
	default: break;
	}
	return false;
}
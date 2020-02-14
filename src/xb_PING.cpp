#include <xb_board.h>
#include <xb_PING.h>

#ifdef XB_WIFI
#include <xb_WIFI.h>
#endif

#ifdef XB_GUI
#include <xb_GUI_Gadget.h>
TGADGETMenu* XB_PING_menuhandle;
TGADGETInputDialog* XB_PING_InputDialog1_TickIntervalCheck;
TGADGETInputDialog* XB_PING_InputDialog2_pingip;
TGADGETInputDialog* XB_PING_InputDialog3_TickFirstCheck;
#endif

typedef enum {
	pfsIDLE,
	pfsCheckPingOK_START,
	pfsCheckPingOK_STEP1,
	pfsCheckPingOK_STEP2
} TPING_FunctionStep;
TPING_FunctionStep PING_FunctionStep;

bool CFG_Show_Debug = true;
bool CFG_DoPING = true;
uint32_t CFG_TickIntervalCheck = 5000;
uint32_t CFG_TickFirstCheck = 10000;
IPAddress CFG_pingIP(172, 217, 20, 164);


bool PING_8888_IS = false;
int PING_8888_TC = 0;
bool PING_GATEWAY_IS = false;


void XB_PING_Setup()
{
	board.Log(FSS("Init"), true, true,tlInfo);
	board.LoadConfiguration();
	board.Log(FSS("...OK"));

	PING_FunctionStep = pfsIDLE;
}

WiFiClient pingoclient;
uint32_t waitforconnect = 1000;
int resultconnect = 0;
uint32_t lasttickconnect = 50;
uint32_t tickconnect = 0;

uint32_t XB_PING_DoLoop()
{
	switch (PING_FunctionStep)
	{
	case pfsIDLE:
	{
		return 0;
	}
	case pfsCheckPingOK_START:
	{
		if (!CFG_DoPING) return 0;

		PING_FunctionStep = pfsCheckPingOK_STEP1;
		return CFG_TickFirstCheck;
	}
	case pfsCheckPingOK_STEP1:
	{
		if (!CFG_DoPING) return 0;
		
		if (CFG_Show_Debug) board.Log(String("Do check connect to IP:"+ CFG_pingIP.toString()+" ").c_str(), true, true);
		
		tickconnect = SysTickCount;
		resultconnect = pingoclient.connect(CFG_pingIP, 80, waitforconnect);

		if (resultconnect==0)
		{
			if (CFG_Show_Debug) board.Log(String(".ERRORCNT(" + String(PING_8888_TC) + ").RESULT(" + String(resultconnect) + ")").c_str());
			waitforconnect += 10;
			PING_8888_IS = false;
			PING_8888_TC++;
			pingoclient.stop();
			if (PING_8888_TC > 20)
			{
				PING_8888_TC = 0;
				return  CFG_TickIntervalCheck;
			}
			return 100;
		}

		PING_FunctionStep = pfsCheckPingOK_STEP2;
		return lasttickconnect;
	}
	case pfsCheckPingOK_STEP2:
	{
		if (!CFG_DoPING) return 0;

		if (resultconnect == 1)
		{
			if (pingoclient.connected() != 0)
			{
				if (CFG_Show_Debug) board.Log(".OK");
				PING_8888_IS = true;
				PING_8888_TC = 0;
				waitforconnect = 1000;
				pingoclient.stop();
			}
			else
			{
				if (CFG_Show_Debug) board.Log(String(".ERRORCNT(" + String(PING_8888_TC) + ").RESULT(" + String(resultconnect) + ")").c_str());
				waitforconnect += 100;
				PING_8888_IS = false;
				PING_8888_TC++;
			
			}
		}
		else
		{
			if (CFG_Show_Debug) board.Log(String(".ERRORCNT(" + String(PING_8888_TC) + ").RESULT(" + String(resultconnect) + ")").c_str());
			waitforconnect += 100;
			PING_8888_IS = false;
			PING_8888_TC++;
			
		}

		if (CFG_Show_Debug)
			if (tickconnect != 0)
			{
				board.Log(String(".(" + String(SysTickCount - tickconnect) + "ms)").c_str());
			}

		PING_FunctionStep = pfsCheckPingOK_STEP1;

		if (PING_8888_TC >= 5)
		{
			WIFI_HardDisconnect();
			waitforconnect = 1000;
			PING_8888_TC = 0;
		}
		else
		{
			if (PING_8888_TC==0)
				if ((SysTickCount - tickconnect) > 50)
				{
					lasttickconnect = (SysTickCount - tickconnect) - 50;
					if (lasttickconnect >= 500) lasttickconnect = 50;
				}
		}

		return CFG_TickIntervalCheck;
	}
	default: PING_FunctionStep = pfsIDLE; break;
	}
	return 0;
}

bool XB_PING_DoMessage(TMessageBoard *Am)
{
	switch (Am->IDMessage)
	{
	case IM_FREEPTR:
	{
#ifdef XB_GUI
		FREEPTR(XB_PING_menuhandle);
		FREEPTR(XB_PING_InputDialog1_TickIntervalCheck)
		FREEPTR(XB_PING_InputDialog2_pingip)
		FREEPTR(XB_PING_InputDialog3_TickFirstCheck)
#endif
		return true;
	}

	case IM_ETH_CONNECT:
	case IM_WIFI_CONNECT:
	{
		PING_8888_IS = false;
		PING_GATEWAY_IS = true;
		PING_FunctionStep = pfsCheckPingOK_START;
		return true;
	}
	case IM_ETH_DISCONNECT:
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
		GET_TASKNAME("PING");
		return true;
	}
	case IM_GET_TASKSTATUS_STRING:
	{
		switch (PING_FunctionStep)
		{
		case pfsIDLE: *(Am->Data.PointerString) = FSS("IDLE           "); break;
		case pfsCheckPingOK_START: *(Am->Data.PointerString) = FSS("Starting...          "); break;
		case pfsCheckPingOK_STEP1: 
		{
			*(Am->Data.PointerString) = FSS("C.P.1 INET:");
			*(Am->Data.PointerString) += String((int)PING_8888_IS) + " GW:";
			*(Am->Data.PointerString) += String((int)PING_GATEWAY_IS);
			break;
		}
		case pfsCheckPingOK_STEP2:
		{
			*(Am->Data.PointerString) = FSS("C.P.2 INET:");
			*(Am->Data.PointerString) += String((int)PING_8888_IS)+ " GW:";
			*(Am->Data.PointerString) += String((int)PING_GATEWAY_IS);

			break;
		}
		default: *(Am->Data.PointerString) = FSS("..."); break;
		}
	
		return true;
	}
#ifdef XB_GUI
	case IM_MENU:
	{
		OPEN_MAINMENU()
		{
			XB_PING_menuhandle = GUIGADGET_CreateMenu(&XB_PING_DefTask, 0,false,X,Y);
		}

		BEGIN_MENU(0, "OPTION", WINDOW_POS_LAST_RIGHT_ACTIVE, WINDOW_POS_Y_DEF, 64, MENU_AUTOCOUNT, 0, true);
		{
			BEGIN_MENUITEM_CHECKED("Enable check", taLeft, CFG_DoPING)
			{
				CLICK_MENUITEM()
				{
					CFG_DoPING = !CFG_DoPING;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM_CHECKED("Show debug", taLeft, CFG_Show_Debug)
			{
				CLICK_MENUITEM()
				{
					CFG_Show_Debug = !CFG_Show_Debug;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Set tick interval check ("+String(CFG_TickIntervalCheck)+"ms)", taLeft)
			{
				CLICK_MENUITEM()
				{
					XB_PING_InputDialog1_TickIntervalCheck = GUIGADGET_CreateInputDialog(&XB_PING_DefTask, 1, true);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Set IP to check (" + CFG_pingIP.toString() + ")", taLeft)
			{
				CLICK_MENUITEM()
				{
					XB_PING_InputDialog2_pingip = GUIGADGET_CreateInputDialog(&XB_PING_DefTask, 2, true);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Set Tick First Check IP (" + String(CFG_TickFirstCheck) + "ms)", taLeft)
			{
				CLICK_MENUITEM()
				{
					XB_PING_InputDialog3_TickFirstCheck = GUIGADGET_CreateInputDialog(&XB_PING_DefTask, 3, true);
				}
			}
			END_MENUITEM()

			SEPARATOR_MENUITEM()
			CONFIGURATION_MENUITEMS()
		}
		END_MENU()

		return true;
	}
	case IM_INPUTDIALOG:
	{
		BEGIN_DIALOG_MINMAX(1, "Set tick interval check", "Set tick interval check (in ms, exp.1s=1000ms):", tivUInt32, 12, &CFG_TickIntervalCheck, 1000, 60000)
		{
		}
		END_DIALOG()
		BEGIN_DIALOG(2, "Set IP to check", "Set IP to check", tivIP, 16, &CFG_pingIP)
		{
		}
		END_DIALOG()
		BEGIN_DIALOG_MINMAX(3, "Set tick first check", "Set tick first check (in ms, exp.1s=1000ms):", tivUInt32, 12, &CFG_TickFirstCheck, 5000, 60000)
		{
		}
		END_DIALOG()

		return true;
	}
#endif
	case IM_LOAD_CONFIGURATION:
	{
		if (board.PREFERENCES_BeginSection("XB_PING"))
		{
			CFG_Show_Debug = board.PREFERENCES_GetBool("Show_Debug", CFG_Show_Debug);
			CFG_TickIntervalCheck = board.PREFERENCES_GetUINT32("IntervalCheck", CFG_TickIntervalCheck);
			CFG_pingIP = board.PREFERENCES_GetUINT32("pingIP", (uint32_t)CFG_pingIP);
			CFG_TickFirstCheck = board.PREFERENCES_GetUINT32("TickFirstCheck", CFG_TickFirstCheck);

			board.PREFERENCES_EndSection();
		}
		else
		{
			return false;
		}
		return true;
	}
	case IM_SAVE_CONFIGURATION:
	{
		if (board.PREFERENCES_BeginSection("XB_PING"))
		{
			board.PREFERENCES_PutBool("Show_Debug", CFG_Show_Debug);
			board.PREFERENCES_PutUINT32("IntervalCheck", CFG_TickIntervalCheck);
			board.PREFERENCES_PutUINT32("pingIP", (uint32_t)CFG_pingIP);
			board.PREFERENCES_PutUINT32("TickFirstCheck", CFG_TickFirstCheck);
			board.PREFERENCES_EndSection();
		}
		else
		{
			return false;
		}
		return true;
	}
	default: break;
	}
	return false;
}

TTaskDef XB_PING_DefTask = { 1, &XB_PING_Setup,&XB_PING_DoLoop,&XB_PING_DoMessage };

#include <xb_board.h>
#include <xb_PING.h>
#include <WiFi.h>
#include <lwip/dns.h>

void XB_PING_Setup();
uint32_t XB_PING_DoLoop();
bool XB_PING_DoMessage(TMessageBoard *Am);

TTaskDef XB_PING_DefTask = { 1, &XB_PING_Setup,&XB_PING_DoLoop,&XB_PING_DoMessage };
TPING_FunctionStep PING_FunctionStep;

bool PING_8888_IS = true;
int PING_8888_TC = 0;

bool PING_GATEWAY_IS = true;
uint32_t PING_8888_addr = IPAddress(8, 8, 4, 4);
uint32_t PING_GATEWAY_addr = IPAddress(192, 168, 1, 1);

void XB_PING_Setup()
{
	board.Log(FSS("Init"), true, true,tlInfo);

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
			
			pingoclient.connect("google.com", 80,1000);
			if (pingoclient.connected())
			{
				pingoclient.stop();
				PING_8888_IS = true;
				PING_8888_TC = 0;
				return 1750;
			}
			else
			{
				PING_8888_TC++;
				if (PING_8888_TC >= 3)
				{
					PING_8888_IS = false;
				}
				return 600;
			}
			return 0;
		}
		else
		{
			PING_8888_IS = true;
		}

		return 700;
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
	default: break;
	}
	return false;
}
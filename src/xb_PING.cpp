#include <xb_board.h>
#include <xb_PING.h>
#include <utils\esp_ping.h>
#include <utils\ping.h>
#include <WiFi.h>

void XB_PING_Setup();
uint32_t XB_PING_DoLoop();
bool XB_PING_DoMessage(TMessageBoard *Am);

TTaskDef XB_PING_DefTask = { 1, &XB_PING_Setup,&XB_PING_DoLoop,&XB_PING_DoMessage };
TPING_FunctionStep PING_FunctionStep;

bool PING_8888_IS = true;
bool PING_GATEWAY_IS = true;
uint32_t PING_8888_addr = IPAddress(8, 8, 8, 8);
uint32_t PING_GATEWAY_addr = IPAddress(192, 168, 1, 1);

void XB_PING_Setup()
{
	board.Log(FSS("Init"), true, true,tlInfo);

	board.Log(FSS("...OK"));
	
	PING_FunctionStep = pfsIDLE;
}

uint32_t XB_PING_DoLoop()
{
	if (WiFi.status()==WL_CONNECTED ) docheckping = true;

	switch (PING_FunctionStep)
	{
	case pfsIDLE:
	{
		return 0;
	}
	case pfsCheckPingOK:
	{
		String s;
		static int ping_0_errorcount = 0;
		static int ping_1_errorcount = 0;

		if (ping_option_info[0].ping_OK == false)
		{
			ping_0_errorcount++;
			if (ping_0_errorcount > 5)
			{
				PING_8888_IS = false;
				ping_0_errorcount = 0;
			}
		}
		else
		{
			ping_0_errorcount = 0;
			PING_8888_IS = true;
		}

		if (ping_option_info[1].ping_OK == false)
		{
			ping_1_errorcount++;
			if (ping_1_errorcount > 5)
			{
				PING_GATEWAY_IS = false;
				ping_option_info[0].ping_OK = false;
				PING_8888_IS = false;
				ping_1_errorcount = 0;
			}
		}
		else
		{
			ping_1_errorcount = 0;
			PING_GATEWAY_IS = true;
		}
		//docheckping = false;
		return 500;
	}
	default: PING_FunctionStep = pfsIDLE; break;
	}
	//docheckping = false;
	return 0;
}

bool XB_PING_DoMessage(TMessageBoard *Am)
{
	switch (Am->IDMessage)
	{
	case IM_WIFI_CONNECT:
	{
		PING_FunctionStep = pfsCheckPingOK;
	
		PING_GATEWAY_addr = WiFi.gatewayIP();

		esp_ping_set_target(0, PING_TARGET_IP_ADDRESS, &PING_8888_addr, 4);
		esp_ping_set_target(1, PING_TARGET_IP_ADDRESS, &PING_GATEWAY_addr, 4);

		PING_8888_IS = false;
		ping_option_info[0].ping_OK = false;
		PING_GATEWAY_IS = true;
		ping_option_info[1].ping_OK = true;

		ping_init();

		return true;
	}
	case IM_WIFI_DISCONNECT:
	{
		PING_FunctionStep = pfsIDLE;
		PING_8888_IS = false;
		ping_option_info[0].ping_OK = false;
		PING_GATEWAY_IS = false;
		ping_option_info[1].ping_OK = false;
		ping_deinit();
		statusdoping = 0;


		return true;
	}
	case IM_OTA_UPDATE_STARTED:
	{
		PING_FunctionStep = pfsIDLE;
		PING_8888_IS = false;
		ping_option_info[0].ping_OK = false;
		PING_GATEWAY_IS = false;
		ping_option_info[1].ping_OK = false;
		ping_deinit();
		statusdoping = 0;
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
// xb_PING.h

#ifndef _XB_PING_h
#define _XB_PING_h

#include <xb_board.h>

typedef enum {
	pfsIDLE,
	pfsCheckPingOK
} TPING_FunctionStep;

extern TTaskDef XB_PING_DefTask;
extern bool PING_8888_IS;
extern bool PING_GATEWAY_IS;
extern uint32_t PING_8888_addr;
extern uint32_t PING_GATEWAY_addr;


#endif


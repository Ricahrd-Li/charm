
#ifndef __CHARM_EVENTS_H__
#define __CHARM_EVENTS_H__

#include "charmProjections.h"
#include "traceCoreAPI.h"

/* Language ID */
#define _CHARM_LANG_ID		2	// language ID for charm 

/* Event IDs */
#define  _E_CREATION           	1
#define  _E_BEGIN_PROCESSING   	2
#define  _E_END_PROCESSING     	3
#define  _E_ENQUEUE            	4
#define  _E_DEQUEUE            	5
#define  _E_BEGIN_COMPUTATION  	6
#define  _E_END_COMPUTATION    	7
#define  _E_BEGIN_INTERRUPT    	8
#define  _E_END_INTERRUPT      	9
#define  _E_MSG_RECV_CHARM     	10
#define  _E_USER_EVENT_CHARM	13
#define  _E_BEGIN_PACK          16
#define  _E_END_PACK            17
#define  _E_BEGIN_UNPACK        18
#define  _E_END_UNPACK          19

/* Trace Macros */
#define REGISTER_CHARM \
	{ RegisterLanguage(_CHARM_LANG_ID, "charm"); \
	  RegisterEvent(_CHARM_LANG_ID, _E_CREATION         ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_BEGIN_PROCESSING ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_END_PROCESSING   ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_ENQUEUE          ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_DEQUEUE          ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_BEGIN_COMPUTATION); \
	  RegisterEvent(_CHARM_LANG_ID, _E_END_COMPUTATION  ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_BEGIN_INTERRUPT  ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_END_INTERRUPT    ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_MSG_RECV_CHARM   ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_USER_EVENT_CHARM ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_BEGIN_PACK       ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_END_PACK         ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_BEGIN_UNPACK     ); \
	  RegisterEvent(_CHARM_LANG_ID, _E_END_UNPACK       ); \
	}
#define _LOG_E_CREATION_1(env) 		{ creation(env); }
#define _LOG_E_CREATION_N(env, n) 	{ creation(env, n); }
#define _LOG_E_BEGIN_EXECUTE(env) 	{ beginExecute(env); }
#define _LOG_E_BEGIN_EXECUTE_DETAILED(event, msgType, ep, srcPe, ml) \
	{ beginExecuteDetailed(event, msgType, ep, srcPe, ml); }
#define _LOG_E_END_EXECUTE()	 	{ endExecute(); }
//TODO#define _LOG_E_BEGIN_PROCESSING() 
//TODO#define _LOG_E_END_PROCESSING() 
#define _LOG_E_ENQUEUE(env) 		{ enqueue(env); }
#define _LOG_E_DEQUEUE(env) 		{ dequeue(env); }
#define _LOG_E_BEGIN_COMPUTATION() 	{ beginComputation(); }
#define _LOG_E_END_COMPUTATION() 	{ endComputation(); }
//#define _LOG_E_BEGIN_INTERRUPT()
//#define _LOG_E_END_INTERRUPT() 
#define _LOG_E_MSG_RECV_CHARM(env, pe) 	{ messageRecv(env, pe); }
#define _LOG_E_USER_EVENT_CHARM(x) 		{ userEvent(x); }
#define _LOG_E_BEGIN_PACK() 			{ beginPack(); }
#define _LOG_E_END_PACK() 				{ endPack(); }
#define _LOG_E_BEGIN_UNPACK() 			{ beginUnpack(); }
#define _LOG_E_END_UNPACK() 			{ endUnpack(); }

#endif

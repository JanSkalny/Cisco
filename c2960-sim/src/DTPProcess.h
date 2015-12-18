#pragma once

#include "Line.h"
#include "Core.h"
#include "process.h"

class Interface;

class DTPProcess :
	public Process
{
	typedef enum {
		IF_MODE_ACCESS,
		IF_MODE_DYNAMIC_AUTO,
		IF_MODE_DYNAMIC_DESIRABLE,
		IF_MODE_TRUNK,
	} IfMode;


	typedef enum {
		TAS_ON=1,
		TAS_OFF=2,
		TAS_DESIRABLE=3,
		TAS_AUTO=4
	} Tas;

	typedef enum {
		TOS_ACCESS=0,
		TOS_TRUNK=8,
	} Tos;

	typedef enum {
		TAT_NEGO = 0,
		TAT_NATIVE = 1,
		TAT_ISL = 2,
		TAT_1Q = 5
	} Tat;

	typedef enum {
		TOT_NATIVE = 2,
		TOT_ISL = 4,
		TOT_1Q = 10
	} Tot;

protected:
	time_t m_tLastEvent;
	Interface *m_pInterface;

	IfMode m_mode;
	bool m_bEnable;

public:
	DTPProcess(Interface *pInterface);
	~DTPProcess(void);

	void sendUpdate();

	int getNextEventTimeout();
	bool onTimeout();
	bool onRecv(Frame *pFrame);
	bool onUp(Interface *pInterface);

	void setMode(IfMode mode);
	char *getModeDesc();
	bool isDynamic() {
		if (m_mode == IF_MODE_DYNAMIC_AUTO || 
			m_mode == IF_MODE_DYNAMIC_DESIRABLE)
			return true;
		return false;
	}

	bool isEnabled() { return m_bEnable; }

	void start();
	void stop();

	static void registerCommands();

	CMD(cmdInterfaceSwModeAccess) {
		pInterface->getDTPProc()->setMode(IF_MODE_ACCESS);
	}
	CMD(cmdInterfaceSwModeTrunk) {
		pInterface->getDTPProc()->setMode(IF_MODE_TRUNK);
	}
	CMD(cmdInterfaceSwModeDynamicAuto) {
		pInterface->getDTPProc()->setMode(IF_MODE_DYNAMIC_AUTO);
	}
	CMD(cmdInterfaceSwModeDynamicDesirable) {
		pInterface->getDTPProc()->setMode(IF_MODE_DYNAMIC_DESIRABLE);
	}
	CMD(cmdInterfaceSwNoMode) {
		pInterface->getDTPProc()->setMode(IF_MODE_DYNAMIC_DESIRABLE);
	}
	CMD(cmdInterfaceSwNonegotiate) {
		pInterface->getDTPProc()->start();	
	}
	CMD(cmdInterfaceSwNoNonegotiate) {
		pInterface->getDTPProc()->stop();
	}
};


#pragma once

#include "ProcessList.h"

class Line;
class Command;
class Interface;

class Core
{
public:
	typedef enum {
		MAX_EVENTS = 1024
	};

protected:
	bool m_bUpdateEventList;
	int m_nEventCount;
	HANDLE m_hEvents[MAX_EVENTS];
	HANDLE m_hUpdateEvent;

	void doUpdateEventList();
	int getNextEventTimeout();

	ProcessList m_proc;

public:
	Core(void);
	~Core(void);

	void work();
	void waitForEvents();
	void execCommands();
	void execProcesses();
	int pollInterfaces();

	bool processFrame(Frame *pFrame);
	bool validateFrame(Frame *pFrame);
	bool forwardFrame(Frame *pFrame);
	bool sendFrame(Interface *pInterface, Frame *pFrame);

	void updateEventList();

	void registerProcess(Process *pProc);
	void unregisterProcess(Process *pProc);

	static void registerCommands();

	CMD(cmdConfigureTerminal) {
		if (pLine->getMode() == MODE_EXEC)
			pLine->setMode(MODE_CONF);
	}
	CMD(cmdExitConfigure) {
		if (pLine->getMode() == MODE_CONF)
			pLine->setMode(MODE_EXEC);
	}
	CMD(cmdShowVersion);
	CMD(cmdEnable) {
		pLine->setPrivilegeLevel(15);
	}
	CMD(cmdDisable) {
		pLine->setPrivilegeLevel(1);
	}
};


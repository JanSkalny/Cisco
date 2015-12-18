#pragma once

#include "Line.h"
#include "CommandTree.h"
#include "Core.h"
#include "Vlan.h"

class PortSecurity;
class DTPProcess;

class Interface
{

public:
	enum {
		MAX_NAME_LEN = 256,
		MAX_DESC_LEN = 1024,
	};


protected:
	char m_sName[MAX_NAME_LEN];
	char m_sNameShort[MAX_NAME_LEN];
	char m_sDescription[MAX_DESC_LEN];

	bool m_bShutdown;
	bool m_bLineState;
	bool m_bProtoState;

	bool m_bCDPEnabled;

	MAC m_hwAddr;

	VlanId m_swNativeVlan;
	VlanId m_swAccessVlan;
	bool m_bIsTrunk;

	ProcessList m_processes;
	DTPProcess *m_pDTPProc;
	PortSecurity *m_pPortSecurityProc;

	Frame *onRecv(Frame *pFrame);
	bool onSend(Frame *pFrame);
	void onUp();
	void onDown();

public:
	Interface(const char *sName, MAC addr);
	~Interface(void);

	virtual bool send(Frame* frame) = 0;
	virtual Frame* recv() = 0;

#ifdef WIN32
	virtual HANDLE getEvent() { return 0; }
#else
	int getFD();
#endif

	virtual bool open();
	virtual bool shutdown();
	void setDescription(char *sDescription);

	virtual void start();

	char * getName() { return m_sName; }
	char * getNameShort() { return m_sNameShort; } 
	char * getDescription() { return m_sDescription; }
	MAC getHwAddr() { return m_hwAddr; }

	virtual bool isTrunk() { return m_bIsTrunk; }
	virtual bool isUp() { return m_bLineState && m_bProtoState; }
	void setTrunk(bool bTrunk) { if (m_bIsTrunk == bTrunk) return; shutdown(); m_bIsTrunk = bTrunk; open(); }
	virtual int getNativeVlan() { return m_swNativeVlan; }
	virtual int getAccessVlan() { return m_swAccessVlan; }

	void setNativeVlan(VlanId vlan) { m_swNativeVlan = vlan; }
	void setAccessVlan(VlanId vlan) { m_swAccessVlan = vlan; }

	bool isCDPEnabled() { return m_bCDPEnabled; }
	void setCDPEnabled(bool bVal) { m_bCDPEnabled = bVal; }
		
	char * getLineStateDescr() {
		if (m_bShutdown)
			return "administratively down";
		if (!m_bLineState)
			return "down";
		return "up";
	}

	char * getProtocolStateDescr() {
		if (!m_bProtoState)
			return "down";
		return "up";
	}

	BYTE *encapsulateDot1q(Frame *pFrame);

	DTPProcess *getDTPProc() { return m_pDTPProc; }
	PortSecurity *getPortSecurityProc() { return m_pPortSecurityProc; }

	void registerProcess(Process *pProc, int nPrio);
	void unregisterProcess(Process *pProc);

	static void registerCommands();
	CMD(cmdInterfaceShutdown) {
		pInterface->shutdown();
	}
	CMD(cmdInterfaceNoShutdown) {
		pInterface->open();
	}
	CMD(cmdInterfaceDescription) {
		pInterface->setDescription(pCommand->arguments.getString(0));
	}
	CMD(cmdInterfaceNoDescription) {
		pInterface->setDescription("");
	}
	CMD(cmdInterfaceExit) {
		pLine->setMode(MODE_CONF);
	}
	CMD(cmdInterfaceSwAccessVlan) {
		pInterface->setAccessVlan(pCommand->arguments.getInt(0));
	}
	CMD(cmdInterfaceSwNativeVlan) {
		pInterface->setNativeVlan(pCommand->arguments.getInt(0));
	}
};


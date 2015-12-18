#pragma once

#include "Line.h"
#include "Core.h"

class Interface;
class InterfaceMgr;

class PortSecurity :
	public Process
{
	typedef enum {
		VIOLATE_SHUTDOWN,
		VIOLATE_RESTRICT,
		VIOLATE_PROTECT,
	} ViolationMode;

	typedef enum {
		AGE_ABSOLUTE,
		AGE_INACTIVITY,
	} AgingMode;

	class Entry {
	public:
		MAC mac;
		time_t tLastAccess;
		bool bStatic;
		bool bSticky;
	};

	bool m_bEnabled;
	Interface *m_pInterface;

	int m_nMaximum;
	ViolationMode m_violationMode;
	AgingMode m_agingMode;
	int m_nAgingTime;
	bool m_bSticky;
	int m_nViolationCount;
	
	typedef map<MAC, Entry*> EntryMap;
	EntryMap m_entries;

	void learnAddr(MAC addr, bool bSticky, bool bStatic);
	void forgetAddr(MAC addr);
	bool hasAddr(MAC addr);

public:
	PortSecurity(Interface *pInterface);
	~PortSecurity(void);

	int getNextEventTimeout();
	bool onTimeout();
	bool onRecv(Frame *pFrame);
	bool onUp(Interface *pInterface);
	bool onDown(Interface *pInterface);

	bool setMaximum(int nMaximum) { 
		if (nMaximum < getCount())
			return false;
		m_nMaximum = nMaximum;
		return true;
	}
	void setViolationMode(ViolationMode mode) {
		m_violationMode = mode;
	}
	void setAgingMode(AgingMode mode) {
		m_agingMode = mode;
	}
	void setAgingTime(int nTimeSec) {
		m_nAgingTime = nTimeSec;
	}
	bool isEnabled() { return m_bEnabled; }

	char *getViolationModeDesc() {
		switch (m_violationMode) {
		case VIOLATE_SHUTDOWN:	return "Shutdown";
		case VIOLATE_PROTECT:	return "Protect";
		case VIOLATE_RESTRICT:	return "Restrict";
		}
		return 0;
	}
	char *getAgingModeDesc() {
		switch (m_agingMode) {
		case AGE_ABSOLUTE:		return "Absolute";
		case AGE_INACTIVITY:	return "Inactivity";
		}
		return 0;
	}
	int getMaximum() { return m_nMaximum; }
	int getViolationCount() { return m_nViolationCount; }
	int getCount() { return m_entries.size(); }

	void start() {
		if (!m_bEnabled) {
			m_pInterface->registerProcess(this, 50);
			g_switch.unregisterProcess(this);
			m_bEnabled = true;
		}
	}
	void stop() {
		if (m_bEnabled) {
			m_pInterface->unregisterProcess(this);
			g_switch.unregisterProcess(this);
			m_bEnabled = false;
		}
		m_entries.clear(); //TODO: free
	}

	static void registerCommands();

	CMD(cmdPortSec) { 
		if (pInterface->getDTPProc()->isDynamic()) {
			pLine->write("\r\nCommand rejected: %s is a dynamic port.", pInterface->getName());
		} else {
			pInterface->getPortSecurityProc()->start();
		}
	}
	CMD(cmdNoPortSec) { pInterface->getPortSecurityProc()->stop(); }
	CMD(cmdPortSecMaximum) { 
		if (!pInterface->getPortSecurityProc()->setMaximum(pCommand->arguments.getInt(0)))
			pLine->write("\r\nMaximum is less than number of currently secured mac-addresses.");
	}
	CMD(cmdNoPortSecMaximum) { 
		if (!pInterface->getPortSecurityProc()->setMaximum(1))
			pLine->write("\r\nMaximum is less than number of currently secured mac-addresses.");
	}
	CMD(cmdPortSecAddress) {
		PortSecurity *pPortSec = pInterface->getPortSecurityProc();

		if (pPortSec->getCount() >= pPortSec->getMaximum()) {
			pLine->write("\r\nTotal secure mac-addresses on interface %s has reached maximum limit.\r\n", pInterface->getName());
			return;
		}

		pPortSec->learnAddr(pCommand->arguments.getMAC(0), false, true);
	}
	CMD(cmdNoPortSecAddress) {
		PortSecurity *pPortSec = pInterface->getPortSecurityProc();
		MAC addr = pCommand->arguments.getMAC(0);

		if (!pPortSec->hasAddr(addr)) { 
			pLine->write("\r\nMac-address %s does not exist.\r\n", MAC2String(addr));
			return;
		}

		pPortSec->forgetAddr(addr);
	}
	CMD(cmdPortSecViolationModeShut) { pInterface->getPortSecurityProc()->setViolationMode(VIOLATE_SHUTDOWN); }
	CMD(cmdPortSecViolationModeRest) { pInterface->getPortSecurityProc()->setViolationMode(VIOLATE_RESTRICT); }
	CMD(cmdPortSecViolationModeProt) { pInterface->getPortSecurityProc()->setViolationMode(VIOLATE_PROTECT); }
	CMD(cmdPortSecAgingTypeAbsolute) { pInterface->getPortSecurityProc()->setAgingMode(AGE_ABSOLUTE); }
	CMD(cmdPortSecAgingTypeInactivity) { pInterface->getPortSecurityProc()->setAgingMode(AGE_INACTIVITY); }
	CMD(cmdPortSecAgingTime) {  pInterface->getPortSecurityProc()->setAgingTime(pCommand->arguments.getInt(0) * 60); }
	CMD(cmdShowPortSec) {
		InterfaceList::iterator it;
		InterfaceList *pInterfaces = g_interfaces.getList();
		PortSecurity *pPortSec;
		Interface *pIf;

		pLine->write("\r\n");
		pLine->write("Secure Port  MaxSecureAddr  CurrentAddr  SecurityViolation  Security Action\r\n");
        pLine->write("                (Count)       (Count)          (Count)\r\n");
		pLine->write("---------------------------------------------------------------------------\r\n");
		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) {
			pIf = *it;
			pPortSec = pIf->getPortSecurityProc();
			if (!pPortSec->isEnabled())
				continue;
			
			pLine->write("% 10s  % 13d  % 11d  % 17d  %s\r\n",
				pIf->getNameShort(),
				pPortSec->getMaximum(),
				pPortSec->getCount(),
				pPortSec->getViolationCount(),
				pPortSec->getViolationModeDesc()
				);
		}
		pLine->write("---------------------------------------------------------------------------\r\n");

	}
	CMD(cmdShowPortSecInterface) {
		int nConfigured = 0;
		int nSticky = 0;
		PortSecurity *pPortSec = pCommand->arguments.getInterface(0)->getPortSecurityProc();
		EntryMap::iterator it;
		EntryMap *pEntries = &pPortSec->m_entries;

		for (it=pEntries->begin(); it!=pEntries->end(); ++it) {
			if (it->second->bStatic)
				nConfigured++;
			if (it->second->bSticky)
				nSticky++;
		}

		pLine->write("\r\n");
		pLine->write("Port Security              : %s\r\n", (pPortSec->m_bEnabled ? "Enabled" : "Disabled"));
		pLine->write("Port Status                : Secure-%sdown\r\n", "??"); //TODO: up, down, shutdown -- pInterface->isUp() ? "up" : "down");
		pLine->write("Violation Mode             : %s\r\n", pPortSec->getViolationModeDesc());
		pLine->write("Aging Time                 : %d mins\r\n", pPortSec->m_nAgingTime/60);
		pLine->write("Aging Type                 : %s\r\n", pPortSec->getAgingModeDesc());
		pLine->write("SecureStatic Address Aging : Disabled\r\n");
		pLine->write("Maximum MAC Addresses      : %d\r\n", pPortSec->m_nMaximum);
		pLine->write("Total MAC Addresses        : %d\r\n", pPortSec->m_entries.size());
		pLine->write("Configured MAC Addresses   : %d\r\n", nConfigured);
		pLine->write("Sticky MAC Addresses       : %d\r\n", nSticky);
		pLine->write("Last Source Address        : 0000.0000.0000\r\n");
		pLine->write("Security Violation Count   : %d\r\n", pPortSec->m_nViolationCount);
	}
};

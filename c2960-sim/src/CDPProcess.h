#pragma once

#include "Line.h"
#include "Core.h"
#include "CDPEntry.h"

class Interface;
class InterfaceMgr;

class CDPProcess :
	public Process
{
protected:
	time_t m_tLastUpdate;

	typedef map<MAC, CDPEntry*> EntryMap;
	EntryMap m_entries;

	static int m_nHoldtime;
	static int m_nTimer;

public:
	CDPProcess(void);
	~CDPProcess(void);

	static int getHoldTime() { return m_nHoldtime; }

	int getNextEventTimeout();
	bool onTimeout();
	bool onRecv(Frame *pFrame);
	bool onUp(Interface *pInterface);

	void addEntry(CDPEntry *pEntry);
	void age();
	EntryMap * getEntryMap() { return &m_entries; }

	void sendUpdates();
	void sendUpdate(Interface *pInterface);

	static void setHoldtime(int nHoldtime) { m_nHoldtime = nHoldtime; g_switch.updateEventList(); }
	static void setTimer(int nTimer) { m_nTimer = nTimer; g_switch.updateEventList(); }

	static void registerCommands();
	CMD(cmdShowCDP) {
		if (!g_pCDP) {
			pLine->write("\r\n%% CDP is not enabled\r\n");
			return;
		}

		pLine->write("\r\n");
		pLine->write("Global CDP information:\r\n");
		pLine->write("        Sending CDP packets every %d seconds\r\n", g_pCDP->m_nTimer);
		pLine->write("        Sending a holdtime value of %d seconds\r\n", g_pCDP->m_nHoldtime);
		pLine->write("        Sending CDPv2 advertisements is  enabled");
	}

	CMD(cmdShowCDPNei) {
		EntryMap::iterator it;
		EntryMap *pMap;
		CDPEntry *pEntry;
		char sCap[13];
		DWORD dwCap;
		int i;

		char *sPlatform;

		if (!g_pCDP) {
			pLine->write("\r\n%% CDP not enabled\n\n");
			return;
		}

		sCap[12] = 0;
		pMap = g_pCDP->getEntryMap();

		pLine->write("\r\nCapability Codes: R - Router, T - Trans Bridge, B - Source Route Bridge\r\n");
		pLine->write("                  S - Switch, H - Host, I - IGMP, r - Repeater, P - Phone\r\n\r\n");
		pLine->write("Device ID        Local Intrfce     Holdtme    Capability  Platform  Port ID\r\n");
		for (it=pMap->begin(); it!=pMap->end(); ++it) {
			pEntry = it->second;

			memset(sCap, ' ', 12);
			i = 0;
			dwCap = pEntry->getVarDWORD(CDP_CAPABILITIES);
			if (dwCap & 0x01)	sCap[i+=2] = 'R';
			if (dwCap & 0x02)	sCap[i+=2] = 'T';
			if (dwCap & 0x04)	sCap[i+=2] = 'B';
			if (dwCap & 0x08)	sCap[i+=2] = 'S';
			if (dwCap & 0x10)	sCap[i+=2] = 'H';
			if (dwCap & 0x20)	sCap[i+=2] = 'I';
			if (dwCap & 0x40)	sCap[i+=2] = 'r';
			if (dwCap & 0x80)	sCap[i+=2] = 'P';

			sPlatform = pEntry->getVarStr(CDP_PLATFORM);
			if (strlen(sPlatform) > 10)
				sPlatform[10] = 0;

			pLine->write("%- 17s%- 17s%- 12d%- 12s%- 10s%s\r\n", 
				pEntry->getVarStr(CDP_DEVICE_ID),
				pEntry->pOrigin->getName(),
				(int)(m_nHoldtime - (g_tNow - pEntry->tLastUpdate)),
				sCap,
				sPlatform,
				pEntry->getVarStr(CDP_PORT_ID)
				);
			//TODO:
		} 
	}
	CMD(cmdShowCDPNeiDetail) {
		EntryMap::iterator it;
		EntryMap *pMap;
		DWORD dwCap;
		char sCapabilities[1024];
		CDPEntry *pEntry;

		if (!g_pCDP) {
			pLine->write("\r\n%% CDP not enabled\n\n");
			return;
		}

		pMap = g_pCDP->getEntryMap();

		pLine->write("\r\n");
		for (it=pMap->begin(); it!=pMap->end(); ++it) {
			pEntry = it->second;

			dwCap = pEntry->getVarDWORD(CDP_CAPABILITIES);
			sprintf(sCapabilities, "%s%s%s%s%s%s%s%s",
				(dwCap & 0x01) ? "Router " : "",
				(dwCap & 0x02) ? "Trans Bridge " : "",
				(dwCap & 0x04) ? "Source Route Bridge " : "",
				(dwCap & 0x08) ? "Switch " : "",
				(dwCap & 0x10) ? "Host " : "",
				(dwCap & 0x20) ? "IGMP " : "",
				(dwCap & 0x40) ? "Repeater " : "",
				(dwCap & 0x80) ? "Phone " : ""
			);

			pLine->write("-------------------------\r\n");
			pLine->write("Device ID: %s\r\n", pEntry->getVarStr(CDP_DEVICE_ID));
			pLine->write("Entry address(es):\r\n");
			pLine->write("Platform: %s,  Capabilities: %s\r\n", pEntry->getVarStr(CDP_PLATFORM), sCapabilities);
			pLine->write("Interface: %s,  Port ID (outgoing port): %s\r\n", pEntry->pOrigin->getName(), pEntry->getVarStr(CDP_PORT_ID));
			pLine->write("Holdtime : %d sec\r\n", pEntry->yHoldTime + (g_tNow - pEntry->tLastUpdate));
			pLine->write("\r\n");
			pLine->write("Version :\r\n%s\r\n", pEntry->getVarStr(CDP_VERSION));
			pLine->write("\r\n");
		/*
			pLine->write("advertisement version: 2\r\n");
			pLine->write("Protocol Hello:  OUI=0x00000C, Protocol ID=0x0112; payload len=27, value=00000000FFFFFFFF010230FF000000000000001AA139F280FF0000\r\n");
			pLine->write("VTP Management Domain: 'd113'\r\n");
			pLine->write("Native VLAN: 1\r\n");
			pLine->write("Duplex: full\r\n");
			pLine->write("Management address(es):\r\n");
			*/
		}
	}
	CMD(cmdCDPRun) {
		if (!g_pCDP) {
			g_pCDP = new CDPProcess();
			g_switch.registerProcess(g_pCDP);
			g_interfaces.registerProcess(g_pCDP, 95);
		}
	}
	CMD(cmdNoCDPRun) {
		if (g_pCDP) {
			g_switch.unregisterProcess(g_pCDP);
			g_interfaces.unregisterProcess(g_pCDP);
			delete g_pCDP;
			g_pCDP = 0;
		}
	}
	CMD(cmdCDPTimer) {
		setTimer(pCommand->arguments.getInt(0));
	}
	CMD(cmdCDPHoldtime) {
		setHoldtime(pCommand->arguments.getInt(0));
	}
	CMD(cmdCDPIfEnable) {
		pInterface->setCDPEnabled(true);
		
		if (!g_pCDP) {
			pLine->write("\r\n%% Cannot enable CDP on this interface, since CDP is not running");
			return;
		} 

		// ak bezi cdp, poslime update
		g_pCDP->sendUpdate(pInterface);
	}
	CMD(cmdCDPIfDisable) {
		pInterface->setCDPEnabled(false);
	}
};


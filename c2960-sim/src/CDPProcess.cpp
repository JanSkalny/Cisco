#include "stdafx.h"

#include "CDPEntry.h"
#include "CDPFrame.h"
#include "Interface.h"
#include "InterfaceMgr.h"
#include "Line.h"
#include "CommandTree.h"

#include "CDPProcess.h"

extern InterfaceMgr g_interfaces;
extern CommandTree g_commands;
extern time_t g_tNow;
extern CDPProcess *g_pCDP;

CDPProcess::CDPProcess(void)
{
	m_tLastUpdate = 0;

	m_nTimer = 60;
	m_nHoldtime = 180;
}

CDPProcess::~CDPProcess(void)
{
}

int CDPProcess::getNextEventTimeout() {
	time_t tSleep;

	tSleep = m_nTimer + (m_tLastUpdate - g_tNow);
	if (tSleep < 0)
		return 0; // now!

	return (int)tSleep;
}

bool CDPProcess::onTimeout() {
	if (g_tNow - m_tLastUpdate > m_nTimer) {
		sendUpdates();
		age();
	}
	return true;
}

bool CDPProcess::onRecv(Frame *pFrame)
{
	CDPFrame *pCDPFrame;

	if (pFrame->wEtherType != ETHERTYPE_CDP) 
		return true;

	printf("* CDP * got packet on %s\n", pFrame->pIfOrigin->getName());

	pCDPFrame = new CDPFrame(*pFrame);
	addEntry(pCDPFrame);

	return false;
}

bool CDPProcess::onUp(Interface *pInterface) {
	// globalny proces
	// poslime update na zobudeny iface
	sendUpdate(pInterface);

	return true;
}

void CDPProcess::sendUpdates() {
	InterfaceList::iterator it;
	InterfaceList *pInterfaces = g_interfaces.getList();
	
	for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it)
		sendUpdate(*it);

	m_tLastUpdate = g_tNow;
}

void CDPProcess::sendUpdate(Interface *pInterface)
{
	CDPFrame frame;

	if (!pInterface->isUp())
		return;

	if (!pInterface->isCDPEnabled())
		return;

	frame.vlan = pInterface->getNativeVlan();
	frame.macSA = pInterface->getHwAddr();
	frame.pIfOrigin = pInterface;

	frame.create(0);

	pInterface->send(&frame);

	printf("* CDP * send packet to %s\n", pInterface->getName());
}

void CDPProcess::addEntry(CDPEntry *pEntry)
{
	//TODO: velke cisco dovoli, mat cdp od tej istej adresy -- rozhoduje aj iface?
	if (m_entries.count(pEntry->macAddr)) {
		delete m_entries[pEntry->macAddr];
		m_entries.erase(pEntry->macAddr);
	}
	m_entries[pEntry->macAddr] = pEntry;
}

void CDPProcess::age() 
{
	EntryMap::iterator it;

	for (it=m_entries.begin(); it!=m_entries.end(); ) {
		if (it->second->tLastUpdate + m_nHoldtime < g_tNow) {
			delete it->second;
			it = m_entries.erase(it);
		} else {
			--it;
		}
	}
}

int CDPProcess::m_nHoldtime = 180;
int CDPProcess::m_nTimer = 60;

void CDPProcess::registerCommands() {
	g_commands.registerCommand(MODE_CONF, 15, "cdp", "Global CDP configuration subcommands");
	g_commands.registerCommand(MODE_CONF, 15, "no cdp", ";Global CDP configuration subcommands");

	g_commands.registerCommand(MODE_CONF, 15, "cdp run", ";Enable CDP", CDPProcess::cmdCDPRun);
	g_commands.registerCommand(MODE_CONF, 15, "no cdp run", ";;Enable CDP", CDPProcess::cmdNoCDPRun);
	g_commands.registerCommand(MODE_CONF, 15, "cdp timer <5-254>", ";Specify the rate at which CDP packets are sent       (in sec);Rate at which CDP packets are sent (in  sec)", CDPProcess::cmdCDPTimer);
	g_commands.registerCommand(MODE_CONF, 15, "cdp holdtime <10-255>", ";Specify the holdtime (in sec) to be sent in packets;Length  of time  (in sec) that receiver must keep this packet", CDPProcess::cmdCDPTimer);

	g_commands.registerCommand(MODE_CONF_IF, 15, "cdp enable", "CDP Interface subcommands;Enable CDP on interface", CDPProcess::cmdCDPIfEnable);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no cdp enable", ";CDP Interface subcommands;Enable CDP on interface", CDPProcess::cmdCDPIfDisable);

	g_commands.registerCommand(MODE_EXEC, 1, "show cdp", ";CDP information", CDPProcess::cmdShowCDP);
	g_commands.registerCommand(MODE_EXEC, 1, "show cdp neighbor", ";;CDP neighbor entries", CDPProcess::cmdShowCDPNei);
	g_commands.registerCommand(MODE_EXEC, 1, "show cdp neighbor detail", ";;;Show detailed information", CDPProcess::cmdShowCDPNeiDetail);
}

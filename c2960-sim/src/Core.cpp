#include "stdafx.h"

#include "Frame.h"
#include "Command.h"
#include "Line.h"
#include "Interface.h"
#include "InterfaceMgr.h"
#include "CommandTree.h"
#include "svn.h"
#include "Core.h"
#include "MacAddressTable.h"
#include "Vlan.h"
#include "VlanMgr.h"

Core::Core(void)
{
	m_bUpdateEventList = true;
	m_hUpdateEvent = CreateEvent(0, 0, 0, 0);
}


Core::~Core(void)
{
}

void Core::work()
{
	int nRecvd = 0;

	while (1) {
		// pockame, kym sa nieco nestane, alebo kym sa nieco stat nema
		// ale len ak sme v predoslom kole nedostali ziadny ramec
		if (nRecvd == 0)
			waitForEvents();

		time(&g_tNow);

		// spracujeme commandy, co sa maju vykonat
		execCommands();

		// obsluzime procesy (cdp, vtp, keepalive)
		execProcesses();

		// obsluzime rozhrania
		nRecvd = pollInterfaces();
	}
}

void Core::execCommands() {
	//TODO:
}

void Core::execProcesses() {
	ProcessList::iterator it;

	for (it=m_proc.begin(); it!=m_proc.end(); ++it) {
		// kazdy kto je na 0 -- exec!
		if ((*it)->m_pProc->getNextEventTimeout() == 0)
			(*it)->m_pProc->onTimeout();
	}
}

int Core::pollInterfaces() {
	InterfaceList *pInterfaces;
	InterfaceList::iterator it;
	Interface *pInterface;
	Frame *pFrame;

	int nRecvd = 0;

	pInterfaces = g_interfaces.getList();

	// kazde rozhranie s eventom 
	for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) {
		pInterface = *it;
		if (!(pFrame = pInterface->recv()))
			continue;

		nRecvd++;
		
#if DEBUG_PACKET
		printf("got frame from %s - da=%s len=%d ",
			pInterface->getName(),
			MAC2String(pFrame->macDA),
			pFrame->getLen());
#endif

		// overime integritu / extra parsovanie
		if (validateFrame(pFrame)) {
#ifdef DEBUG_PACKET
			printf("vlan=%d eth=%04x\n", pFrame->vlan, pFrame->wEtherType);
#endif

			// naucime sa z ramcu do CAM tabulky
			g_macAddressTable.learn(pFrame->macSA, pInterface, pFrame->vlan);

			// pokusime sa ho zjest, ak nie, posunieme susedom
			if (!processFrame(pFrame)) 
				forwardFrame(pFrame);
		} else {
#ifdef DEBUG_PACKET
			printf("-- drop\n");
#endif
		}
		
		delete pFrame;
	}	

	return nRecvd;
}

bool Core::validateFrame(Frame *pFrame) {
	Interface *pInterface = pFrame->pIfOrigin;
	
	if (pInterface->isTrunk()) {
		if (!pFrame->bTagged) {
			// ak dojde netagovany => native
			pFrame->vlan = pInterface->getNativeVlan();
		} else if (pFrame->vlan == pInterface->getNativeVlan()) {
			// ak dojde tagovany ako nativna vlana => drop
			return false;
		}
	} else {
		// tagovane ramce zahadzujeme
		if (pFrame->bTagged) 
			return false;
		// netagovane zaradime do access vlany
		pFrame->vlan = pInterface->getAccessVlan();
	}

	return true;
}

bool Core::processFrame(Frame *pFrame) {

	//TODO: ak je pre nas alebo bcast na 1 hop, zozerieme, ianc false
	if (pFrame->wEtherType == ETHERTYPE_DTP)
		return true;
	if (pFrame->wEtherType == ETHERTYPE_VTP)
		return true;
	if (pFrame->wEtherType == ETHERTYPE_CDP)
		return true;
	
	//XXX: toto by sme mohli prerobit na event procesu

	return false;
}

bool Core::forwardFrame(Frame *pFrame)
{
	InterfaceList *pInterfaces;
	InterfaceList::iterator it;
	Interface *pInterface;

	pInterfaces = g_interfaces.getList();

	// pre neznamu vlanu zahadzujeme
	if (!g_vlans.has(pFrame->vlan))
		return false;

	// skonzultujeme CAM tabulku
	pInterface = g_macAddressTable.lookup(pFrame->macDA, pFrame->vlan);

	if (pInterface) {
		// jednym rozhranim
		sendFrame(pInterface,pFrame);
	} else {
		// vsade!
		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) {
			pInterface = *it;
			sendFrame(pInterface,pFrame);
		}
	}

	return true;
}

bool Core::sendFrame(Interface *pInterface, Frame *pFrame)
{
	// neposielame na port, kde sme ramec prijali
	if (pInterface == pFrame->pIfOrigin)
		return false;

	// neposleme na accessport, ak to ide do konkretnej vlany
	// a access port nematri do danej vlany
	if ((pFrame->vlan != VLAN_ALL) &&
		(!pInterface->isTrunk()) && 
		(pInterface->getAccessVlan() != pFrame->vlan))
		return false;

	pInterface->send(pFrame);
	return true;
}

void Core::updateEventList()
{
	m_bUpdateEventList = true; 
	SetEvent(m_hUpdateEvent);
}

void Core::waitForEvents()
{
	int nTimeout;
	DWORD dwRet;

	if (m_bUpdateEventList)
		doUpdateEventList();

	nTimeout = getNextEventTimeout();

	// nemame na co cakat!
	if (!nTimeout)
		return;

	if (!m_nEventCount) {
		Sleep(nTimeout*1000);
	} else {
		DWORD dwTickStart = GetTickCount();	
		dwRet = WaitForMultipleObjects(m_nEventCount, m_hEvents, 0, nTimeout*1000);
		if (dwRet == -1) {
			printf("wait failed, updating event list\n");
			updateEventList();
		} 
		char sBuf[1024];
		sprintf(sBuf, "select(timeout=%d): slept for %d ms (ret=%d)\n", nTimeout, GetTickCount() - dwTickStart, dwRet);
		OutputDebugString(sBuf);
	}
}

int Core::getNextEventTimeout() {
	ProcessList::iterator it;
	int nShortest=1000, nTimeout;

	for (it=m_proc.begin(); it!=m_proc.end(); ++it) {
		nTimeout = (*it)->m_pProc->getNextEventTimeout();
		if (nTimeout < 0)
			continue;
		if (nTimeout < nShortest)
			nShortest = nTimeout;
	}

	return nShortest;
}

void Core::doUpdateEventList() {
	InterfaceList *pInterfaces;
	InterfaceList::iterator it;
	Interface *pInterface;

	printf("[CORE] updating event list\n");

	m_bUpdateEventList = false;
	m_nEventCount = 0;
	pInterfaces = g_interfaces.getList();

	// event na aktualizovanie zoznamu eventov :) -- inception
	m_hEvents[m_nEventCount++] = m_hUpdateEvent;

	// kazde rozhranie s eventom 
	for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) {
		pInterface = *it;
		if (pInterface->getEvent())
			m_hEvents[m_nEventCount++] = pInterface->getEvent();
	}
}

void Core::registerProcess(Process *pProc)
{
	m_proc.push(pProc, 0);
	updateEventList();
}

void Core::unregisterProcess(Process *pProc)
{
	if (m_proc.remove(pProc))
		updateEventList();
}

void Core::registerCommands()
{
	g_commands.registerCommand(MODE_EXEC, 0, "show", "Show running system information");

	g_commands.registerCommand(MODE_EXEC, 0, "show version", ";System hardware and software status", Core::cmdShowVersion);

	g_commands.registerCommand(MODE_EXEC, 15, "configure terminal", "Enter configuration mode;Configure from the terminal", Core::cmdConfigureTerminal);
	g_commands.registerCommand(MODE_CONF, 0, "exit", "Exit from the EXEC", Core::cmdExitConfigure);

	g_commands.registerCommand(MODE_EXEC, 0, "enable", "Turn on privileged commands", Core::cmdEnable);
	g_commands.registerCommand(MODE_EXEC, 0, "disable", "Turn off privileged commands", Core::cmdDisable);
}

 CMDI(Core::cmdShowVersion) {
	pLine->write("\r\nCisco IOS C2960 Software (C2960-LANBASEK9-M), Version 15.0(1)SE, RELEASE SOFTWARE (fc1)\r\n\r\n");
	pLine->write("Simulator rev. %d%s\r\n", SVN_REVISION, SVN_LOCAL_MODS ? "M" : "");
}
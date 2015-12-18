#include "stdafx.h"

#include "Command.h"
#include "Line.h"
#include "LineTelnet.h"
#include "Interface.h"
#include "CommandTree.h"
#include "InterfaceMgr.h"
#include "Frame.h"
#include "Core.h"
#include "DTPProcess.h"
#include "PortSecurity.h"

extern CommandTree g_commands;
extern InterfaceMgr g_interfaces;
extern Core g_switch;

Interface::Interface(const char *sName, MAC addr)
{
	char *sPos;
	
	memset(m_sDescription, 0, sizeof(m_sDescription));

	strncpy(m_sName, sName, MAX_NAME_LEN-1);

	memset(m_sNameShort, 0, MAX_NAME_LEN);
	memcpy(m_sNameShort, sName, 2);
	sPos = strpbrk(m_sName, "1234567890/");
	if (sPos) 
		memcpy(m_sNameShort+2, sPos, strlen(sPos));
	
	// administratively down / down
	m_bShutdown = true;
	m_bLineState = false;
	m_bProtoState = false;

	m_hwAddr = addr;

	m_bIsTrunk = false;
	m_swAccessVlan = 1;
	m_swNativeVlan = 1;

	m_pDTPProc = new DTPProcess(this);
	m_pPortSecurityProc = new PortSecurity(this);

	//TODO: zapnut dtp
}


Interface::~Interface(void)
{
	if (!m_bShutdown)
		shutdown();
}

void Interface::start() {
	if (m_pDTPProc)
		m_pDTPProc->start();
}

bool Interface::open()
{
	if (!m_bShutdown)
		return false;

	m_bShutdown = false;

	return true;
}

bool Interface::shutdown()
{
	if (m_bShutdown)
		return false;

	m_bShutdown = true;
	m_bLineState = false;
	m_bProtoState = false;

	onDown();

	return true;
}

void Interface::setDescription(char *sDescription)
{
	strncpy(m_sDescription, sDescription, MAX_DESC_LEN-1);
}

BYTE * Interface::encapsulateDot1q(Frame *pFrame) {
	static BYTE pClone[Frame::MAX_LEN + 4];
	BYTE *pData = pFrame->getData();

	// da + sa
	memcpy(pClone, pData, 12);

	// VLAN tag
	pClone[12] = 0x81;
	pClone[13] = 0x00;
	*((WORD*)&(pClone[14])) = htons(pFrame->vlan);

	// ethertype a zvysok
	memcpy(pClone+16, pData+12, pFrame->getLen() - 12);

	return pClone;
}

void Interface::registerProcess(Process *pProc, int nPrio) 
{
	m_processes.push(pProc, nPrio);
}

void Interface::unregisterProcess(Process *pProc) 
{
	m_processes.remove(pProc);
}

Frame *Interface::onRecv(Frame *pFrame) {
	bool bPass = true;
	ProcessList::iterator it;

	for (it=m_processes.begin(); it!=m_processes.end(); ++it) 
		bPass &= (*it)->m_pProc->onRecv(pFrame);

	if (!bPass) {
		delete pFrame;
		pFrame = 0;
	}

	return pFrame;
}

void Interface::onUp() {
	ProcessList::iterator it;

	g_switch.updateEventList();
	g_pCon0->syslog("%%LINEPROTO-5-UPDOWN: Line protocol on Interface %s, changed state to up", m_sName);
	
	for (it=m_processes.begin(); it!=m_processes.end(); ++it) 
		(*it)->m_pProc->onUp(this);
}

void Interface::onDown() {
	ProcessList::iterator it;

	g_switch.updateEventList();
	g_pCon0->syslog("%%LINEPROTO-5-UPDOWN: Line protocol on Interface %s, changed state to down", m_sName);
	
	for (it=m_processes.begin(); it!=m_processes.end(); ++it) 
		(*it)->m_pProc->onDown(this);
}

bool Interface::onSend(Frame *pFrame) {
	bool bPass = true;
	ProcessList::iterator it;

	for (it=m_processes.begin(); it!=m_processes.end(); ++it) 
		bPass &= (*it)->m_pProc->onSend(pFrame);

	return bPass;
}

void Interface::registerCommands() {
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport", "Set switching mode characteristics");
	
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport access vlan <1-4094>", ";Set access mode characteristics of the interface;Set VLAN when interface is in access mode;VLAN ID of the VLAN when this port is in access mode", Interface::cmdInterfaceSwAccessVlan);
	g_commands.registerCommand(MODE_CONF_IF, 15, 
		"switchport trunk native vlan <1-4094>", 
		";"\
		"Set trunking characteristics of the interface;"\
		"Set trunking native characteristics when interface is in trunking mode;"\
		"Set native VLAN when interface is in trunking mode;VLAN ID of the native VLAN when this port is in trunking mode", 
		Interface::cmdInterfaceSwNativeVlan);

	
	g_commands.registerCommand(MODE_CONF_IF, 15, "shutdown", "Shutdown the selected interface", Interface::cmdInterfaceShutdown);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no shutdown", ";Shutdown the selected interface", Interface::cmdInterfaceNoShutdown);
	g_commands.registerCommand(MODE_CONF_IF, 15, "description LINE", "Interface specific description;<cr>", Interface::cmdInterfaceDescription);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no description", ";Interface specific description", Interface::cmdInterfaceNoDescription);
	g_commands.registerCommand(MODE_CONF_IF, 1, "exit", "Exit from interface configuration mode", Interface::cmdInterfaceExit);
}

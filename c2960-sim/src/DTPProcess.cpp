#include "stdafx.h"

#include "Interface.h"
#include "Frame.h"
#include "DTPFrame.h"
#include "Line.h"
#include "CommandTree.h"

#include "DTPProcess.h"

extern InterfaceMgr g_interfaces;
extern CommandTree g_commands;
extern time_t g_tNow;
extern CDPProcess *g_pCDP;

DTPProcess::DTPProcess(Interface *pInterface)
	: Process()
{
	m_tLastEvent = 0;
	m_pInterface = pInterface;
	m_bEnable = false;
	m_mode = IF_MODE_DYNAMIC_AUTO;
}


DTPProcess::~DTPProcess(void)
{
}

int DTPProcess::getNextEventTimeout() 
{
	time_t tSleep;

	tSleep = 30 + (m_tLastEvent - g_tNow);
	if (tSleep < 0)
		return 0; // now!

	return (int)tSleep;
}

bool DTPProcess::onTimeout() 
{
	if (g_tNow - m_tLastEvent > 30) 
		sendUpdate();
	return true;
}

bool DTPProcess::onRecv(Frame *pFrame) 
{
	DTPFrame *pDTPFrame;
	bool bTrunk = false, bNotify = false;

	if (pFrame->wEtherType != ETHERTYPE_DTP) 
		return true;

	//TODO: spracujeme frame ako DTP
	printf("* DTP * got packet on %s\n", m_pInterface->getName());

	pDTPFrame = new DTPFrame(*pFrame);

	switch (m_mode) {
	case IF_MODE_ACCESS:
		break;

	case IF_MODE_DYNAMIC_DESIRABLE:
		switch (pDTPFrame->yTas) {
		case TAS_AUTO:
		case TAS_ON:
		case TAS_DESIRABLE:
			bTrunk = true;
		}
		break;

	case IF_MODE_DYNAMIC_AUTO:
		switch (pDTPFrame->yTas) {
		case TAS_ON:
		case TAS_DESIRABLE:
			bTrunk = true;
		}
		break;

	case IF_MODE_TRUNK:
		bTrunk = true;
		break;
	}

	if (m_pInterface->isTrunk() != bTrunk) 
		bNotify = true;

	if (bNotify)
		printf("%s: change to %s\n", m_pInterface->getName(), bTrunk ? "TRUNK" : "NOT trunk");

	m_pInterface->setTrunk(bTrunk);

	if (bNotify)
		sendUpdate();

	return false;
}

bool DTPProcess::onUp(Interface *pInterface) 
{
	if (m_bEnable)
		sendUpdate();

	return true;
}

void DTPProcess::sendUpdate() {
	DTPFrame frame;

	if (!m_pInterface->isUp())
		m_tLastEvent = g_tNow - 25; // o 5 sekund zobudme
	m_tLastEvent = g_tNow;

	frame.vlan = m_pInterface->getNativeVlan();
	frame.macSA = m_pInterface->getHwAddr();

	frame.sVTPDomain = 0;
	switch (m_mode) {
	case IF_MODE_TRUNK:				frame.yTas = TAS_ON;		break;
	case IF_MODE_ACCESS:			frame.yTas = TAS_OFF;		break;
	case IF_MODE_DYNAMIC_DESIRABLE:	frame.yTas = TAS_DESIRABLE;	break;
	case IF_MODE_DYNAMIC_AUTO:		frame.yTas = TAS_AUTO;		break;
	}
	frame.yTos = m_pInterface->isTrunk() ? TOS_TRUNK : TOS_ACCESS;
	frame.yTat = TAT_1Q;
	frame.yTot = TOT_1Q;
	frame.macNeighbor = m_pInterface->getHwAddr();

	frame.create(0);

	printf("* DTP * send packet on %s\n", m_pInterface->getName());
	m_pInterface->send(&frame);
}

void DTPProcess::setMode(IfMode mode) { 
	bool bNotify = false;

	// ak sa zmeni mode, a mame DTP, poslime update
	if (m_mode != mode) 
		bNotify = true;

	m_mode = mode;

	if (mode == IF_MODE_ACCESS) 
		m_pInterface->setTrunk(false);
	if (mode == IF_MODE_TRUNK) 
		m_pInterface->setTrunk(true);

	if (bNotify && m_bEnable) 
		sendUpdate();
}

char* DTPProcess::getModeDesc()
{
	switch (m_mode) {
	case IF_MODE_ACCESS:			return "static access";
	case IF_MODE_DYNAMIC_AUTO:		return "dynamic auto";
	case IF_MODE_DYNAMIC_DESIRABLE:	return "dynamic desirable";
	case IF_MODE_TRUNK:				return "static trunk";						
	}
	return "";
}

void DTPProcess::start() {
	if (!m_bEnable) {
		m_pInterface->registerProcess(this, 100);
		g_switch.unregisterProcess(this);
		m_bEnable = true;

		if (m_mode == IF_MODE_ACCESS) 
			m_pInterface->setTrunk(false);
		if (m_mode == IF_MODE_TRUNK) 
			m_pInterface->setTrunk(true);
		sendUpdate();
	}
}

void DTPProcess::stop() {
	if (m_bEnable) {
		m_pInterface->unregisterProcess(this);
		g_switch.unregisterProcess(this);
		m_bEnable = false;
	}
}

void DTPProcess::registerCommands() {
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport mode", "Set switching mode characteristics;Set trunking mode of the interface");

	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport mode access", ";;Set trunking mode to ACCESS unconditionally", DTPProcess::cmdInterfaceSwModeAccess);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport mode trunk", ";;Set trunking mode to TRUNK unconditionally", DTPProcess::cmdInterfaceSwModeTrunk);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport mode dynamic auto", ";;Set trunking mode to dynamically negotiate access or trunk mode;Set trunking mode dynamic negotiation parameter to AUTO", DTPProcess::cmdInterfaceSwModeDynamicAuto);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport mode dynamic desirable", ";;;Set trunking mode dynamic negotiation parameter to DESIRABLE", DTPProcess::cmdInterfaceSwModeDynamicDesirable);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport mode","Negate a command or set its defaults;Set switching mode characteristics;Set trunking mode dynamic negotiation parameter to AUTO;",DTPProcess::cmdInterfaceSwNoMode);

	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport nonegotiate", ";Device will not engage in negotiation protocol on this interface", DTPProcess::cmdInterfaceSwNonegotiate);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport nonegotiate", ";;Device will not engage in negotiation protocol on this interface", DTPProcess::cmdInterfaceSwNoNonegotiate);
}

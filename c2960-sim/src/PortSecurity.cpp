#include "stdafx.h"

#include "Interface.h"
#include "InterfaceMgr.h"
#include "Line.h"
#include "LineTelnet.h"
#include "CommandTree.h"
#include "Frame.h"
#include "DTPProcess.h"

#include "PortSecurity.h"


PortSecurity::PortSecurity(Interface *pInterface)
{
	m_pInterface = pInterface;
	m_bEnabled = false;

	m_bSticky = false;
	m_violationMode = VIOLATE_SHUTDOWN;
	m_agingMode = AGE_ABSOLUTE;
	m_nAgingTime = 0;
	m_nMaximum = 1;
	m_nViolationCount = 0;
}


PortSecurity::~PortSecurity(void)
{
}

int PortSecurity::getNextEventTimeout() 
{
	//TODO: aging
	return -1;
}

bool PortSecurity::onTimeout() 
{
	return true;
}

bool PortSecurity::onRecv(Frame *pFrame)
{
	MAC addr = pFrame->macSA;

	if (m_entries.count(addr)) {
		// pozname
		if (m_agingMode == AGE_INACTIVITY)
			m_entries[addr]->tLastAccess = g_tNow;
	} else {
		// novy zaznam
		if ((int)(m_entries.size() + 1) > m_nMaximum) {
			// violate
			switch (m_violationMode) {
			case VIOLATE_RESTRICT:
				// TODO:syslog 
				g_pCon0->syslog("PORT_SECURITY-2-PSECURE_VIOLATION: Security violation occurred, caused by MAC address %s on port %s", 
					MAC2String(addr),
					m_pInterface->getName());
				m_nViolationCount++;
			case VIOLATE_PROTECT:
				return false;
				break;
			case VIOLATE_SHUTDOWN:
				m_nViolationCount++;
				m_pInterface->shutdown();
				break;
			}
		} else {
			learnAddr(addr, m_bSticky, false);
		}
	}

	return true;
}

void PortSecurity::learnAddr(MAC addr, bool bSticky, bool bStatic) 
{
	Entry * pEntry = new Entry();
	pEntry->mac = addr;
	pEntry->bSticky = bSticky;
	pEntry->bStatic = bStatic;
	pEntry->tLastAccess = g_tNow;
	m_entries[addr] = pEntry;
}

void PortSecurity::forgetAddr(MAC addr)
{
	if (m_entries.count(addr)) {
		delete m_entries[addr];
		m_entries.erase(addr);
	}
}

bool PortSecurity::hasAddr(MAC addr)
{
	return (m_entries.count(addr) > 0) ? true : false;
}

bool PortSecurity::onUp(Interface *pInterface) {
	// security je PER-INTERFACE process, takze mozeme clear len tak
	m_entries.clear();
	return true;
}

bool PortSecurity::onDown(Interface *pInterface) {
	// security je PER-INTERFACE process, takze mozeme clear len tak
	m_entries.clear();
	return true;
}

void PortSecurity::registerCommands() {
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security", ";Security related command", PortSecurity::cmdPortSec);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport port-security", ";;Security related command", PortSecurity::cmdNoPortSec);

	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security maximum <1-132>", ";;Max secure addresses;Maximum addresses", PortSecurity::cmdPortSecMaximum);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport port-security maximum", ";;;Max secure addresses", PortSecurity::cmdNoPortSecMaximum);

	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security mac-address H.H.H", ";;Secure mac address;48 bit mac address", PortSecurity::cmdPortSecAddress);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport port-security mac-address H.H.H", ";;;Secure mac address;48 bit mac address", PortSecurity::cmdNoPortSecAddress);

	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security violation", ";;Security violation mode");
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security violation shutdown", ";;;Security violation shutdown mode", PortSecurity::cmdPortSecViolationModeShut);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security violation restrict", ";;;Security violation restrict mode", PortSecurity::cmdPortSecViolationModeRest);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security violation protect", ";;;Security violation protect mode", PortSecurity::cmdPortSecViolationModeProt);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport port-security violation", ";;;Security violation mode", PortSecurity::cmdPortSecViolationModeShut);

	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security aging", ";;Port-security aging commands");
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security aging time <1-1440>", ";;;Port-security aging time;Aging time in minutes. Enter a value between 1 and 1440", PortSecurity::cmdPortSecAgingTime);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security aging type", ";;;Port-security aging type");
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security aging type absolute", ";;;;Absolute aging (default)", PortSecurity::cmdPortSecAgingTypeAbsolute);
	g_commands.registerCommand(MODE_CONF_IF, 15, "switchport port-security aging type inactivity", ";;;;Aging based on inactivity time period", PortSecurity::cmdPortSecAgingTypeInactivity);
	g_commands.registerCommand(MODE_CONF_IF, 15, "no switchport port-security aging type", ";;;;Port-security aging type", PortSecurity::cmdPortSecAgingTypeAbsolute);

	g_commands.registerCommand(MODE_EXEC, 2, "show port-security", ";Show secure port information", PortSecurity::cmdShowPortSec);
	g_commands.registerCommand(MODE_EXEC, 2, "show port-security interface FastEthernet", ";;Show secure interface;FastEthernet IEEE 802.3", PortSecurity::cmdShowPortSecInterface);
}
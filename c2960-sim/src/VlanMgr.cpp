#include "stdafx.h"


#include "Command.h"
#include "Line.h"
#include "Interface.h"
#include "InterfaceMgr.h"
#include "CommandTree.h"

#include "VlanMgr.h"
#include "VTPProcess.h"

VlanMgr::VlanMgr(void)
{
	Vlan *pVlan;

	pVlan = new Vlan(1, "default", true);
	m_vlans[1] = pVlan;
	pVlan->commit();

	pVlan = new Vlan(1002, "fddi-default", true);
	pVlan->yType = Vlan::VLAN_TYPE_FDDI;
	m_vlans[1002] = pVlan;
	pVlan->commit();

	pVlan = new Vlan(1003, "token-ring-default", true);
	pVlan->yType = Vlan::VLAN_TYPE_TrCRF;
	m_vlans[1003] = pVlan;
	pVlan->commit();

	pVlan = new Vlan(1004, "fddinet-default", true);
	pVlan->yType = Vlan::VLAN_TYPE_FDDI_NET;
	m_vlans[1004] = pVlan;
	pVlan->commit();

	pVlan = new Vlan(1005, "trnet-default", true);
	pVlan->yType = Vlan::VLAN_TYPE_TrBRF;
	m_vlans[1005] = pVlan;
	pVlan->commit();
}

VlanMgr::~VlanMgr(void)
{
}

void VlanMgr::remove(VlanId id) {
	if (!has(id))
		return;
	delete(m_vlans[id]);
	m_vlans.erase(id);
}

Vlan *VlanMgr::get(VlanId id)
{
	Vlan *pVlan;

	if (!m_vlans.count(id)) {
		pVlan = new Vlan(id);
		m_vlans[id] = pVlan;

		return pVlan;
	}

	return m_vlans[id];
}

void VlanMgr::commit() 
{
	VlanMap::iterator it;
	bool bNotify = false;

	for (it=m_vlans.begin(); it!=m_vlans.end(); ++it) {
		bNotify |= !(it->second->isCommited());
		it->second->commit();
	}

	if (bNotify)
		g_vtp.onVlanModified();
}

void VlanMgr::empty() {
	VlanMap::iterator it;
	Vlan *pVlan;

	for (it=m_vlans.begin(); it!=m_vlans.end(); ) {
		pVlan = it->second;
		if (pVlan->isReserved()) {
			++it;
			continue;
		}
		delete pVlan;
		it = m_vlans.erase(it);
	}
}

void VlanMgr::replace(VlanMap *pVlans) 
{
	VlanMap::iterator it;

	empty();

	for (it=pVlans->begin(); it!=pVlans->end(); ++it)
		m_vlans[it->first] = it->second;
}

void VlanMgr::registerCommands()
{
	g_commands.registerCommand(MODE_EXEC, 1, "show vlan", ";VTP VLAN status", VlanMgr::cmdShowVlan);
	g_commands.registerCommand(MODE_EXEC, 1, "show vlan brief", ";;VTP all VLAN status in brief", VlanMgr::cmdShowVlan);

	g_commands.registerCommand(MODE_CONF, 15, "vlan <1-4096>", "Vlan commands;ISL VLAN IDs 1-4094", VlanMgr::cmdConfigVlan);
	g_commands.registerCommand(MODE_CONF, 15, "no vlan <1-4096>", ";Vlan commands;ISL VLAN IDs 1-4094", VlanMgr::cmdConfigNoVlan);

	g_commands.registerCommand(MODE_CONF_VLAN, 0, "no", "Negate a command or set its defaults");
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "name WORD", "Ascii name of the VLAN;The ascii name for the VLAN", VlanMgr::cmdVlanName);
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "no name", ";Ascii name of the VLAN", VlanMgr::cmdVlanNoName);
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "shutdown", "Shutdown VLAN switching", VlanMgr::cmdVlanShutdown);
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "no shutdow", ";Shutdown VLAN switching", VlanMgr::cmdVlanNoShutdown);
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "state active", "Operational state of the VLAN;VLAN Active State", VlanMgr::cmdVlanStateActive);
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "state suspended", "Operational state of the VLAN;VLAN Suspended State", VlanMgr::cmdVlanStateSuspend);
	g_commands.registerCommand(MODE_CONF_VLAN, 15, "no state", ";Operational state of the VLAN", VlanMgr::cmdVlanStateActive);
	g_commands.registerCommand(MODE_CONF_VLAN, 0, "exit", "Apply changes, bump revision number, and exit mode", VlanMgr::cmdVlanExit);
}	
	

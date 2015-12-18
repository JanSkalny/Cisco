#include "stdafx.h"

#include "Interface.h"
#include "Command.h"
#include "Line.h"
#include "CommandTree.h"
#include "Core.h"

#include "MacAddressTable.h"

MacAddressTable::MacAddressTable(void)
{
}


MacAddressTable::~MacAddressTable(void)
{
}


void MacAddressTable::learn(MAC addr, Interface *pInterface, VlanId vlan, bool bStatic)
{
	Entry *pEntry;

	// broadcast sa neucime!
	if (IS_BROADCAST_MAC(addr))
		return;

	// ak uz danu adresu pozname, vytiahnime si jej zaznam
	if (m_table.count(addr)) {
		pEntry = m_table[addr];
	} else {
		// nova adresa => novy zaznam
		//TODO: kontrolovat limit adries
		pEntry = new Entry();
		m_table[addr] = pEntry;
		pEntry->addr = addr;
	}

	// zastavime age
	pEntry->tLastActive = g_tNow;

	// ak nieje staticka, upravime ostatne parametre
	if (!pEntry->bStatic) {
		pEntry->pInterface = pInterface;
		pEntry->vlan = vlan;
		pEntry->bStatic = bStatic;
	}
}

Interface *MacAddressTable::lookup(MAC addr, VlanId vlan)
{
	Entry *pEntry;

	// broadcast a multicast ide vsade
	if (IS_BROADCAST_MAC(addr))
		return 0;
	
	// neznama tiez vsade
	if (!m_table.count(addr))
		return 0;

	// ak je v inej vlane, ne-e (broadcast)
	pEntry = m_table[addr];
	if (pEntry->vlan != vlan)
		return 0;

	return pEntry->pInterface;
}

bool MacAddressTable::removeStatic(MAC addr)
{
	Entry *pEntry;

	if (!m_table.count(addr))
		return false;

	pEntry = m_table[addr];
	if (!pEntry->bStatic)
		return false;

	m_table.erase(addr);
	delete pEntry;

	return true;
}

void MacAddressTable::registerCommands()
{
	g_commands.registerCommand(MODE_EXEC, 1, "show mac address-table", ";MAC configuration;MAC forwarding table", MacAddressTable::cmdShowMacAddressTable);
	g_commands.registerCommand(MODE_CONF, 15, "mac address-table static H.H.H vlan <1-1005> interface FastEthernet", "Global MAC configuration subcommands;Configure the MAC address table;static keyword;48 bit mac address;VLAN keyword;VLAN id of mac address table;interface;FastEthernet IEEE 802.3", MacAddressTable::cmdMacAddressTableStatic);
	g_commands.registerCommand(MODE_CONF, 15, "mac address-table static H.H.H vlan <1-1005>", ";;;;;;", MacAddressTable::cmdMacAddressTableStatic);
	g_commands.registerCommand(MODE_CONF, 15, "no mac address-table static H.H.H", ";Global MAC configuration subcommands;Configure the MAC address table;static keyword;48 bit mac address", MacAddressTable::cmdMacAddressTableNoStatic);
}
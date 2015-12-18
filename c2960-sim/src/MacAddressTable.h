#pragma once

class Interface;

class MacAddressTable // : public Process 
{
protected:
	class Entry {
	public:
		Entry() { pInterface=0; vlan=0; bStatic = false; }
		MAC addr;
		Interface *pInterface;
		VlanId vlan;
		bool bStatic;
		time_t tLastActive;
	};

	typedef map<MAC,MacAddressTable::Entry*> MACMap;

	MACMap m_table;

public:
	MacAddressTable(void);
	~MacAddressTable(void);

	MACMap *getTable() { return &m_table; }

	void learn(MAC addr, Interface *pInterface, VlanId vlan, bool bStatic=false);
	Interface *lookup(MAC addr, VlanId vlan);
	bool removeStatic(MAC addr);

	static void registerCommands();
	CMD(cmdShowMacAddressTable) {
		MACMap *pTable = g_macAddressTable.getTable();
		MACMap::iterator it;
		MacAddressTable::Entry *pEntry;

		pLine->write("\r\n");
		pLine->write("Destination Address  Address Type  VLAN  Destination Port\r\n");
		pLine->write("-------------------  ------------  ----  --------------------\r\n");
		for (it=pTable->begin(); it!=pTable->end(); ++it) {
			pEntry = it->second;
			pLine->write("%- 21s", MAC2String(pEntry->addr));
			if (pEntry->bStatic) 
				pLine->write("static        ");
			else 
				pLine->write("dynamic       ");
			if (pEntry->vlan == VLAN_ALL)
				pLine->write("All   ");
			else 
				pLine->write("%- 6d%s", pEntry->vlan,"");
			if (pEntry->pInterface)
				pLine->write(pEntry->pInterface->getName());
			pLine->write("\r\n");
		}
		pLine->write("\r\n");
	}
	CMD(cmdMacAddressTableNoStatic) {
		g_macAddressTable.removeStatic(pCommand->arguments.getMAC(0));
	}
	CMD(cmdMacAddressTableStatic) {
		MAC addr;
		VlanId vlan;
		Interface *pIf=0;

		addr = pCommand->arguments.getMAC(0);
		vlan = pCommand->arguments.getInt(1);
		if (pCommand->arguments.hasInterface(2))
			pIf = pCommand->arguments.getInterface(2);
	
		g_macAddressTable.learn(addr, pIf, vlan, true);
	}
	CMD(cmdMacAddressTableAging);
	CMD(cmdMacAddressTableAgingVlan);
};


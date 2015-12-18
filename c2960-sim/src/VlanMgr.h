#pragma once

class VlanMgr
{
	VlanMap m_vlans;

public:
	VlanMgr(void);
	~VlanMgr(void);

	bool has(VlanId id) { return m_vlans.count(id) ? true : false; }
	Vlan *get(VlanId id);
	void remove(VlanId id);
	void replace(VlanMap *pVlans);
	void empty();
	void commit();
	VlanMap *getMap() { return &m_vlans; }

	bool isReserved(VlanId id);
	
	static void registerCommands();

	CMD(cmdConfigVlan) {
		pLine->setMode(MODE_CONF_VLAN,(void*)g_vlans.get(pCommand->arguments.getInt(0)));	
	}
	CMD(cmdConfigNoVlan) {
		VlanId id = pCommand->arguments.getInt(0);
		if (g_vlans.has(id)) {
			pVlan = g_vlans.get(id);
			if (pVlan->isReserved()) 
				return pLine->write("\r\n%%Default VLAN %d may not be deleted.", pVlan->id);
			g_vlans.remove(id);
		}
	}
	CMD(cmdVlanName) {
		if (pVlan->isReserved()) 
			return pLine->write("\r\n%%Default VLAN %d may not have its name changed.", pVlan->id);
		pVlan->setName(pCommand->arguments.getString(0));
	}
	CMD(cmdVlanNoName) {
		char sName[20];
		if (pVlan->isReserved())
			return;
		sprintf(sName, "VLAN%04d", pVlan->id);
		pVlan->setName(sName);
	}
	CMD(cmdVlanShutdown) {
		if (pVlan->isReserved()) 
			return pLine->write("\r\n%%Command is only allowed on VLAN 2..1001.");
		pVlan->setShutdown(true);
	}
	CMD(cmdVlanNoShutdown) {
		if (pVlan->isReserved()) 
			return pLine->write("\r\n%%Command is only allowed on VLAN 2..1001.");
		pVlan->setShutdown(false);
	}
	CMD(cmdVlanStateActive) {
		if (pVlan->isReserved()) 
			return pLine->write("\r\n%%Default VLAN %d may not have its operational state changed.", pVlan->id);
		pVlan->setActive(true);
	}
	CMD(cmdVlanStateSuspend) {
		if (pVlan->isReserved()) 
			return pLine->write("\r\n%%Default VLAN %d may not have its operational state changed.", pVlan->id);
		pVlan->setActive(false);
	}
	CMD(cmdVlanExit) {
		pLine->setMode(MODE_CONF);
		g_vlans.commit();
	}
	CMD(cmdShowVlan) {
		VlanMap::iterator it;
		VlanMap *pVlans = g_vlans.getMap();
		InterfaceList::iterator iit;
		InterfaceList *pInterfaces = g_interfaces.getList();
		char sStatus[100];
		int nMatch;

		pLine->write("\r\nVLAN Name                             Status    Ports\r\n");
		pLine->write("---- -------------------------------- --------- -------------------------------\r\n");

		for (it=pVlans->begin(); it!=pVlans->end(); ++it) {
			pVlan = it->second;
			
			if (!pVlan->bShutdown) {
				sprintf(sStatus, pVlan->bActive ? "active" : "suspended");
			} else {
				sprintf(sStatus, pVlan->bActive ? "act/lshut" : "sus/lshut");
			}

			pLine->write("%-4d %-32s %-9s ",
				pVlan->id,
				pVlan->sName,
				sStatus
				);
			
			nMatch = 0;
			for (iit=pInterfaces->begin(); iit!=pInterfaces->end(); ++iit) {
				pInterface = *iit;
				if (!pInterface->isTrunk() && pInterface->getAccessVlan() == pVlan->id) {
					if (nMatch++) 
						pLine->write(", ");
					if (nMatch>1 && nMatch%4==1)
						pLine->write("\r\n                                                ");
					pLine->write(pInterface->getNameShort());
				}
			}
			pLine->write("\r\n");
		}
	}
};


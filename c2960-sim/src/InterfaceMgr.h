#pragma once

typedef list<Interface*> InterfaceList;

class Interface;

class InterfaceMgr {

	InterfaceList m_interfaces;
	ProcessList m_processes;

public:
	InterfaceMgr();

	bool add(Interface *pInterface);
	Interface *find(char *sName);
	InterfaceList* getList() { return &m_interfaces; };

	void open();

	static void doShowSwitchport(Line *pLine, Interface *pInterface);
	static void doShowTrunk1(Line *pLine, Interface *pInterface);
	static void doShowTrunk2(Line *pLine, Interface *pInterface);
	static void doShowInterface(Line *pLine, Interface *pInterface);
	static void doShowStatus(Line *pLine, Interface *pInterface);

	void registerProcess(Process *pProc, int nPrio);
	void unregisterProcess(Process *pProc);

	static void registerCommands() {
		g_commands.registerCommand(MODE_EXEC, 0, "show ip", ";IP information");

		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces", ";Interface status and configuration", InterfaceMgr::cmdShowInterfaceAll);
		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces FastEthernet", ";;FastEthernet IEEE 802.3;FastEthernet interface number", InterfaceMgr::cmdShowInterface);

		g_commands.registerCommand(MODE_EXEC, 1, "show ip interface", ";;IP interface status and configuration", InterfaceMgr::cmdShowIpInterface);
		g_commands.registerCommand(MODE_EXEC, 1, "show ip interface brief", ";;;Brief summary of IP status and configuration", InterfaceMgr::cmdShowIpInterfaceBrief);

		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces switchport", ";;Show interface switchport information", InterfaceMgr::cmdShowSwitchportAll);
		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces FastEthernet switchport", ";;FastEthernet IEEE 802.3;FastEthernet interface number;", InterfaceMgr::cmdShowSwitchport);

		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces trunk", ";;Show interface trunk information", InterfaceMgr::cmdShowTrunkAll);
		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces FastEthernet trunk", ";;FastEthernet IEEE 802.3;Show interface trunk information", InterfaceMgr::cmdShowTrunk);

		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces status", ";;Show interface line status", InterfaceMgr::cmdShowIntStatusAll);
		g_commands.registerCommand(MODE_EXEC, 1, "show interfaces FastEthernet status", ";;FastEthernet IEEE 802.3;Show interface line status", InterfaceMgr::cmdShowIntStatus);

		g_commands.registerCommand(MODE_CONF, 15, "interface FastEthernet", "Select an interface to configure;FastEthernet IEEE 802.3", InterfaceMgr::cmdConfigureInterface, InterfaceMgr::evalInterfaceNames);
	}

	CMD(cmdShowInterfaceAll) {
		InterfaceList *pInterfaces = g_interfaces.getList();
		InterfaceList::iterator it;

		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) 
			doShowInterface(pLine, *it);
	}
	CMD(cmdShowInterface) {
		doShowInterface(pLine, pCommand->arguments.getInterface(0));
	}
	CMD(cmdShowSwitchportAll) {
		InterfaceList *pInterfaces = g_interfaces.getList();
		InterfaceList::iterator it;

		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) 
			doShowSwitchport(pLine, *it);
	}
	CMD(cmdShowSwitchport) {
		doShowSwitchport(pLine, pCommand->arguments.getInterface(0));
	}
	CMD(cmdShowTrunkAll) {
		InterfaceList *pInterfaces = g_interfaces.getList();
		InterfaceList::iterator it;
			
		pLine->write("\r\n\r\n");
		pLine->write("Port        Mode             Encapsulation  Status        Native vlan\r\n");
		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) 
			doShowTrunk1(pLine, *it);

		pLine->write("\r\n");
		pLine->write("Port        Vlans allowed on trunk\r\n");
		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it)
			doShowTrunk2(pLine, *it);
	}
	CMD(cmdShowTrunk) {
		pInterface = pCommand->arguments.getInterface(0);

		pLine->write("\r\n\r\n");
		pLine->write("Port        Mode             Encapsulation  Status        Native vlan\r\n");
		doShowTrunk1(pLine, pInterface);
		pLine->write("\r\n");
		pLine->write("Port        Vlans allowed on trunk\r\n");
		doShowTrunk2(pLine, pInterface);
	}

	CMD(cmdShowIntStatusAll) {
		InterfaceList *pInterfaces = g_interfaces.getList();
		InterfaceList::iterator it;
			
		pLine->write("\r\n");
		pLine->write("\r\n");
		pLine->write("Port      Name               Status       Vlan       Duplex  Speed Type\r\n");
		for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) 
			doShowStatus(pLine, *it);
	}
	CMD(cmdShowIntStatus) {
		pInterface = pCommand->arguments.getInterface(0);

		pLine->write("\r\n");
		pLine->write("\r\n");
		pLine->write("Port      Name               Status       Vlan       Duplex  Speed Type\r\n");
		doShowStatus(pLine, pInterface);
	}

	CMD(cmdConfigureInterface) {
		pLine->setMode(MODE_CONF_IF, pCommand->arguments.getInterface(0));
	}
	CMD(cmdShowIpInterface) {
	}
	CMD(cmdShowIpInterfaceBrief) {
		InterfaceList *pInterfaces;
		Interface *pIf;
		InterfaceList::iterator it;
	
		pInterfaces = g_interfaces.getList();

		pLine->write("\r\n");

		// Inteface (27 znakov)
		pLine->write("Interface");
		pLine->writeLen(SPACES, 18);

		// ip adresa (16 znakov)
		pLine->write("IP-Address");
		pLine->writeLen(SPACES, 6);

		// OK (4)
		pLine->write("OK? ");

		// Method (7)
		pLine->write("Method ");

		// status (22)
		pLine->write("Status");
		pLine->writeLen(SPACES, 16);

		// protocol
		pLine->write("Protocol");
		pLine->write("\r\n");

		for (it=pInterfaces->begin(); it!=pInterfaces->end();++it) {
			pIf = *it;
			pLine->write("%- 27s", pIf->getName());
			pLine->write("unassigned      ");
			pLine->write("YES ");
			pLine->write("unsed  ");
			pLine->write("%- 22s", pIf->getLineStateDescr());
			pLine->write("%s", pIf->getProtocolStateDescr());
			pLine->write("\r\n");
		}
	}

	EVAL(evalInterfaceNames);
};


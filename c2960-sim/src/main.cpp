
#include "stdafx.h"

#include "Vlan.h"
#include "ArgumentList.h"
#include "Worker.h"
#include "Command.h"
#include "Line.h"
#include "LineTelnetClient.h"
#include "LineTelnet.h"
#include "CommandTree.h"
#include "Core.h"
#include "Interface.h"
#include "InterfaceNull.h"
#include "InterfacePcap.h"
#include "InterfaceMgr.h"
#include "MacAddressTable.h"
#include "CDPEntry.h"
#include "CDPProcess.h"
#include "DTPProcess.h"
#include "PortSecurity.h"
#include "VlanMgr.h"
#include "VTPProcess.h"

#include <fstream>
#include "yaml-cpp/yaml.h"

#ifdef WIN32
#include "compat.h"
#endif

int g_nTelnetPort;

bool parse(char *sPath)
{
	int i;
	std::string sTelnetPort, sBaseMacAddr;
	YAML::Node config;

	// predvolene hodnoty
	g_baseHwAddr = 0x0000000df0adde00;
	g_nTelnetPort = 10023;

	try {
		std::ifstream fin(sPath);
		YAML::Parser parser(fin);

		if (!parser.GetNextDocument(config)) 
			return false;

		// precitame telnet port

		if (config["console_port"].GetScalar(sTelnetPort))
			g_nTelnetPort = atoi(sTelnetPort.c_str());

		// precitame zakladnu mac adresu
		if (config["base_mac_addr"].GetScalar(sBaseMacAddr)) 
			String2MAC(sBaseMacAddr.c_str(), g_baseHwAddr);

		// precitame konfiguraciu rozhrani
		const YAML::Node& interfaces = config["interfaces"];
		for(i=0; i<(int)interfaces.size(); i++) {
			std::string sController, sId, sDeviceId;
			const YAML::Node& iface = interfaces[i];

			if (!iface["controller"].GetScalar(sController) ||
				!iface["id"].GetScalar(sId)) {
				printf("[!!] incomplete interface configuration\n");
				continue;
			}

			if (sController == "null") {
				g_interfaces.add(new InterfaceNull(sId.c_str(), MAC_ADD(g_baseHwAddr,i+1)));
			} else if (sController == "pcap") {
				if (!iface["device_id"].GetScalar(sDeviceId)) {
					printf("[!!] incomplete interface configuration (pcap)\n");
					continue;
				}
				g_interfaces.add(new InterfacePcap(sId.c_str(), MAC_ADD(g_baseHwAddr,i+1), sDeviceId.c_str()));
			}
		}

	} catch(YAML::Exception& e) {
	    std::cout << "config: " << e.what() << "\n";
		return false;
	}

	return true;
}

int main(int argc, char **argv)
{
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	// nacitame konfiguracny subor
	if (argc <= 1) {
		if (!parse("switch.conf")) {
			if (!parse("config.yaml")) {
				printf("failed to load default config file switch.conf\n");
				exit(1);
			}
		}
	} else {
		if (!parse(argv[1])) {
			printf("failed to load config file \"%s\"\n", argv[1]);
			exit(1);
		}
	}

	time(&g_tNow);
	
	g_pCon0 = new LineTelnet(g_nTelnetPort);
	g_pCon0->start();

	Core::registerCommands();
	Interface::registerCommands();
	InterfaceMgr::registerCommands();
	MacAddressTable::registerCommands();
	Line::registerCommands();
	CDPProcess::registerCommands();
	DTPProcess::registerCommands();
	VTPProcess::registerCommands();
	PortSecurity::registerCommands();
	VlanMgr::registerCommands();

	g_pCDP = new CDPProcess();
	g_switch.registerProcess(g_pCDP);
	g_interfaces.registerProcess(g_pCDP, 95);

	g_switch.registerProcess(&g_vtp);
	g_interfaces.registerProcess(&g_vtp, 96);
		
	// zapnime rozhrania a spustime switch
	g_interfaces.open();
	g_switch.work();

	return 0;
}

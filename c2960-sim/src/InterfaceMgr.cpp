#include "stdafx.h"

#include "Command.h"
#include "Line.h"
#include "Interface.h"
#include "CommandTree.h"
#include "DTPProcess.h"

#include "InterfaceMgr.h"

extern CommandTree g_commands;
extern InterfaceMgr g_interfaces;

InterfaceMgr::InterfaceMgr()
{
}


bool InterfaceMgr::add(Interface *pInterface)
{
	ProcessList::iterator it;

	if (!pInterface)
		return false;

	m_interfaces.push_back(pInterface);

	// spustime na rozhrani vsetky globalne procesy co mame
	for (it=m_processes.begin(); it!=m_processes.end(); ++it) 
		pInterface->registerProcess((*it)->m_pProc, (*it)->m_nPrio);

	//TODO:
	// apply configuration from running config

	// a nastartujeme co treba
	pInterface->start();

	return true;
}

Interface *InterfaceMgr::find(char *sName)
{
	InterfaceList::iterator it;
	Interface *pInterface;

	for (it=m_interfaces.begin(); it!=m_interfaces.end(); ++it) {
		pInterface = (*it);
		if (strnicmp(pInterface->getName(), sName, strlen(pInterface->getName())) == 0) 
			return pInterface;
	}

	return 0;
}

void  InterfaceMgr::open()
{
	InterfaceList::iterator it;

	for (it=m_interfaces.begin(); it!=m_interfaces.end(); ++it) 
		(*it)->open();
}

void InterfaceMgr::registerProcess(Process *pProc, int nPrio) {
	// pridame do zoznamu a kazdemu z aktivnych rozhrani
	InterfaceList::iterator it;

	m_processes.push(pProc, nPrio);

	for (it=m_interfaces.begin(); it!=m_interfaces.end(); ++it)
		(*it)->registerProcess(pProc, nPrio);
}

void InterfaceMgr::unregisterProcess(Process *pProc) {
	// odstranime zo zoznamu aj kazdemu z aktivnych rozhrani
	InterfaceList::iterator it;

	m_processes.remove(pProc);

	for (it=m_interfaces.begin(); it!=m_interfaces.end(); ++it)
		(*it)->unregisterProcess(pProc);
}

EVALI(InterfaceMgr::evalInterfaceNames) {
	EvalList list;
	InterfaceList *pInterfaces;
	InterfaceList::iterator it;
	
	pInterfaces = g_interfaces.getList();

	for (it=pInterfaces->begin(); it!=pInterfaces->end();++it) 
		list.push_back((*it)->getName());

	return list;
}

void InterfaceMgr::doShowSwitchport(Line *pLine, Interface *pInterface)
{
	pLine->write("\r\n");

	pLine->write("Name: %s\r\n", pInterface->getName());
	pLine->write("Switchport: Enabled\r\n");
	pLine->write("Administrative Mode: %s\r\n", pInterface->getDTPProc()->getModeDesc());
	pLine->write("Operational Mode: TODO\r\n");
	pLine->write("Administrative Trunking Encapsulation: dot1q\r\n");
	pLine->write("Negotiation of Trunking: Disabled\r\n"); // isEnabled
	pLine->write("Access Mode VLAN: %d (TODO)\r\n", pInterface->getAccessVlan());
	pLine->write("Trunking Native Mode VLAN: %d (TODO)\r\n", pInterface->getNativeVlan());
	pLine->write("Trunking VLANs Enabled: ALL\r\n");
	pLine->write("Trunking VLANs Active: none\r\n");
	pLine->write("Protected: false\r\n");
	pLine->write("Priority for untagged frames: 0\r\n");
	pLine->write("Override vlan tag priority: FALSE\r\n");
	pLine->write("Voice VLAN: none\r\n");
	pLine->write("Appliance trust: none\r\n");
}

void InterfaceMgr::doShowInterface(Line *pLine, Interface *pInterface) 
{
	pLine->write("\r\n");
	pLine->write("%s is %s, line protocol is %s\r\n", pInterface->getName(), pInterface->getLineStateDescr(), pInterface->getProtocolStateDescr());
	pLine->write("  Hardware is Fast Ethernet, address is %s", MAC2String(pInterface->getHwAddr()));
	pLine->write(" (bia %s)\r\n",MAC2String(pInterface->getHwAddr()));
	pLine->write("  MTU 1500 bytes, BW 100000 Kbit/sec, DLY 100 usec,\r\n");
	pLine->write("\r\n");
	//TODO:
/*

FastEthernet0/1 is up, line protocol is up (connected)
  Hardware is Fast Ethernet, address is 001a.a162.de81 (bia 001a.a162.de81)
  MTU 1500 bytes, BW 100000 Kbit/sec, DLY 100 usec,
pLine->write("     reliability 255/255, txload 1/255, rxload 1/255
pLine->write("  Encapsulation ARPA, loopback not set
pLine->write("  Keepalive set (10 sec)
pLine->write("  Full-duplex, 100Mb/s, media type is 10/100BaseTX
pLine->write("  input flow-control is off, output flow-control is unsupported
pLine->write("  ARP type: ARPA, ARP Timeout 04:00:00
  Last input 00:01:07, output 00:00:00, output hang never
  Last clearing of "show interface" counters never
  Input queue: 0/75/0/0 (size/max/drops/flushes); Total output drops: 0
  Queueing strategy: fifo
  Output queue: 0/40 (size/max)
  5 minute input rate 0 bits/sec, 0 packets/sec
  5 minute output rate 0 bits/sec, 0 packets/sec
     188 packets input, 31722 bytes, 0 no buffer
     Received 188 broadcasts (91 multicasts)
     0 runts, 0 giants, 0 throttles
     0 input errors, 0 CRC, 0 frame, 0 overrun, 0 ignored
     0 watchdog, 91 multicast, 0 pause input
     0 input packets with dribble condition detected
     2851 packets output, 211288 bytes, 0 underruns
     0 output errors, 0 collisions, 1 interface resets
     0 unknown protocol drops
     0 babbles, 0 late collision, 0 deferred
     0 lost carrier, 0 no carrier, 0 pause output
     0 output buffer failures, 0 output buffers swapped out
*/
}

void InterfaceMgr::doShowStatus(Line *pLine, Interface *pInterface) 
{
	pLine->write("%-9s %-18s connected    %-10d a-full  a-100 10/100BaseTX\r\n",
		pInterface->getNameShort(), pInterface->getDescription(), pInterface->getAccessVlan());
}

void InterfaceMgr::doShowTrunk1(Line *pLine, Interface *pInterface) {
	if (!pInterface->isTrunk())
		return;

	pLine->write("%-11s %-16s %-14s %-13s %d\r\n",
		pInterface->getNameShort(),
		"on",
		"802.1q",
		"trunking",
		pInterface->getNativeVlan()
		);
}

void InterfaceMgr::doShowTrunk2(Line *pLine, Interface *pInterface) {
	if (!pInterface->isTrunk())
		return;

	pLine->write("%-11s 1-4096\r\n", pInterface->getNameShort());
}
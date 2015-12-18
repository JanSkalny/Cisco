#include "stdafx.h"

#include "Vlan.h"
#include "Line.h"
#include "LineTelnet.h"
#include "Core.h"
#include "Interface.h"

#include "CommandTree.h"
#include "InterfaceMgr.h"
#include "VlanMgr.h"
#include "MacAddressTable.h"
#include "CDPEntry.h"
#include "CDPProcess.h"
#include "VTPProcess.h"

LineTelnet *g_pCon0;
CommandTree g_commands;
InterfaceMgr g_interfaces;
MacAddressTable g_macAddressTable;
Core g_switch;
time_t g_tNow;
MAC g_baseHwAddr;
CDPProcess *g_pCDP;
VlanMgr g_vlans;
VTPProcess g_vtp;

char * MAC2String(MAC addr)
{
	static char sMac[20];

	memset(sMac, 0, 20);
	sprintf(sMac, "%02x%02x.%02x%02x.%02x%02x",
		((BYTE*)&addr)[0],
		((BYTE*)&addr)[1],
		((BYTE*)&addr)[2],
		((BYTE*)&addr)[3],
		((BYTE*)&addr)[4],
		((BYTE*)&addr)[5]
		);

	return sMac;
}

bool String2MACparse(const char *str, int start, int end, MAC& addr)
{
	int c, i, val;

	for (i=start; i<end; ) {
		c = tolower(*(str++));

		if ('a' <= c && c <= 'f') {
			val = c-'a'+10;
		} else if ('0' <= c && c <= '9') {
			val = c-'0';
		} else {
			return false;
		}

		if (i%2)
			addr |= (long long)val << (i++*4-4);
		else
			addr |= (long long)val << (i++*4+4);
	}

	return true;
}


bool String2MAC(const char *str, MAC &addr)
{
	int i;
	const char *x;

	addr = 0;

	// AAAA.bbbb.cccc
	x = strchr(str, '.');
	if (!x) return false;
	i = 4 - (int)(x - str);
	if (!String2MACparse(str, i, 4, addr)) return false;
	str = x+1;
	
	// aaaa.BBBB.cccc
	x = strchr(str, '.');
	if (!x) return false;
	i = 4 - (int)(x - str);
	if (!String2MACparse(str, 4+i, 8, addr)) return false;
	str = x+1;

	// aaaa.bbbb.CCCC
	i = 4 - strlen(str);
	if (i < 0) return false;
	if (!String2MACparse(str, 8+i, 12, addr)) return false;

	return true;
}

IN_ADDR inaddrify(unsigned long addr)
{
    IN_ADDR tmp;
    tmp.S_un.S_addr = addr;
    return tmp;
}
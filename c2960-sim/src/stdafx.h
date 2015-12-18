
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netdb.h>
#else
#include <stdarg.h>
#include <Winsock2.h>
#include "contrib/telnet.h"
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

using namespace std;
#include <list>
#include <deque>
#include <vector>
#include <map>

#include <pcap.h>

#define SPACES "                                                        "

class Line;
class Command;
class Interface;
class Vlan;

typedef void(*CommandHandler)(Line *pLine, Command *pCommand, Interface *pInterface, Vlan *pVlan, ...);
//#define CMDI(x) void (x)(Line *pLine, Command *pCommand, ...)
//#define CMD(x) static CMDI(x)
//#define CMDX static CommandHandler
//#define CMD_PARAM Line *pLine, Command *pCommand, ...
#define CMD(x) static void (x) (Line *pLine, Command *pCommand, Interface *pInterface, Vlan *pVlan, ...)
#define CMDI(x) void (x) (Line *pLine, Command *pCommand, Interface *pInterface, Vlan *pVlan, ...)

typedef list<char*> EvalList;
typedef EvalList (*CommandEvaluator)(Line *pLine, Command *pCommand);
#define EVALI(x) EvalList (x)(Line *pLine, Command *pCommand)
#define EVAL(x) static EVALI(x)

typedef long long MAC;
char* MAC2String(MAC addr);
bool String2MAC(const char *, MAC& addr);

#define MAC2PTR(mac) (BYTE*)(&(mac))
#define PTR2MAC(x) \
(\
((long long)((BYTE*)x)[5])<<40 |\
((long long)((BYTE*)x)[4])<<32 |\
((long long)((BYTE*)x)[3])<<24 |\
((long long)((BYTE*)x)[2])<<16 |\
((long long)((BYTE*)x)[1])<<8 |\
((long long)((BYTE*)x)[0]) \
)

#define MAC_ADD(mac, x) (mac + (((long long)x)<<40))

#define IS_BROADCAST_MAC(x) ( ((BYTE*)&x)[0] & 0x1 )

#define MAC_BCAST		0x0000ffffffffffff
#define MAC_MCAST_DTP	0x0000cccccc0c0001 
#define MAC_MCAST_CDP	0x0000cccccc0c0001
#define MAC_MCAST_VTP	0x0000cccccc0c0001

class CommandTree;
class InterfaceMgr;
class MacAddressTable;
class Core;
class VlanMgr;
class CDPProcess;
class VTPProcess;
class LineTelnet;

extern LineTelnet *g_pCon0;
extern CommandTree g_commands;
extern InterfaceMgr g_interfaces;
extern MacAddressTable g_macAddressTable;
extern Core g_switch;
extern time_t g_tNow;
extern MAC g_baseHwAddr;
extern CDPProcess *g_pCDP;
extern VTPProcess g_vtp;
extern VlanMgr g_vlans;

class DataBlock {
public:
	DataBlock(int nLen) {
		this->nLen = nLen;
		pData = (BYTE*) malloc(nLen);
		memset(pData, 0, nLen);
	}
	~DataBlock() {
		free(pData);
	}
	int nLen;
	BYTE *pData;
};

IN_ADDR inaddrify(unsigned long addr);

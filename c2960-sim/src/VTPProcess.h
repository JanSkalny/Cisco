#pragma once

#include "process.h"

class VlanMgr;

class VTPProcess :
	public Process
{
	enum {
		MAX_DOMAIN_NAME_LEN = 32,
	};
	enum {
		VTP_SUMMARY_TIMER = 300,
		VTP_REQUEST_TIMER = 5,
		VTP_NEIGHBOR_TIMEOUT = 10,
	};

	typedef enum {
		VTP_MODE_CLIENT,
		VTP_MODE_SERVER,
		VTP_MODE_TRANSPARENT,
	} VTPMode;

	class Neighbor {
	public:
		Neighbor(MAC addr) {
			nFollowers = yExpectedSeq = 0;
			tLastSeen = g_tNow;
			this->addr = addr;
		}
		MAC addr;
		int nFollowers;
		BYTE yExpectedSeq;
		VlanMap vlans;
		time_t tLastSeen;
	};

	typedef map<MAC, Neighbor*> NeighborMap;
	NeighborMap m_neighbors;

	time_t m_tLastTimeout;
	time_t m_tLastSummarySend;
	time_t m_tLastRequestSend;
	bool m_bActiveRequest;

	VTPMode m_mode;
	char m_sDomain[MAX_DOMAIN_NAME_LEN+1];
	DWORD m_dwRevision;

	char *m_sSecret;
	BYTE m_sSecretMD5[16];
	BYTE m_sDigest[16];
	DWORD m_dwUpdaterAddr;
	char m_sUpdateTS[12];
	typedef list<DataBlock*> DataBlockList;
	DataBlockList m_vlanInfoBlocks;

	DWORD m_dwStatsSummaryTx, m_dwStatsSummaryRx;
	DWORD m_dwStatsSubsetTx, m_dwStatsSubsetRx;
	DWORD m_dwStatsRequestTx, m_dwStatsRequestRx;

public:
	VTPProcess(void);
	~VTPProcess(void);

	int getNextEventTimeout();
	bool onTimeout();
	bool onRecv(Frame *pFrame);
	void onVlanModified();
	bool onUp(Interface *pInterface);

	void setMode(VTPMode mode);
	void setDomain(char *sName);

	Neighbor *getNeighbor(MAC addr);

	void sendSummary();
	void sendSummary(Interface *pInterface);
	void sendRequest();
	void sendRequest(Interface *pInterface);
	void sendFullUpdate(Interface *pInterface);

	void updateVlanInfoBlocks();
	void createVlanInfoBlock(int nLen, VlanMap::iterator &it);
	void deleteVlanInfoBlocks();
	DataBlockList *getVlanInfoBlocks() { return &m_vlanInfoBlocks; }
	BYTE *getVlanInfoDigest() { return m_sDigest; }
	DWORD getRevision() { return m_dwRevision; }
	char *getDomainName() { return m_sDomain; }
	char *getUpdateTS() { return m_sUpdateTS; }
	DWORD getUpdaterAddr() { return m_dwUpdaterAddr; }
	char *getModeDesc() {
		switch(m_mode) {
		case VTP_MODE_SERVER: return "Server";
		case VTP_MODE_CLIENT: return "Client";
		case VTP_MODE_TRANSPARENT: return "Transparent";
		}
		return "";
	}
	BYTE *getDigest() { return m_sDigest; }

	DWORD getStatsSummaryRx() { return m_dwStatsSummaryRx; };
	DWORD getStatsSummaryTx() { return m_dwStatsSummaryTx; };
	DWORD getStatsSubsetRx() { return m_dwStatsSubsetRx; };
	DWORD getStatsSubsetTx() { return m_dwStatsSubsetTx; };
	DWORD getStatsRequestRx() { return m_dwStatsRequestRx; };
	DWORD getStatsRequestTx() { return m_dwStatsRequestTx; };
	
	static void registerCommands();
	CMD(cmdVTPModeClient) { g_vtp.setMode(VTP_MODE_CLIENT); }
	CMD(cmdVTPModeServer) { g_vtp.setMode(VTP_MODE_SERVER); }
	CMD(cmdVTPModeTransparent) { g_vtp.setMode(VTP_MODE_TRANSPARENT); }
	CMD(cmdVTPDomain) { g_vtp.setDomain(pCommand->arguments.getString(0)); }
	CMD(cmdVTPPassword) { }
	CMD(cmdVTPNoPassword) { }
	CMD(cmdVTPVersion) { }
	CMD(cmdShowVTPStatus) {
		char *sUpdateTS = g_vtp.getUpdateTS();
		BYTE *sDigest = g_vtp.getDigest();
		
		pLine->write("\r\n");
		pLine->write("VTP Version capable             : 1\r\n");
		pLine->write("VTP version running             : 1\r\n");
		pLine->write("VTP Domain Name                 : %s\r\n", g_vtp.getDomainName());
		pLine->write("VTP Pruning Mode                : Disabled\r\n");
		pLine->write("VTP Traps Generation            : Disabled\r\n");
		pLine->write("Device ID                       : %s\r\n", MAC2String(g_baseHwAddr));
		pLine->write("Configuration last modified by %s at %c%c-%c%c-%c%c %c%c:%c%c:%c%c\r\n",
			inet_ntoa(inaddrify(g_vtp.getUpdaterAddr())), 
			sUpdateTS[0], sUpdateTS[1],
			sUpdateTS[2], sUpdateTS[3],
			sUpdateTS[4], sUpdateTS[5],
			sUpdateTS[6], sUpdateTS[7],
			sUpdateTS[8], sUpdateTS[9],
			sUpdateTS[10], sUpdateTS[11]);
		pLine->write("Local updater ID is 0.0.0.0 (no valid interface found)\r\n\r\n");

		pLine->write("Feature VLAN:\r\n");
		pLine->write("--------------\r\n");
		pLine->write("VTP Operating Mode                : %s\r\n", g_vtp.getModeDesc());
		pLine->write("Maximum VLANs supported locally   : 255\r\n", g_vtp.getModeDesc());
		pLine->write("Number of existing VLANs          : %d\r\n", g_vlans.getMap()->size());
		pLine->write("Configuration Revision            : %d\r\n", g_vtp.getRevision());
		pLine->write("MD5 digest                        : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",
			sDigest[0], sDigest[1], sDigest[2], sDigest[3], sDigest[4], sDigest[5], sDigest[6], sDigest[7]);
		pLine->write("                                    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",
			sDigest[8], sDigest[9], sDigest[10], sDigest[11], sDigest[12], sDigest[13], sDigest[14], sDigest[15]);
	}
	CMD(cmdShowVTPCounter) {
		pLine->write("\r\n");
		pLine->write("VTP statistics:\r\n");
		pLine->write("Summary advertisements received    : %d\r\n", g_vtp.getStatsSummaryRx());
		pLine->write("Subset advertisements received     : %d\r\n", g_vtp.getStatsSubsetRx());
		pLine->write("Request advertisements received    : %d\r\n", g_vtp.getStatsRequestRx());
		pLine->write("Summary advertisements transmitted : %d\r\n", g_vtp.getStatsSummaryTx());
		pLine->write("Subset advertisements transmitted  : %d\r\n", g_vtp.getStatsSubsetTx());
		pLine->write("Request advertisements transmitted : %d\r\n", g_vtp.getStatsRequestTx());
		pLine->write("Number of config revision errors   : 0\r\n");
		pLine->write("Number of config digest errors     : 0\r\n");
		pLine->write("Number of V1 summary errors        : 0\r\n");
		pLine->write("\r\n");
	}
};


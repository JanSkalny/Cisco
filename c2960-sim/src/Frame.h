#pragma once

#include "Vlan.h"

class Interface;

#define ETHERTYPE_NONE 0
#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_8021Q 0x8100
#define ETHERTYPE_RAW 0xffff
#define ETHERTYPE_DTP 0x2004
#define ETHERTYPE_CDP 0x2000
#define ETHERTYPE_VTP 0x2003

#define PROTO_TCP	0x06
#define	PROTO_UDP	0x11
#define	PROTO_ICMP	0x01
#define	PROTO_IGMP	0x02
#define	PROTO_EIGRP	0x58
#define	PROTO_OSPF	0x59

class FrameData {
public:
	Interface *pIfOrigin;

	VlanId vlan;
	bool bTagged;
	MAC macSA, macDA;
	WORD wEtherType;

	bool bEthernet2;

	int nL3Offset;
};

class Frame : public FrameData
{
	
public:
	typedef enum {
		MAX_LEN = 1600
	};

	typedef enum {
		OK=0,
		ERR_RUNT,
		ERR_GIANT,
		ERR_CHECKSUM
	} ParserError;

protected:
	BYTE *m_pData;
	int m_nLen;
	bool m_bConstructed;

public:
	Frame(void);
	Frame(const Frame&);
	Frame(struct pcap_pkthdr *header, BYTE *data, Interface *origin);
	virtual ~Frame(void);

	virtual bool parse();
	virtual bool create(int nPayloadLen);
	WORD computeChecksum(BYTE *hdr, int len, DWORD initial);

	BYTE *getData() { return m_pData; }
	int getLen() { return m_nLen; }

	// .. parsovane parametre pridavat do triedy 
};


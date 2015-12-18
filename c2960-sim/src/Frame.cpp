#include "stdafx.h"

#include "Frame.h"
#include "net_conversion.h"


Frame::Frame(void)
{
	m_pData = 0;
	m_bConstructed = false;
	m_nLen = 0;

	nL3Offset = 0;
	bEthernet2 = true;
}
Frame::Frame(struct pcap_pkthdr *header, BYTE *data, Interface *pOrigin)
{
	Frame();
	
	m_pData = data;
	m_bConstructed = false;
	m_nLen = (header) ? header->caplen : 0;
	pIfOrigin = pOrigin;

	parse();
}

Frame::Frame(const Frame& orig) 
	: FrameData(orig)	// default copy constructor - prekopiruje vsetko
{
	m_nLen = orig.m_nLen;
	m_pData = (m_nLen > 0) ? (BYTE*) malloc(m_nLen) : 0;
	m_bConstructed = true;
	memcpy(m_pData, orig.m_pData, m_nLen);
}

Frame::~Frame(void)
{
	if (m_bConstructed && m_pData) {
		free(m_pData);
		m_pData = 0;
	}
}

bool Frame::parse()
{	
	int o;

	// cleanup
	bTagged = false;

	// kontrola FCS
	//TODO:

	USE_NET_CONVERSION(m_pData);

	// l2 dst a src mac
	macDA = PTR2MAC(m_pData);
	macSA = PTR2MAC(m_pData+6);
	wEtherType = READ_WORD_NO(12);
	o = 14;

	if (wEtherType < 0x600) {
		bEthernet2 = false;

		// 802.3
		if (READ_WORD(o) == ETHERTYPE_RAW) {
			// ipx
			wEtherType = ETHERTYPE_RAW;
			return 0;
		} else if (READ_WORD(o) == 0xaaaa) {
			// SNAP
			//TODO: pozriet spravny offset
			wEtherType = READ_WORD_NO(o+6);
		} else {
			wEtherType = ETHERTYPE_NONE;
			return true;
		}
		o += 8;
	} else if (wEtherType == 0x8100) {
		bTagged = true;
		vlan = (READ_WORD_NO(14) & 0xfff);
		wEtherType = READ_WORD_NO(16);
		o+=4;
	}/* else { // ethernet2 } */ 

	nL3Offset = o;

	bEthernet2 = true;
	
	if (bTagged) {
		// strip VLAN tag
		for (o=12; o!=m_nLen; o++)
			m_pData[o] = m_pData[o+4];
		m_nLen -= 4;
	}

	return true;
}

bool Frame::create(int nPayloadLen)
{
	nL3Offset = (bEthernet2) ? 14 : 22;
	m_nLen = nPayloadLen + nL3Offset;
	if (m_nLen >= 1600) {
		printf("frame is too large\n");
		return false;
	}

	if (m_bConstructed && m_pData) 
		free(m_pData);
	
	m_pData = (BYTE*)malloc(m_nLen + 10/* fake */);
	m_bConstructed = true;

	USE_NET_CONVERSION(m_pData);

	memcpy(m_pData+0, MAC2PTR(macDA), 6);
	memcpy(m_pData+6, MAC2PTR(macSA), 6);
	 
	if (bEthernet2) {
		WRITE_WORD_NO(12, wEtherType);
	} else {
		WRITE_WORD_NO(12, (WORD)m_nLen-14);
		WRITE_WORD_NO(14, 0xAAAA);
		WRITE_DWORD_NO(16, 0x0300000c);
		WRITE_WORD_NO(20, wEtherType);
	}

	return true;
}

WORD Frame::computeChecksum(BYTE *hdr, int len, DWORD initial)
{
	DWORD sum = initial;

	while (len > 1) {
		sum += *((WORD*)hdr);
		hdr += 2;
		len -= 2;
	}

	if (len == 1) 
		sum += *hdr;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ntohs((WORD)(~sum));
}


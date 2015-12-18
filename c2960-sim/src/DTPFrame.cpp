#include "stdafx.h"

#include "DTPFrame.h"
#include "net_conversion.h"


DTPFrame::DTPFrame(void)
{
}


DTPFrame::~DTPFrame(void)
{
}

bool DTPFrame::parse()
{
	int o;

	WORD wType=1, wLen;

	if (!Frame::parse())
		return false;

	o = nL3Offset;

	yTos = yTas = yTot = yTat = 0;
	sVTPDomain = 0;
	macNeighbor = 0;

	USE_NET_CONVERSION(m_pData);

	// version
	yVer = READ_BYTE(o++);

	// TLV
	wLen = wType = 0xffff;
	while (o < m_nLen && wType && wLen) {
		wType = READ_WORD_NO(o);
		wLen = READ_WORD_NO(o+2);
		switch(wType) {
		case 1: 
			// VTP domain name	string
			sVTPDomain = (char*)(m_pData+o+4);
			break; 
		case 2: 
			// STATUS			enum 1B - 04=dyn_auto 03=dyn_desir 81=trunk 02=access
			yTas = READ_NIBBLE_LO(o+4);
			yTos = READ_NIBBLE_HI(o+4);				
			break; 
		case 3: 
			// DTP type			const 1B - "0xa5"
			yTat = READ_NIBBLE_LO(o+4);
			yTot = READ_NIBBLE_HI(o+4);				
			break; 
		case 4: 
			// Neighbor			sender mac addr
			macNeighbor = READ_MAC(o+4);
			break; 
		}
		o += wLen;
	}

	return true;
}

bool DTPFrame::create(int nPayloadLen)
{
	int o, nLen = nPayloadLen;
	WORD wVTPDomainLen;

	wVTPDomainLen = sVTPDomain ? strlen(sVTPDomain) : 1;

	nLen += 1; // version
	nLen += 4*4 + 1 + 1 + 6 + wVTPDomainLen; // TLV + data

	bEthernet2 = false;
	wEtherType = ETHERTYPE_DTP;
	macDA = MAC_MCAST_DTP;
	yVer = 1;

	if (!Frame::create(nLen))
		return false;

	o = nL3Offset;

	USE_NET_CONVERSION(m_pData);

	WRITE_BYTE(o++, 0x01); // version

	// TLV - VTP domain name
	WRITE_WORD_NO(o, 0x0001);
	WRITE_WORD_NO(o+2, 4+wVTPDomainLen);
	if (sVTPDomain)
		memcpy(BY(o+4), sVTPDomain, wVTPDomainLen);
	else
		WRITE_BYTE(o+4, 0);
	o += 4+wVTPDomainLen;

	// TLV status
	WRITE_WORD_NO(o, 0x0002);
	WRITE_WORD_NO(o+2, 5);
	WRITE_NIBBLE_HI(o+4, yTos);
	WRITE_NIBBLE_LO(o+4, yTas);	
	o += 5;

	// TLV dtp_type
	WRITE_WORD_NO(o, 0x0003);
	WRITE_WORD_NO(o+2, 5);
	WRITE_NIBBLE_HI(o+4, yTot);
	WRITE_NIBBLE_LO(o+4, yTat);	
	o += 5;

	// TLV neighbor
	WRITE_WORD_NO(o, 0x0004);
	WRITE_WORD_NO(o+2, 10);
	WRITE_MAC(o+4, macNeighbor);
	
	return true;
}
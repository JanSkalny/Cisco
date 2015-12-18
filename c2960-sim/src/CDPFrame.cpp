#include "stdafx.h"

#include "CDPEntry.h"
#include "CDPFrame.h"
#include "Interface.h"
#include "InterfaceMgr.h"
#include "CDPProcess.h"
#include "net_conversion.h"

CDPFrame::CDPFrame(void)
{
}

CDPFrame::~CDPFrame(void)
{
}

bool CDPFrame::parse()
{
	int o;
	BYTE yTmp;
	WORD wLen, wTmp;
	DWORD dwTmp;
	CDPVarType type;

	if (!Frame::parse())
		return false;

	o = nL3Offset;

	USE_NET_CONVERSION(m_pData);

	yVer = READ_BYTE(o);
	yHoldTime = yTTL = READ_BYTE(o+1);
	wChecksum = READ_WORD_NO(o+2);
	o+=4;

	macAddr = macSA;
	tLastUpdate = g_tNow;
	pOrigin = pIfOrigin;

	type=CDP_DEVICE_ID, wLen=1;
	while (o < m_nLen && type && wLen) {
		type = (CDPVarType) READ_WORD_NO(o);
		wLen = READ_WORD_NO(o+2);

		switch (type) {
		case CDP_DEVICE_ID:
		case CDP_PORT_ID:
		case CDP_VERSION:
		case CDP_PLATFORM:
		case CDP_VTP_DOMAIN:
			// device id
			//TODO: retazce niesu escapovane 0, no nastastie je dalsi TYPE zacinajuci 0 :)
			addVar(type, (char*)BY(o+4));
			break;

		case CDP_CAPABILITIES:
			dwTmp = READ_DWORD_NO(o+4);
			addVar(type, dwTmp);
			break;

		case CDP_NATIVE_VLAN:
			wTmp = READ_WORD_NO(o+4);
			addVar(type, wTmp);
			break;

		case CDP_DUPLEX:
			yTmp = READ_BYTE(o+4);
			addVar(type, yTmp);
			break;

		case CDP_MGMT_ADDRESS:
		case CDP_ADDRESS:
			// TODO:
			break;
		}

		o += wLen;
	}

	return true;
}

bool CDPFrame::create(int nPayloadLen)
{
	int o, oChecksum, nLen = nPayloadLen;
	bool bChecksumHack1 = false;
	WORD wChecksum;
	BYTE yLastByte;
	
	bEthernet2 = false;
	wEtherType = ETHERTYPE_CDP;
	macDA = MAC_MCAST_CDP;

	//TODO: prepocitat nLen
	nLen =	4 // header
			+ 7 // hostname 
			+ 4+strlen(pIfOrigin->getName()) // port
			+ 8; //capa

	if (!Frame::create(nLen))
		return false;

	o = nL3Offset;

	USE_NET_CONVERSION(m_pData);

	WRITE_BYTE(o, 0x02);	// ver
	WRITE_BYTE(o+1, (BYTE)CDPProcess::getHoldTime);	// TTL
	WRITE_WORD_NO(o+2, 0x0000); // TODO: checksum
	oChecksum = o+2;
	o+=4;

	// TLV hostname
	// MEGA-hack
	WRITE_WORD_NO(o, CDP_DEVICE_ID);
	WRITE_WORD_NO(o+2, 4+3);
	memcpy(BY(o+4), "SW1", 3);
	o+=7;

	//TLV port id
	WRITE_WORD_NO(o, CDP_PORT_ID);
	WRITE_WORD_NO(o+2, 4+strlen(pIfOrigin->getName()));
	memcpy(BY(o+4), pIfOrigin->getName(), strlen(pIfOrigin->getName()));
	o+=4+strlen(pIfOrigin->getName());

	WRITE_WORD_NO(o, CDP_CAPABILITIES);
	WRITE_WORD_NO(o+2, 8);
	WRITE_DWORD_NO(o+4, 0x00000008); // plain switch
	o+=8;

	if (nLen & 1) {
		bChecksumHack1 = true;

        /* Swap bytes in last word */
		yLastByte = m_pData[nL3Offset+nLen] = m_pData[nL3Offset+nLen-1];
		m_pData[nL3Offset+nLen-1] = 0;

		nLen++;
	}

	wChecksum = computeChecksum(BY(nL3Offset), nLen, 0);
	WRITE_WORD_NO(oChecksum, wChecksum);

	if (bChecksumHack1) {
		nLen--;
		m_pData[nL3Offset+nLen-1] = yLastByte;
	}

	return true;
}


  
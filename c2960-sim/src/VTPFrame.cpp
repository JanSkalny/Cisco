#include "stdafx.h"

#include "net_conversion.h"
#include "Vlan.h"
#include "Line.h"
#include "Interface.h"
#include "InterfaceMgr.h"
#include "VlanMgr.h"
#include "VTPProcess.h"

#include "VTPFrame.h"


VTPFrame::VTPFrame(void)
{
	m_pVlans = 0;
	ySeq = 0;
	yVersion = 1;
	pVlanInfo = 0;
	memset(sDomainName, 0, sizeof(sDomainName));
}

VTPFrame::~VTPFrame(void)
{
}

bool VTPFrame::parse()
{
	int o, nLen;

	if (!Frame::parse())
		return false;

	o = nL3Offset;

	USE_NET_CONVERSION(m_pData);

	// spolocna hlavicka
	yVersion = READ_BYTE(o);
	yCode = READ_BYTE(o+1);
	ySeq = READ_BYTE(o+2);
	yDomainNameLen = READ_BYTE(o+3);
	o += 4;

	if (yDomainNameLen > MAX_DOMAIN_NAME_LEN)
		yDomainNameLen = MAX_DOMAIN_NAME_LEN;
	memset(sDomainName, 0, sizeof(sDomainName));
	memcpy(sDomainName, BY(o), yDomainNameLen);
	o += MAX_DOMAIN_NAME_LEN;

	dwRevision = READ_DWORD_NO(o);
	o += 4;

	// zvysok sa lisi podla code-u
	switch (yCode) {
	case VTP_CODE_SUMMARY:
		memcpy(sUpdateTs, BY(o), UPDATE_TS_LEN);
		memcpy(sMd5Digest, BY(o+UPDATE_TS_LEN), MD5_DIGEST_LEN);
		break;

	case VTP_CODE_SUBSET:
		nLen = 1;
		while (nLen && o < m_nLen) {
			parseVlan(o, nLen);
			o += nLen;
		}
		break;

	case VTP_CODE_REQUEST:
		break;

	//TODO: objavili sme code=4 :)
	}

	return true;
}

void VTPFrame::parseVlan(int o, int& nLen) 
{
	Vlan *pVlan;

	BYTE yStatus, yType, yNameLen;
	VlanId id;
	WORD wMtu;
	DWORD dwDot10Index;

	USE_NET_CONVERSION(m_pData);

	nLen = READ_BYTE(o);
	if (nLen+o > m_nLen)
		// kratka sprava
		return;
	yStatus = READ_BYTE(o+1);
	yType = READ_BYTE(o+2);
	yNameLen = READ_BYTE(o+3);

	id = READ_WORD_NO(o+4);
	wMtu = READ_WORD_NO(o+6);
	dwDot10Index = READ_DWORD_NO(o+8);

	if (yType != 1) {
		// not an ethernet
		return;
	}
	if (id == 1) {
		// default ignorujeme
		return;
	}
	
	pVlan = new Vlan(id);
	pVlan->setActive(yStatus&1 ? false : true);
	pVlan->commit();
	pVlan->dwDot10Index = dwDot10Index;
	memcpy(pVlan->sName, BY(o+12), yNameLen);
	pVlan->sName[yNameLen] = 0; 
	(*m_pVlans)[id] = pVlan;
}

bool VTPFrame::create(int nPayloadLen)
{
	int nLen = nPayloadLen, o;
	char *sDomain;

	wEtherType = ETHERTYPE_VTP;
	bEthernet2 = false;
	macDA = MAC_MCAST_VTP;
	yVersion = 1;
	
	sDomain = g_vtp.getDomainName();

	nLen = 4 + 32;
	switch (yCode) {
	case VTP_CODE_SUMMARY:	nLen += 4 + 4 + 12 + 16 + 5; break;
	case VTP_CODE_SUBSET:	nLen += 4 + pVlanInfo->nLen; break; //TODO: 
	case VTP_CODE_REQUEST:	nLen += 4; break;
	}

	if (!Frame::create(nLen))
		return false;

	o = nL3Offset;

	USE_NET_CONVERSION(m_pData);

	// spolocna hlavicka
	WRITE_BYTE(o, yVersion);
	WRITE_BYTE(o+1, yCode);
	WRITE_BYTE(o+2, ySeq);
	WRITE_BYTE(o+3, strlen(sDomain));

	memcpy(BY(o+4), sDomain, MAX_DOMAIN_NAME_LEN);

	o += 4 + MAX_DOMAIN_NAME_LEN;

	switch (yCode) {
	case VTP_CODE_SUMMARY:
		WRITE_DWORD_NO(o, g_vtp.getRevision());
		WRITE_DWORD_NO(o+4, 0); // XXX: updater identity
		memset(BY(o+8), 0x30, 12); // "000000000000"
		memcpy(BY(o+20), g_vtp.getVlanInfoDigest(), 16);
		o+=20+16;
		BY(o)[0] = 1;
		BY(o)[1] = 1;
		BY(o)[2] = 0;
		BY(o)[3] = 2;
		BY(o)[4] = 0;
		break;
	case VTP_CODE_SUBSET:
		WRITE_DWORD_NO(o, g_vtp.getRevision());
		memcpy(BY(o+4), pVlanInfo->pData, pVlanInfo->nLen);
		break;
	case VTP_CODE_REQUEST:	
		WRITE_DWORD_NO(o, 0); // XXX: start
		break;
	}


	return true;
}
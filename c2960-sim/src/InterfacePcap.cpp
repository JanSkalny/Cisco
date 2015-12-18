#include "stdafx.h"

#include "Frame.h"
#include "InterfacePcap.h"

#ifdef WIN32
#include <Win32-Extensions.h>
#endif

InterfacePcap::InterfacePcap(const char *sName, MAC addr, const char *sDevice)
	:Interface(sName, addr)
{
	m_sDeviceName = strdup(sDevice);
	m_pcap = 0;
}


InterfacePcap::~InterfacePcap(void)
{
	free(m_sDeviceName);
	shutdown();
}

bool InterfacePcap::open()
{
	char err[PCAP_ERRBUF_SIZE]; 

	Interface::open();

	printf("InterfacePcap::open on %s (%s)\n", m_sName, m_sDeviceName);

	// initial cleanup
	memset(err, 0, sizeof(err));
	if (m_pcap != NULL)
		shutdown();

	// open capture source
	m_pcap = pcap_open_live(m_sDeviceName, Frame::MAX_LEN, 1, 0, err);
	if (err[0] != 0 || !m_pcap) {
		printf("pcap_open_live\n");
		return false;
	}

	// set non-blocking mode
	if (pcap_setnonblock(m_pcap, 1, err) == -1) {
		printf("pcap_setnonblock\n");
		return false;
	}

	//XXX: not implemented
	//if (pcap_setdirection (m_pcap, PCAP_D_IN) == -1) {
	//	printf("pcap_setdirection not supported\n");
	//}

	// set minimum read size (events) -- ziadne cakanie v kernely
	if (pcap_setmintocopy(m_pcap, 0) == -1) {
		printf("pcap_setmintocopy failed\n");
		return false;
	}

	m_bLineState = true;
	m_bProtoState = true;

	onUp();

	return true;
}

bool InterfacePcap::shutdown()
{
	Interface::shutdown();

	if (m_pcap)
		pcap_close(m_pcap);
	m_pcap = 0;

	return true;
}

bool InterfacePcap::send(Frame* pFrame)
{
	BYTE * pData;
	int nLen;

	if (!pFrame)
		return false;

	if (!m_pcap)
		return false;
	
	pData = pFrame->getData();
	nLen = pFrame->getLen();

#ifdef DEBUG_PACKET
	printf(" - via %s (%d)\n", 
		getName(),
		pFrame->vlan
		);
#endif

	if (isTrunk() && pFrame->vlan != getNativeVlan()) {
		pData = encapsulateDot1q(pFrame);
		nLen = pFrame->getLen() + 4;
	} else {
		pData = pFrame->getData();
		nLen = pFrame->getLen();
	}

	//TODO: XXX:
#if 0
	int i;
	for (i=0; i!=nLen; i++) 
		assert(pData[i] != 0xcd);
#endif

	if (pcap_sendpacket(m_pcap, pData, nLen)) {
		pcap_geterr (m_pcap);
		return false;
	}

	markDup(pData, nLen);

	return true;
}

Frame* InterfacePcap::recv()
{
	Frame *pFrame;
	struct pcap_pkthdr *header = 0;
	BYTE *pData = 0;

	if (!m_pcap)
		return 0;

	if (!pcap_next_ex(m_pcap, &header, (const u_char**)&pData))
		return 0;	// TODO: recv err, fail, neprijali sme data
	if (!pData || !header)
		return 0;	// TODO: runt fail, pcap nam dal vtakovinu
	if (header->caplen != header->len)
		return 0;	// TODO: fail, rovnako sa nam nepacia nekompletne ramce

	// duplikovane ramce nechceme
	if (isDup(pData, header->len)) 
		return 0;
	
	// ak je vsetko ok, vytvorime packet
	pFrame = new Frame(header, pData, this);

	return Interface::onRecv(pFrame);
}

#ifdef WIN32
HANDLE InterfacePcap::getEvent() {
	if (!m_bShutdown && m_pcap)
		return pcap_getevent(m_pcap);
	return 0;
}
#endif

void InterfacePcap::markDup(BYTE *pData, int nLen)
{
	BYTE *pID;

	if (nLen > DUP_CHK_SIZE)
		nLen = DUP_CHK_SIZE;

	pID = (unsigned char*) malloc(DUP_CHK_SIZE);
	memcpy(pID, pData, nLen);

	m_dupHistory.push_back(pID);
}

bool InterfacePcap::isDup(BYTE* pData, int nLen)
{
	bool bDup = false;
	DupHistory::iterator it, itd;

	if (nLen > DUP_CHK_SIZE)
		nLen = DUP_CHK_SIZE;

	// nemame s cim porovnat, zarucene novy
	if (!m_dupHistory.size())
		return false;

	// najdeme si v historii iterator kde sa nachadza dany message
	for (it=m_dupHistory.begin(); it!=m_dupHistory.end(); ++it) {
		if (memcmp(pData, *it, nLen) == 0) {
			bDup = true;
			break;
		}
	}

	if (bDup) {
		// odstranime z historie vsetko az po najdeny ramec
		for (itd=m_dupHistory.begin(); itd!=it; itd++)
			free(*itd);
		m_dupHistory.erase(m_dupHistory.begin(), it);
	}

	return bDup;
}

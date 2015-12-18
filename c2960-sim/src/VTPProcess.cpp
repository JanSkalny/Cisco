#include "stdafx.h"

#include "Vlan.h"
#include "Interface.h"
#include "Line.h"
#include "Core.h"
#include "InterfaceMgr.h"
#include "VTPFrame.h"
#include "VlanMgr.h"
#include "net_conversion.h"
#include "contrib\\md5.h"

#include "VTPProcess.h"

VTPProcess::VTPProcess(void)
{
	m_tLastRequestSend = 0;
	m_tLastSummarySend = 0;
	m_mode = VTP_MODE_SERVER;
	m_bActiveRequest = false;
	m_dwRevision = 0;
	m_tLastTimeout = 0;
	memset(m_sDomain, 0, sizeof(m_sDomain));
	memset(m_sSecretMD5, 0, sizeof(m_sSecretMD5));

	m_dwStatsSummaryRx = m_dwStatsSummaryTx = 0;
	m_dwStatsRequestRx = m_dwStatsRequestTx = 0;
	m_dwStatsSubsetRx = m_dwStatsSubsetTx = 0;
}

VTPProcess::~VTPProcess(void)
{
	deleteVlanInfoBlocks();
}

int VTPProcess::getNextEventTimeout() 
{
	time_t tSleep;

	// kazdych 5 sekund nieco urobime!
	tSleep = 5 + (m_tLastTimeout - g_tNow);
	if (tSleep < 0)
		return 0; // now!

	return (int)tSleep;
}

bool VTPProcess::onTimeout()
{
	NeighborMap::iterator it;
	Neighbor *pNeighbor;

	m_tLastTimeout = g_tNow;

	// age starych susedov
	for (it=m_neighbors.begin(); it!=m_neighbors.end(); ) {
		pNeighbor = it->second;
		if (g_tNow - pNeighbor->tLastSeen > VTP_NEIGHBOR_TIMEOUT) {
			delete (pNeighbor);
			it = m_neighbors.erase(it);
		} else {
			++it;
		}
	}
		
	if (!m_sDomain[0])
		return true;

	switch (m_mode) {
	case VTP_MODE_CLIENT:
		if (m_bActiveRequest && g_tNow - m_tLastRequestSend > VTP_REQUEST_TIMER) 
			sendRequest();
	case VTP_MODE_SERVER:
		//XXX: summary posiela aj klient!
		if (g_tNow - m_tLastSummarySend > VTP_SUMMARY_TIMER)
			sendSummary();
		break;
	}

	return true;
}

bool VTPProcess::onRecv(Frame *pFrame)
{
	VTPFrame *pVTPFrame;
	Neighbor *pNeighbor;

	if (pFrame->wEtherType != ETHERTYPE_VTP) 
		return true;

	printf("* VTP got packet on %s\n", pFrame->pIfOrigin->getName());

	pNeighbor = getNeighbor(pFrame->macSA);
	pNeighbor->tLastSeen = g_tNow;

	pVTPFrame = new VTPFrame(*pFrame, &(pNeighbor->vlans));

	// ak je nasa domena NULL, naucme sa susedovu!
	if (!m_sDomain[0]) 
		setDomain(pVTPFrame->sDomainName);

	// ak nesedi VTP domena -- zozerme packet a nic!
	if (strcmp(m_sDomain, pVTPFrame->sDomainName) != 0)
		return false;

	// ak sme transparentny -- preposlime XXX: TODO: len cez trunky
	if (m_mode == VTP_MODE_TRANSPARENT)
		return true;

	switch (pVTPFrame->yCode) {
	case VTPFrame::VTP_CODE_SUMMARY:
		m_dwStatsRequestRx++;
		if (pVTPFrame->ySeq == 0) {
			// sused hovori co ma, ma viac?
			if (pVTPFrame->dwRevision > m_dwRevision)
				sendRequest();
		} else {
			// sused sa nam snazi natlacit update, ma viac?
			if (pVTPFrame->dwRevision < m_dwRevision) 
				return true; // nic nebude!
			// od suseda cakajme mega-update
			pNeighbor->nFollowers = pVTPFrame->ySeq;
			pNeighbor->yExpectedSeq = 1;
			m_dwUpdaterAddr = pVTPFrame->dwUpdater;
			memcpy(m_sUpdateTS, pVTPFrame->sUpdateTs, sizeof(m_sUpdateTS));
		}
		break;
	case VTPFrame::VTP_CODE_SUBSET:
		m_dwStatsSubsetRx++;
		// sused posiela mega-update
		if (pVTPFrame->ySeq != pNeighbor->yExpectedSeq) {
			// fail! stratil si ramec voe! request
			sendRequest(pVTPFrame->pIfOrigin);
		} else {
			// XXX: nic nerobie, udaje spracoval parser
			// uz doslo vsetko?
			if ((pNeighbor->yExpectedSeq++ >= pNeighbor->nFollowers) &&
				(pVTPFrame->dwRevision >= m_dwRevision)) {
				// po dokladnom overeni preplacnime nas ,,vlan.dat'' tym co poslal sused 
				// naucime sa novy revision, prepocitame info a dame vsetkym vediet ze vieme viac!
				// LEAVE NO SURVIVORS!
				g_vlans.replace(&(pNeighbor->vlans));
				m_dwRevision = pVTPFrame->dwRevision;
				updateVlanInfoBlocks();
				sendSummary();
			}
		}
		break;
	case VTPFrame::VTP_CODE_REQUEST:
		m_dwStatsRequestRx++;
		// sused chce nieco odnas? poslime jeho smerom plny update
		sendFullUpdate(pFrame->pIfOrigin);
		break;
	}

	return false;
}

bool VTPProcess::onUp(Interface *pInterface) {
	// globalny process
	// pri upnuti rozhrania poslime spravu!
	sendSummary(pInterface);

	return true;
}

void VTPProcess::setMode(VTPMode mode) 
{
	m_mode = mode;

	g_switch.updateEventList();

	// ak sme sa stali klientom, zacnime aktivne vyhladavat vtp server
	if (mode == VTP_MODE_CLIENT)
		m_bActiveRequest = true;
}

void VTPProcess::setDomain(char *sName) {
	int nLen;
	nLen = strlen(sName);
	if (nLen > VTPFrame::MAX_DOMAIN_NAME_LEN)
		nLen = VTPFrame::MAX_DOMAIN_NAME_LEN;
	memcpy(m_sDomain, sName, nLen);
	updateVlanInfoBlocks();
	sendSummary();
}

void VTPProcess::onVlanModified() {
	m_dwRevision++;
	updateVlanInfoBlocks();
	sendSummary();
}

void VTPProcess::sendSummary()
{
	InterfaceList::iterator it;
	InterfaceList *pInterfaces = g_interfaces.getList();

	for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it) 
		sendSummary(*it);
	
	m_tLastSummarySend = g_tNow;
}

void VTPProcess::sendSummary(Interface *pInterface) {
	VTPFrame *pFrame;

	if (!m_sDomain[0])
		return;

	if (!pInterface->isTrunk() || !pInterface->isUp()) 
		return;

	m_dwStatsSummaryTx++;

	printf("* VTP send summary via %s\n", pInterface->getName());

	pFrame = new VTPFrame();

	pFrame->macSA = pInterface->getHwAddr();
	pFrame->yCode = VTPFrame::VTP_CODE_SUMMARY;
	pFrame->create(0);

	pInterface->send(pFrame);

	delete pFrame;
}

void VTPProcess::sendRequest()
{
	InterfaceList::iterator it;
	InterfaceList *pInterfaces = g_interfaces.getList();

	for (it=pInterfaces->begin(); it!=pInterfaces->end(); ++it)
		sendRequest(*it);

	m_tLastRequestSend = g_tNow;
}

	
void VTPProcess::sendRequest(Interface *pInterface) 
{		
	VTPFrame *pFrame;

	if (!m_sDomain[0])
		return;
		
	if (!pInterface->isTrunk() || !pInterface->isUp()) 
		return;

	m_dwStatsRequestTx++;

	printf("* VTP send request via %s\n", pInterface->getName());

	pFrame = new VTPFrame();

	pFrame->macSA = pInterface->getHwAddr();
	pFrame->yCode = VTPFrame::VTP_CODE_REQUEST;
	pFrame->create(0);
		
	pInterface->send(pFrame);

	delete pFrame;
}

void VTPProcess::sendFullUpdate(Interface *pInterface)
{
	DataBlockList::iterator it;
	DataBlock *pBlock;
	VTPFrame *pFrame;
	int i=0;
	
	if (!m_sDomain[0])
		return;

	if (!pInterface->isTrunk() || !pInterface->isUp()) 
		return;

	pFrame = new VTPFrame();

	pFrame->macSA = pInterface->getHwAddr();
	pFrame->yCode = VTPFrame::VTP_CODE_SUMMARY;
	pFrame->ySeq = 1;
	pFrame->create(0);

	m_dwStatsSummaryTx++;
	printf("* VTP send full update (summary) via %s\n", pInterface->getName());
	pInterface->send(pFrame);

	delete pFrame;

	it = m_vlanInfoBlocks.begin();
	it++;
	for (; it!=m_vlanInfoBlocks.end(); ++it) {
		pBlock = *it;

		pFrame = new VTPFrame();

		pFrame->macSA = pInterface->getHwAddr();
		pFrame->yCode = VTPFrame::VTP_CODE_SUBSET;
		pFrame->ySeq = ++i;
		pFrame->pVlanInfo = pBlock;
		pFrame->create(0);

		m_dwStatsSubsetTx++;
		printf("* VTP send full update (advert-%d) via %s\n", i, pInterface->getName());
		pInterface->send(pFrame);

		delete pFrame;
	}
}

VTPProcess::Neighbor *VTPProcess::getNeighbor(MAC addr) {
	if (!m_neighbors.count(addr))
		m_neighbors[addr] = new Neighbor(addr);
	return m_neighbors[addr];
}


void VTPProcess::deleteVlanInfoBlocks()
{
	DataBlockList::iterator it;
	
	for (it=m_vlanInfoBlocks.begin(); it!=m_vlanInfoBlocks.end(); ++it)
		delete (*it);
	m_vlanInfoBlocks.clear();
}

void VTPProcess::updateVlanInfoBlocks()
{
	DataBlockList::iterator itb;
	DataBlock *pBlock;
	VlanMap::iterator it;
	VlanMap *pVlans = g_vlans.getMap();
	Vlan *pVlan;
	int nLen, nNameLen;
	BYTE yDomainLen;
	md5_state_t mds;

	// zrusme stary obsah
	deleteVlanInfoBlocks();

	// prvy blok je sucast VTP hlavicky
	pBlock = new DataBlock(72);
	
	USE_NET_CONVERSION(pBlock->pData);

	if (!m_sDomain[0])
		return;

	yDomainLen = strlen(m_sDomain);
	if (yDomainLen > VTPFrame::MAX_DOMAIN_NAME_LEN)
		yDomainLen = VTPFrame::MAX_DOMAIN_NAME_LEN;

	WRITE_BYTE(0, 1);
	WRITE_BYTE(1, 1);
	WRITE_BYTE(3, yDomainLen);
	memcpy(BY(4), m_sDomain, yDomainLen);
	WRITE_DWORD_NO(4+32, m_dwRevision);
	
	m_vlanInfoBlocks.push_back(pBlock);

	// odhadneme, kolko budeme potrebovat na vsetky nase vlan-y
	nLen = 0;
	for (it=pVlans->begin(); it!=pVlans->end(); ++it) {
		pVlan = it->second;
		nNameLen = strlen(pVlan->sName);
		nLen += 12 + ((int)(nNameLen / 4) + (nNameLen%4 ? 1 : 0)) * 4;
		if (nLen > 1300) {
			createVlanInfoBlock(nLen, it);
			nLen = 0;
		}
	}
	if (nLen)
		createVlanInfoBlock(nLen, it);
	
	md5_init(&mds);
	md5_append(&mds, m_sSecretMD5, sizeof(m_sSecretMD5));
	for (itb=m_vlanInfoBlocks.begin(); itb!=m_vlanInfoBlocks.end(); ++itb) {
		pBlock = *itb;
		md5_append(&mds, pBlock->pData, pBlock->nLen);
	}
	md5_append(&mds, m_sSecretMD5, sizeof(m_sSecretMD5));
	md5_finish(&mds, m_sDigest); 
}


void VTPProcess::createVlanInfoBlock(int nLen, VlanMap::iterator &it)
{
	VlanMap::iterator itx;
	VlanMap *pVlans = g_vlans.getMap();
	Vlan *pVlan;
	BYTE yVlanLen=0, nNameLen;
	int o;
	DataBlock *pBlock;

	pBlock = new DataBlock(nLen);

	USE_NET_CONVERSION(pBlock->pData);

	o=0;
	for (itx=pVlans->begin(); itx!=it; ++itx) {
		pVlan = itx->second;
		nNameLen = strlen(pVlan->sName);
		yVlanLen = 12 + ((int)(nNameLen / 4) + (nNameLen%4 ? 1 : 0)) * 4;

		WRITE_BYTE(o, yVlanLen);
		WRITE_BYTE(o+1, pVlan->bActive ? 0 : 1);
		WRITE_BYTE(o+2, pVlan->yType); // ethernet
		WRITE_BYTE(o+3, strlen(pVlan->sName));
		WRITE_WORD_NO(o+4, pVlan->id);
		WRITE_WORD_NO(o+6, 1500); // mtu
		WRITE_DWORD_NO(o+8, pVlan->dwDot10Index); // 802.10 index TODO:
		memcpy(BY(o+12), pVlan->sName, strlen(pVlan->sName));

		o+= yVlanLen;
	}

	m_vlanInfoBlocks.push_back(pBlock);
}

void VTPProcess::registerCommands() {
	g_commands.registerCommand(MODE_EXEC, 1, "show vtp", ";VTP information");
	g_commands.registerCommand(MODE_EXEC, 1, "show vtp status", ";;VTP domain status", VTPProcess::cmdShowVTPStatus);
	g_commands.registerCommand(MODE_EXEC, 1, "show vtp counter", ";;VTP statistics", VTPProcess::cmdShowVTPCounter);

	g_commands.registerCommand(MODE_CONF, 15, "vtp", "Configure global VTP state");
	g_commands.registerCommand(MODE_CONF, 15, "no vtp", ";Configure global VTP state");
	g_commands.registerCommand(MODE_CONF, 15, "vtp mode client", ";Configure VTP device mode;Set the device to client mode.", VTPProcess::cmdVTPModeClient);
	g_commands.registerCommand(MODE_CONF, 15, "vtp mode server", ";;Set the device to server mode.", VTPProcess::cmdVTPModeServer);
	g_commands.registerCommand(MODE_CONF, 15, "vtp mode transparent", ";;Set the device to transparent mode.", VTPProcess::cmdVTPModeTransparent);
	g_commands.registerCommand(MODE_CONF, 15, "no vtp mode", ";;Configure VTP device mode", VTPProcess::cmdVTPModeServer);
	g_commands.registerCommand(MODE_CONF, 15, "vtp domain WORD", ";Set the name of the VTP administrative domain.;The ascii name for the VTP administrative domain.", VTPProcess::cmdVTPDomain);
	g_commands.registerCommand(MODE_CONF, 15, "vtp password WORD", ";Set the password for the VTP administrative domain;The ascii password for the VTP administrative domain.", VTPProcess::cmdVTPPassword);
	g_commands.registerCommand(MODE_CONF, 15, "no vtp password", ";;Set the password for the VTP administrative domain", VTPProcess::cmdVTPNoPassword);
	g_commands.registerCommand(MODE_CONF, 15, "vtp version <1-1>", ";Set the adminstrative domain to VTP version;Set the adminstrative domain VTP version number", VTPProcess::cmdVTPVersion);
	g_commands.registerCommand(MODE_CONF, 15, "no vtp version", ";;Set the adminstrative domain VTP version number", VTPProcess::cmdVTPVersion);
}
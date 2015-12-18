#pragma once

#include "interface.h"
#include <pcap.h>

class InterfacePcap :
	public Interface
{
protected:
	pcap_t *m_pcap;
	char *m_sDeviceName;


	// detelcia zaloopovanych ramcov
	typedef enum { DUP_CHK_SIZE = 20 };
	typedef list<BYTE*> DupHistory;

	DupHistory m_dupHistory;
	bool isDup(BYTE* pData, int nLen);
	void markDup(BYTE* pData, int nLen);

public:
	InterfacePcap(const char *sName, MAC addr, const char *sDevice);
	~InterfacePcap(void);

#ifdef WIN32
	virtual HANDLE getEvent();
#else
	int getFD();
#endif

	bool send(Frame* frame);
	Frame* recv();

	bool open();
	bool shutdown();
};


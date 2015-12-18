#pragma once

#include "Worker.h"

class LineTelnet;

class LineTelnetClient : public Worker {

	int m_nSocket;
	bool m_bSendUpdates;
	LineTelnet* m_pParent;

public:
	LineTelnetClient(int sock, bool bSendUpdates, LineTelnet* pParent=NULL);
	virtual ~LineTelnetClient();

	void destroy();

	bool write(char *sBuf, int nLen);
	bool write(char *sFormat, ...);
	int work();
};


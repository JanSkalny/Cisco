#pragma once

#include "Worker.h"
#include "Line.h"

class LineTelnetClient;

class LineTelnet : public Worker, public Line {
	typedef list<LineTelnetClient*> ClientList;

protected:
	bool m_bSharedMode;
	int m_nPort;
	int m_nServerSocket;

	ClientList m_clients;

public:
	LineTelnet(int nPort, bool bSharedMode=true);
	virtual ~LineTelnet();

	bool init();
	void destroy();
	int work();

	bool addClient(int sock);

	virtual void writeLen(char *sBuf, int nLen);
	void writeAll(char *sBuf, int nLen, LineTelnetClient *pExceptClient=NULL);
	virtual void onRead(LineTelnetClient *pClient, char c);
};



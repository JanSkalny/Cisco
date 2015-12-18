#include "stdafx.h"

#include "Command.h"
#include "LineTelnet.h"
#include "LineTelnetClient.h"

LineTelnetClient::LineTelnetClient(int sock, bool bSendUpdates, LineTelnet* pParent)
{
	m_nSocket = sock;
	m_bSendUpdates = bSendUpdates;
	m_pParent = pParent;
}

LineTelnetClient::~LineTelnetClient()
{
	destroy();
}

void LineTelnetClient::destroy()
{
	if (m_nSocket) {
		closesocket(m_nSocket);
		m_nSocket = 0;
	}
}

bool LineTelnetClient::write(char *sFormat, ...)
{
	va_list args;
	char sBuf[1025];
	sBuf[1024] = 0;

	va_start(args, sFormat);
	vsnprintf(sBuf, 1024, sFormat, args);
	va_end(args);

	return write(sBuf, strlen(sBuf));
}

bool LineTelnetClient::write(char *sBuf, int nLen) 
{
	if (send(m_nSocket, sBuf, nLen, 0) != nLen) {
		perror("send failed");
		return false;
	}
	return true;
}

int LineTelnetClient::work()
{
	char c;

	while (1) {
		if (recv(m_nSocket, &c, 1, 0) < 1) {
			perror("recv failed");
			destroy();
			return -1;
		}

		m_pParent->onRead(this, c);
	}

	return 0;
}

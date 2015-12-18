#include "stdafx.h"

#include "Command.h"
#include "LineTelnetClient.h"
#include "LineTelnet.h"

LineTelnet::LineTelnet(int nPort, bool bSharedMode)
{
	m_nServerSocket = 0;
	m_bSharedMode = bSharedMode;
	m_nPort = nPort;
}

LineTelnet::~LineTelnet()
{
	destroy();
}

int LineTelnet::work()
{
	int sock;

	// pripravime si server
	if (!init())
		return -1;

	// pre kazdeho, kto sa pripoji, vytvorime novu zdielanu konzolovu linku line
	while (1) {
		if ((sock = accept(m_nServerSocket, NULL, NULL)) == -1) {
			perror("accept failed");
			return -1;
		}

		addClient(sock);
	}
	
	return 0;
}

bool LineTelnet::init()
{
	char one=1;
	struct sockaddr_in addr;

	// pripravime si tcp server
	if ((m_nServerSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket failed");
		return false;
	}

	if (setsockopt(m_nServerSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
		perror("setsockopt(SO_REUSEADDR) failed");
		return false;
	}
	if (setsockopt(m_nServerSocket, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one)) == -1) {
		perror("setsockopt(SO_KEEPALIVE)");
		return false;
	}
	if (setsockopt(m_nServerSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1) {
		perror("setsockopt(TCP_NODELAY)");
		return false;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(m_nPort);

	if (bind(m_nServerSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind failed");
		return false;
	}

	if (listen(m_nServerSocket, SOMAXCONN) == -1) {
		perror("listen failed");
		return false;
	}

	return true;
}

void LineTelnet::destroy()
{
	LineTelnetClient *pClient;

	if (m_nServerSocket) {
		closesocket(m_nServerSocket);
		m_nServerSocket = 0;
	}

	while (!m_clients.empty()) {
		pClient = m_clients.front();
		m_clients.pop_front();

		delete pClient;
	}
}

bool LineTelnet::addClient(int sock)
{
	LineTelnetClient *pClient;
	BYTE yTelnetCmd[] = { 
		IAC, DO, TELOPT_TTYPE,
		IAC, WILL, TELOPT_ECHO,
		IAC, WILL, TELOPT_SGA,
		IAC, DONT, TELOPT_LINEMODE,
   	};	

	printf("add new client sock=%d\n", sock);
	pClient = new LineTelnetClient(sock, m_bSharedMode, this);
	m_clients.push_back(pClient);
	pClient->start();
	pClient->write((char*)yTelnetCmd, sizeof(yTelnetCmd));
	pClient->write("Connected to virtual switch c2960 - Console port\r\nPress ENTER to get the prompt.\r\n");

	return true;
}

void LineTelnet::onRead(LineTelnetClient *pClient, char c)
{

	//TODO: odfiltrovat specialne znaky? (ctrl+shift+6, etc)
	//writeAll(sBuf, nLen, pClient);
	//writeAll(&c, 1);

	Line::onRead(c);
}

void LineTelnet::writeAll(char *sBuf, int nLen, LineTelnetClient *pExceptClient)
{
	ClientList::iterator it;
	int len = (nLen > 16) ? 16 : nLen;
	for (it=m_clients.begin(); it!=m_clients.end(); ++it) {
		if (*it == pExceptClient) 
			continue;
		/*
		printf("write 0x");
		for (i=0; i!=len; i++)
			printf("%02x", (unsigned char)*(sBuf+i));
		printf("(len=%d) to %p\n", nLen, (*it));
		*/
		(*it)->write(sBuf, nLen);
	}
}

void LineTelnet::writeLen(char *sBuf, int nLen)
{
	writeAll(sBuf, nLen, NULL);
}


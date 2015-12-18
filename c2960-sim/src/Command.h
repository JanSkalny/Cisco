#pragma once

class Interface;

#include "ArgumentList.h"

class Command {
protected:
	

public:
	enum { MAX_LEN = 1024 };

	Command();
	virtual ~Command();

	inline void empty() {
		m_nPos = m_nLen = 0; 
		memset(m_sBuf, 0, MAX_LEN+1);
		arguments.empty();
	}

	ArgumentList arguments;

	inline void set(char *sCommand) {
		m_nPos = m_nLen = strlen(sCommand);
		if (m_nLen > MAX_LEN)
			m_nPos = m_nLen = strlen(sCommand);

		memcpy(m_sBuf, sCommand, m_nLen);
	}
	inline int getPos() { return m_nPos; }
	inline int getLen() { return m_nLen; }
	inline int getTailLen() { return m_nLen - m_nPos; }
	inline char *getTail() { return m_sBuf + m_nPos; }
	inline char *get() { return m_sBuf; }
	inline void start() { m_nPos = 0; }
	inline void end() { m_nPos = m_nLen; }
	inline bool left() { 
		if (isAtStart())
			return false;
		m_nPos--; 
		return true;
	}
	inline bool right() {
		if (isAtEnd())
			return false;
		m_nPos++;
		return true;
	}
	inline bool isAtEnd() { return (m_nPos == m_nLen) ? true : false; } 
	inline bool isAtStart() { return (m_nPos == 0) ? true : false; } 
	
	void push(char c) {
		int i;
		if (m_nLen >= MAX_LEN)
			return;
		for (i=m_nLen; i>=m_nPos; i--) 
			m_sBuf[i+1] = m_sBuf[i];
		m_nLen++;
		m_sBuf[m_nPos++] = c;
	}
		
	void popRight() {
		int i;
		if (m_nPos == m_nLen)
			return;
		for (i=m_nPos; i<=m_nLen; i++) 
			m_sBuf[i] = m_sBuf[i+1];
		m_nLen--;
		printf("after-pr: %s\n", m_sBuf);
	}

	void popLeft() {
		int i;
		if (m_nPos == 0)
			return;
		m_nPos--;
		for (i=m_nPos; i<=m_nLen; i++) 
			m_sBuf[i] = m_sBuf[i+1];
		m_nLen--;
		printf("after-pl: %s\n", m_sBuf);
	}

	char * getPart() {
		int nLen;
		char *sBuf;

		sBuf = m_sBuf;
		while(*sBuf == ' ')
			sBuf++;

		m_sNextPart = strchr(sBuf, ' ');
		nLen = (m_sNextPart == 0) ? strlen(sBuf) : m_sNextPart - sBuf;
		memcpy(m_sPartBuf, sBuf, nLen);
		m_sPartBuf[nLen] = 0;

		if (m_sNextPart) {
			while (*m_sNextPart == ' ')
				m_sNextPart++;
			if (*m_sNextPart == 0)
				m_sNextPart = 0;
		}

		return m_sPartBuf;
	}

	char * getNextPart() {
		int nLen;
		char *sBuf;

		if (!m_sNextPart)
			return 0;

		sBuf = m_sNextPart;
		m_sNextPart = strchr(sBuf, ' ');
		nLen = (m_sNextPart == 0) ? strlen(sBuf) : m_sNextPart - sBuf;
		memcpy(m_sPartBuf, sBuf, nLen);
		m_sPartBuf[nLen] = 0;

		if (m_sNextPart) {
			while (*m_sNextPart == ' ')
				m_sNextPart++;
			if (*m_sNextPart == 0)
				m_sNextPart = 0;
		}

		return m_sPartBuf;
	}

	bool hasNextPart() {
		return m_sNextPart ? true : false;
	}

protected:
	char m_sPartBuf[MAX_LEN], *m_sNextPart;
	char *m_sBuf;
	int m_nPos;
	int m_nLen;
};


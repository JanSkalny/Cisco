#pragma once

typedef enum {
	MODE_EXEC,
	MODE_CONF,
	MODE_CONF_LINE,
	MODE_CONF_IF,
	MODE_CONF_VLAN,
	MODE_CONF_VTP,
} LineMode;

#include "Command.h"

class Line {
	typedef enum { INPUT_MODE_VT100=100, INPUT_MODE_VT220=220, INPUT_MODE_TELNET=1, INPUT_MODE_TELNET_OPTION=2, INPUT_MODE_TEXT=10 } InputMode;

	class History {
		list<char*> m_commands;
		int m_nMaxSize;
		int m_nPos;

	public:
		History() {
			m_nPos = 0;
			m_nMaxSize = 5;
		}

		void add(char* sCommand) {
			char *p;

			if (strlen(sCommand) == 0)
				return;

			if (m_nMaxSize == 0)
				return;

			if (m_commands.size() == m_nMaxSize) {
				p = m_commands.back();
				if (p) 
					free(p);
				m_commands.pop_back();
			}

			m_commands.push_front(strdup(sCommand));
			m_nPos = 0;
		}
		char *getNewer() {
			if (m_nPos == 0)
				return 0;
			printf("history get newer - %d\n", m_nPos-1);

			return getAt(--m_nPos);
		}

		char *getOlder() {
			if (m_nPos == m_commands.size())
				return 0;
			printf("history get older - %d\n", m_nPos+1);
			
			return getAt(++m_nPos);
		}

		char * getAt(int pos) {
			list<char*>::iterator it;
			int i;

			if (pos == 0)
				return 0;

			it = m_commands.begin();
			for (i=1; i<pos; i++)
				++it;

			return *it;
		}

	};

protected:
	Command m_command;
	LineMode m_mode;
	void *m_pModeData;
	int m_nPrivilegeLevel;

	bool m_bLoggingSynchronous;
	bool getLoggingSynchronous() { return m_bLoggingSynchronous; }
	void setLoggingSynchronous(bool bVal) { m_bLoggingSynchronous = bVal; }

	InputMode m_inputMode;
	unsigned char m_cInputTelnetRequest;
	
	History m_history;

public:
	Line();

	LineMode getMode() { return m_mode; };
	void* getModeData() { return m_pModeData; };
	void setMode(LineMode mode, void *pData=0);

	void write(char *sFormat, ...);
	virtual void writeLen(char *sBuf, int nLen) = 0;
	virtual void onRead(unsigned char c);

	void setPrivilegeLevel(int nLevel) { m_nPrivilegeLevel = nLevel; };
	int getPrivilegeLevel() { return m_nPrivilegeLevel; };

	void syslog(char *sMsg, ...);

	void eraseCommand();
	void eraseCommandTail();
	void rewriteCommand();
	void rewriteCommandTail();
	void writeNewLine();
	void writeMode();

	static void registerCommands();

	CMD(cmdLineCON0) {
		pLine->setMode(MODE_CONF_LINE);
	}
	CMD(cmdLineLoggingSync) {
		pLine->setLoggingSynchronous(true);
	}
	CMD(cmdLineNoLoggingSync) {
		pLine->setLoggingSynchronous(false);
	}
	CMD(cmdLineExit) {
		pLine->setMode(MODE_CONF);
	}
};


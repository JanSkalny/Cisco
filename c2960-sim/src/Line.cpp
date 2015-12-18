#include "stdafx.h"

#include "Interface.h"
#include "InterfaceMgr.h"

#include "Command.h"
#include "Line.h"
#include "CommandTree.h"
#include "Vlan.h"
#include "VlanMgr.h"

extern CommandTree g_commands;
extern VlanMgr g_vlans;

void Line::writeMode()
{
	// hostname 
	write("SW");

	// mode
	switch(getMode()) {
	case MODE_CONF:			write("(config)");		break;
	case MODE_CONF_IF:		write("(config-if)");	break;
	case MODE_CONF_LINE:	write("(config-line)");	break;
	case MODE_CONF_VLAN:	write("(config-vlan)");	break;
	}

	// level
	if (getPrivilegeLevel() <= 1) {
		write(">");
	} else {
		write("#");
	}
}

void Line::onRead(unsigned char c)
{
	int n;
	char *sBuf;

	switch (m_inputMode) {
	case INPUT_MODE_TEXT:
		switch (c) {
		case IAC: // TELNET (IAC 0xff)
			m_inputMode = INPUT_MODE_TELNET;
			return;

		case 0:
		case '\n':	// ignorujeme
			return;
		case '\r':
			if (m_command.getLen() > 0) {
				// spracovat prikaz
				m_history.add(m_command.get());
				g_commands.exec(this, &m_command);
				m_command.empty();
			}
			writeNewLine();
			return;

		case 0x03:	// CTRL+c
			if (getMode() != MODE_EXEC) {
				setMode(MODE_EXEC);
				//TODO: apply configuration
			}
			m_command.empty();
			writeNewLine();
			break;

		case 0x1b:	// VT-100 (ESC)
			m_inputMode = INPUT_MODE_VT100;
			return;

		case 0x01:	// CTRL+a
			n = m_command.getPos();
			while (n-- > 0) write("\x08"); // BS
			m_command.start();
			break;

		case 0x05: // CTRL+e
			write(m_command.getTail());
			m_command.end();
			break;

		case 0x08: // BS
		case 0x7f: // DEL (v skutocnosti BS)
			if (!m_command.isAtStart()) {
				m_command.popLeft();
				write("\x08"); //BS
				rewriteCommandTail();
			} else {
				write("\a");
			}
			break;

		case 0x7e: // skutocny DEL
			if (!m_command.isAtEnd()) {
				m_command.popRight();
				rewriteCommandTail();
			} else {
				write("\a");
			}
			break;

		case 0x09: // TAB
			if (!g_commands.expand(this, &m_command))
				write("\a");
			writeNewLine();
			break;

		case '?':
			if (!g_commands.helper(this, &m_command))
				write("\a");
			writeNewLine();
			break;

		default:
			m_command.push(c);
			writeLen((char*)(&c), 1);
			rewriteCommandTail();
		}
		return;

	case INPUT_MODE_TELNET:
		switch (c) {
		case WILL:
		case WONT:
		case DO:
		case DONT:
			m_cInputTelnetRequest = c;
			m_inputMode = INPUT_MODE_TELNET_OPTION;
			return;
		case IAC:
			// TODO: spracovat 0xff
			m_inputMode = INPUT_MODE_TEXT;
			return;
		default:
			printf("neznamy telnet prikaz 0x%02x\n", c);
		}
		m_inputMode = INPUT_MODE_TEXT;
		return;

	case INPUT_MODE_TELNET_OPTION:
		printf("IAC ");

		switch (m_cInputTelnetRequest) {
		case WILL:	printf("WILL "); break;
		case WONT:	printf("WON'T "); break;
		case DO:	printf("DO "); break;
		case DONT:	printf("DON'T "); break;
		default:	printf("%d ", m_cInputTelnetRequest);
		}

		switch (c) {
		case TELOPT_ECHO:	printf("ECHO\n"); break;
		default:			printf("0x%02x\n", c); break;
		}

		m_inputMode = INPUT_MODE_TEXT;
		return;

	case INPUT_MODE_VT100:
		switch (c) {
		case 0x5b:
			m_inputMode = INPUT_MODE_VT220;
			return;
		}
		m_inputMode = INPUT_MODE_TEXT;
		return;

	case INPUT_MODE_VT220:
		switch (c) {
		case 0x41:
			// up
			if(!(sBuf = m_history.getOlder())) {
				write("\a");
			} else {
				eraseCommand();
				m_command.empty();
				m_command.set(sBuf);
				write(sBuf);
			}
			break;
		case 0x42:
			// down
			if(!(sBuf = m_history.getNewer())) {
				write("\a");
				eraseCommand();
				m_command.empty();
			} else {
				eraseCommand();
				m_command.empty();
				m_command.set(sBuf);
				write(sBuf);
			}
			break;

		case 0x43: // right
			if (m_command.right()) {
				write("\x1b\x5b\x43");
			} else {
				write("\x07");
			}
			break;
		case 0x44: // left
			if (m_command.left()) {
				write("\x1b\x5b\x44");
			} else {
				write("\x07");
			}
			break;
		}
		m_inputMode = INPUT_MODE_TEXT;
		return;
	}
}


void Line::eraseCommand() {
	int n;

	eraseCommandTail();

	n = m_command.getPos();
	while (n-- > 0) write("\x08");	// BS
	n = m_command.getPos() + 1;
	while (n-- > 0) write(" ");		// SPACE
	n = m_command.getPos() + 1;
	while (n-- > 0) write("\x08");	// BS
}

void Line::eraseCommandTail() {
	int n;

	n = m_command.getTailLen() + 1;
	while (n-- > 0) write(" ");		// SPACE
	n = m_command.getTailLen() + 1;
	while (n-- > 0) write("\x08");	// BS
}

void Line::rewriteCommand() {
	int n;

	eraseCommand();
	write(m_command.get(), m_command.getLen());
	n = m_command.getTailLen();
	while (n-- > 0) write("\x08"); // BS
}

void Line::rewriteCommandTail()
{
	int n;

	eraseCommandTail();
	write(m_command.getTail(), m_command.getTailLen());
	n = m_command.getTailLen();
	while (n-- > 0) write("\x08"); // BS
}

void Line::writeNewLine() 
{
	write("\r\n");
	writeMode();
	write(m_command.get(), m_command.getLen());
}


void Line::syslog(char *sMsg, ...) 
{
	bool bLogSync;
	va_list args;
	char sBuf[1025];
	sBuf[1024] = 0;

	va_start(args, sMsg);
	vsnprintf(sBuf, 1024, sMsg, args);
	va_end(args);

	bLogSync = getLoggingSynchronous();

	write("\r\n%s", sBuf);

	if (getLoggingSynchronous()) 
		writeNewLine();
}

Line::Line() {
	m_inputMode = INPUT_MODE_TEXT;

	m_mode = MODE_EXEC;
	m_pModeData = 0;
	m_nPrivilegeLevel = 1;
	m_bLoggingSynchronous = false;
}

void Line::write(char *sFormat, ...) 
{
	va_list args;
	char sBuf[1025];
	sBuf[1024] = 0;

	va_start(args, sFormat);
	vsnprintf(sBuf, 1024, sFormat, args);
	va_end(args);

	writeLen(sBuf, strlen(sBuf));
}

void Line::setMode(LineMode mode, void *pData) 
{
	if (m_mode == MODE_CONF_VLAN) 
		g_vlans.commit();
	m_mode = mode;
	m_pModeData = pData; 
};

void Line::registerCommands() {
	g_commands.registerCommand(MODE_CONF, 15, "line console <0>", "Configure a terminal line;Primary terminal line;First line number", cmdLineCON0);
	g_commands.registerCommand(MODE_CONF_LINE, 1, "logging synchronous", "Modify message logging facilities;Synchronized message output", cmdLineLoggingSync);
	g_commands.registerCommand(MODE_CONF_LINE, 1, "no logging synchronous", ";Modify message logging facilities;Synchronized message output", cmdLineNoLoggingSync);
	g_commands.registerCommand(MODE_CONF_LINE, 1, "exit", "Exit from line configuration mode", cmdLineExit);
}

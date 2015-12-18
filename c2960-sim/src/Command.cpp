#include "stdafx.h"

#include "Command.h"

Command::Command() { 
	m_sBuf = new char[MAX_LEN+1];
	empty();
};

Command::~Command() {
	if (m_sBuf) {
		delete[] m_sBuf;
		m_sBuf = 0;
	}
}

#include "stdafx.h"

#include "InterfaceNull.h"
#include "Frame.h"

InterfaceNull::InterfaceNull(const char *sName, MAC addr)
	:Interface(sName, addr)
{
}


InterfaceNull::~InterfaceNull(void)
{
}

bool InterfaceNull::open()
{
	Interface::open();

	m_bLineState = true;
	m_bProtoState = true;

	onUp();

	return true;
}

bool InterfaceNull::send(Frame* frame)
{
	if (!frame)
		return false;

	return true;
}

Frame* InterfaceNull::recv()
{
	return 0;
}
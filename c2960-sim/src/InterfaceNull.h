#pragma once

#include "Interface.h"

class InterfaceNull :
	public Interface
{
public:
	InterfaceNull(const char *sName, MAC addr);
	~InterfaceNull(void);

	bool send(Frame* frame);
	Frame* recv();

	bool open();
};


#pragma once

#include "frame.h"
#include "CDPEntry.h"

class CDPFrame :
	public Frame, public CDPEntry
{
public:
	CDPFrame(void);
	CDPFrame(const Frame& orig) : Frame(orig) { parse(); };
	~CDPFrame(void);

	bool parse();
	bool create(int nPayloadLen);



	BYTE yVer;
	BYTE yTTL;
	WORD wChecksum;
};
#pragma once

#include "frame.h"

class DTPFrame :
	public Frame
{
public:
	DTPFrame(void);
	DTPFrame(const Frame &orig) : Frame (orig) { parse(); };
	~DTPFrame(void);

	BYTE yVer;
	BYTE yTos;	// 0=access 8=trunk
	BYTE yTas;	// 1=trunk 2=access 3=deisre 4=auto
	BYTE yTot;  // ??? a dot1q ?
	BYTE yTat;  // ??? 5 dot1q ? 
	MAC macNeighbor;
	char *sVTPDomain;

	bool parse();
	bool create(int nPayloadLen);
};


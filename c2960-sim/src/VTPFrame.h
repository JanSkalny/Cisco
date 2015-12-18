#pragma once

#include "frame.h"

class Vlan;

class VTPFrame :
	public Frame
{
	VlanMap *m_pVlans;

public:
	enum {
		MAX_DOMAIN_NAME_LEN=32,
		UPDATE_TS_LEN=12,
		MD5_DIGEST_LEN=16,
	};

	typedef enum {
		VTP_CODE_SUMMARY = 1,
		VTP_CODE_SUBSET = 2,
		VTP_CODE_REQUEST = 3,
	} VtpCode;

	VTPFrame(void);
	VTPFrame(const Frame &orig, VlanMap *pVlans) : Frame (orig) { m_pVlans = pVlans; parse();  };
	~VTPFrame(void);

	char sDomainName[MAX_DOMAIN_NAME_LEN+1];
	BYTE yDomainNameLen;
	BYTE yVersion;
	BYTE yCode;
	BYTE ySeq;

	DataBlock *pVlanInfo;

	DWORD dwRevision;
	DWORD dwUpdater;

	char sUpdateTs[UPDATE_TS_LEN+1];
	char sMd5Digest[UPDATE_TS_LEN+1];

	bool parse();
	void parseVlan(int o, int& nlen);
	bool create(int nPayloadLen);
};


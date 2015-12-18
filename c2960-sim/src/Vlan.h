#pragma once

typedef int VlanId;
#define VLAN_ALL -1

typedef map<VlanId, Vlan*> VlanMap;

class VlanData {
public:
	enum {
		MAX_NAME_LEN = 256
	};

	typedef enum {
		VLAN_TYPE_ETHERNET = 1,
		VLAN_TYPE_FDDI = 2,
		VLAN_TYPE_TrCRF = 3,
		VLAN_TYPE_FDDI_NET = 4,
		VLAN_TYPE_TrBRF = 5,
	} VlanType;

	VlanData() {
		dwDot10Index = 100000;
		bActive = true;
		bShutdown = false;
		yType = VLAN_TYPE_ETHERNET; // ETHERNET
	};
	bool bActive;
	bool bShutdown;
	VlanId id;
	char sName[MAX_NAME_LEN];
	BYTE yType;
	DWORD dwDot10Index;
};

class Vlan : public VlanData
{
	VlanData newData;
	
	bool m_bCommited;
	bool m_bReserved;

public:
	Vlan(VlanId id, char *sName=0, bool bReserved=false) : VlanData() {
		this->id = id;
		dwDot10Index = 100000+id;

		if (!sName)
			sprintf(newData.sName, "VLAN%04d", id);
		else
			strncpy(newData.sName, sName, MAX_NAME_LEN);

		m_bReserved = bReserved;
		m_bCommited = false;
	}

	void setName(char *sName) {
		strncpy(newData.sName, sName, VlanData::MAX_NAME_LEN-1);
		m_bCommited = false;
	}
	void setActive(bool bVal) {
		newData.bActive = bVal;
		m_bCommited = false;
	}
	void setShutdown(bool bVal) {
		newData.bShutdown = bVal;
		m_bCommited = false;
	}

	void commit() {
		if (m_bCommited)
			return;

		bActive = newData.bActive;
		bShutdown = newData.bShutdown;
		strncpy(sName, newData.sName, MAX_NAME_LEN-1);
		m_bCommited = true;
	}
		
	bool isCommited() { return m_bCommited; }
	bool isReserved() { return m_bReserved; }
};

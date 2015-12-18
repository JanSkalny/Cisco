#pragma once

typedef enum {
	CDP_DEVICE_ID=1,
	CDP_ADDRESS=2,
	CDP_PORT_ID=3,
	CDP_CAPABILITIES=4,
	CDP_VERSION=5,
	CDP_PLATFORM=6,
	CDP_VTP_DOMAIN=9,
	CDP_NATIVE_VLAN=10,
	CDP_DUPLEX=11,
	CDP_MGMT_ADDRESS=22,
} CDPVarType;

class Interface;

class CDPEntry {
protected:

	class Var {
	public:
			
		typedef enum {
			VAR_STRING,
			VAR_DWORD
		} Type;

		Var(char *sData) {
			this->sData = sData; 
			type = VAR_STRING;
		}
		Var(DWORD dwData) {
			this->dwData = dwData; 
			type = VAR_DWORD;
		}

		char *sData;
		DWORD dwData;
		Type type;
	};

	typedef map<CDPVarType,Var*> VarMap;
	VarMap m_vars;

public:
	CDPEntry() { }
	virtual ~CDPEntry() { empty(); }

	MAC macAddr;
	time_t tLastUpdate;
	Interface *pOrigin;
	BYTE yHoldTime;

	void empty() {
		VarMap::iterator it;

		for (it=m_vars.begin(); it!=m_vars.end(); ++it)
			delete it->second;

		m_vars.clear();
	}
	inline void addVar(CDPVarType type, char *sValue) {
		addVar(type, new Var(sValue));
	}
	inline void addVar(CDPVarType type, DWORD dwValue) {
		addVar(type, new Var(dwValue));
	}
	inline void addVar(CDPVarType type, Var *pVar) {
		if (hasVar(type)) 
			m_vars.erase(type);
		m_vars[type] = pVar;
	}
	inline bool hasVar(CDPVarType type) {
		return m_vars.count(type) > 0 ? true : false;
	}
	inline char *getVarStr(CDPVarType type) {
		return m_vars[type]->sData;
	}
	inline DWORD getVarDWORD(CDPVarType type) {
		return m_vars[type]->dwData;
	}
};
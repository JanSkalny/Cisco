#pragma once

class Interface;

class ArgumentList
{
	class Entry {
	public:
		typedef enum {
			ARGUMENT_INT,
			ARGUMENT_MAC,
			ARGUMENT_INTERFACE,
			ARGUMENT_STRING,
		} ArgumentType;
		
		int m_nValue;
		MAC m_mac;
		Interface *m_pInterface;
		char *m_sValue;
		
		ArgumentType m_type;

	public:
		void init(ArgumentType type) {
			m_type = type;
			m_nValue = 0;
			m_pInterface = 0;
			m_sValue = 0;
			m_mac = 0;
		}

		Entry(int nValue) {
			init(ARGUMENT_INT);
			m_nValue = nValue;
		}
		Entry(Interface *pInterface) {
			init(ARGUMENT_INTERFACE);
			m_pInterface = pInterface;
		}
		Entry(MAC addr) {
			init(ARGUMENT_MAC);
			m_mac = addr;
		}
		Entry(char *sValue) {
			init(ARGUMENT_STRING);
			m_sValue = strdup(sValue);
		}

		~Entry() {
			if (m_sValue)
				free(m_sValue);
		}
	};

	vector<Entry*> m_list;

	void add(Entry* pEntry) {
		m_list.push_back(pEntry);
	}

	Entry *findEntry(int nPos) {
		return m_list[nPos];
	}

public:
	ArgumentList(void);
	~ArgumentList(void);

	void empty() {
		int i, len=m_list.size();

		for (i=0; i!=len; i++) 
			delete m_list[i];

		m_list.clear();
	}

	void add(MAC addr) {
		add(new Entry(addr));
	}
	void add(Interface *pInterface) {
		add(new Entry(pInterface));
	}
	void add(int nValue) {
		add(new Entry(nValue));
	}
	void add(char *sValue) {
		add(new Entry(sValue));
	}

	char *getString(int nPos) {
		Entry *pEntry = findEntry(nPos);
		assert(pEntry->m_type == Entry::ARGUMENT_STRING);
		return pEntry->m_sValue;
	}
	int getInt(int nPos) {
		Entry *pEntry = findEntry(nPos);
		assert(pEntry->m_type == Entry::ARGUMENT_INT);
		return pEntry->m_nValue;
	}
	Interface *getInterface(int nPos) {
		Entry *pEntry = findEntry(nPos);
		assert(pEntry->m_type == Entry::ARGUMENT_INTERFACE);
		return pEntry->m_pInterface;
	}
	MAC getMAC(int nPos) {
		Entry *pEntry = findEntry(nPos);
		assert(pEntry->m_type == Entry::ARGUMENT_MAC);
		return pEntry->m_mac;
	}

	bool has(int nPos, Entry::ArgumentType type) {
		if (nPos >= (int)(m_list.size()))
			return false;

		Entry *pEntry = findEntry(nPos);
		if (!pEntry)
			return false;

		if (pEntry->m_type != type)
			return false;

		return true;
	}

	bool hasInt(int nPos) {
		return has(nPos, Entry::ARGUMENT_INT);
	}
	bool hasInterface(int nPos) {
		return has(nPos, Entry::ARGUMENT_INTERFACE);
	}
	bool hasMAC(int nPos) {
		return has(nPos, Entry::ARGUMENT_MAC);
	}
};


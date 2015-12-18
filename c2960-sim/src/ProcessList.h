#pragma once

#include "Process.h"

class ProcessList
{
	class Entry {
	public:
		Entry(Process *pProc, int nPrio) {
			m_pProc = pProc;
			m_nPrio = nPrio;
		}
		Process* m_pProc;
		int m_nPrio;
	};
	list<Entry*> m_list;

public:
	ProcessList(void);
	~ProcessList(void);

	typedef list<Entry*>::iterator iterator;

	void push(Process *pProc, int nPrio);
	bool remove(Process *pProc);
	void empty();

	iterator begin() { return m_list.begin(); }
	iterator end() { return m_list.end(); }
};

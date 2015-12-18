#include "stdafx.h"

#include "ProcessList.h"


ProcessList::ProcessList(void)
{
}


ProcessList::~ProcessList(void)
{
	empty();
}

void ProcessList::empty() {
	iterator it;
	
	for (it=begin(); it!=end(); ++it) 
		delete (*it);
}

void ProcessList::push(Process *pProc, int nPrio)
{
	iterator it;
	Entry *pEntry;

	pEntry = new Entry(pProc, nPrio);

	for (it=begin(); it!=end(); ++it) {
		if ((*it)->m_nPrio <= nPrio) {
			m_list.insert(it, pEntry);
			return;
		}
	}

	m_list.push_back(pEntry);
}

bool ProcessList::remove(Process *pProc)
{
	iterator it;

	for (it=begin(); it!=end(); it++) {
		if ((*it)->m_pProc == pProc) {
			delete (*it);
			m_list.erase(it);
			return true;
		}
	}

	return false;
}
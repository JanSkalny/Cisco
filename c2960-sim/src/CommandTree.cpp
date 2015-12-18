#include "stdafx.h"

#include "Command.h"
#include "Line.h"
#include "CommandTree.h"
#include "Interface.h"
#include "InterfaceMgr.h"

extern InterfaceMgr g_interfaces;

int CommandTree::registerCommand(LineMode mode, int nPrivilegeLevel, char *sCommand, char *sDescription, CommandHandler cHandler, ...)
{
	char *sCmd, *sDesc;
	char *sOrigCmd, *sOrigDesc;
	char *sNextCmd, *sNextDesc;
	Node *pNode, *pParentNode;
	int nMin, nMax;

	sOrigCmd = sCmd = strdup(sCommand);
	sOrigDesc = sDesc = strdup(sDescription);

	pParentNode = getCommandTree(mode);

	while (sCmd && *sCmd!=0) {
		sNextCmd = strchr(sCmd, ' ');
		sNextDesc = strchr(sDesc, ';');
		if (sNextCmd)
			*(sNextCmd++) = 0;
		if (sNextDesc)
			*(sNextDesc++) = 0;

		pNode = pParentNode->find(sCmd);
		if (!pNode) {
			//if (sNextCmd && strcmp(sNextCmd, "LINE") == 0) {
			if (strcmp(sCmd, "LINE") == 0) {
				pNode = new NodeLine(sCmd, sDesc, nPrivilegeLevel);
			} else if (strcmp(sCmd, "WORD") == 0) {
				pNode = new NodeWord(sCmd, sDesc, nPrivilegeLevel);
			} else if (strcmp(sCmd, "H.H.H") == 0) {
				pNode = new NodeMAC(sCmd, sDesc, nPrivilegeLevel);
			} else if (sCmd[0] == '<') {
				if (strchr(sCmd, '-')) {
					sscanf(sCmd, "<%d-%d>", &nMin, &nMax);
				} else {
					sscanf(sCmd, "<%d>", &nMin);
					nMax = nMin;
				}
				pNode = new NodeNumber(sCmd, sDesc, nPrivilegeLevel, nMin, nMax);
			} else if (strcmp(sCmd, "FastEthernet") == 0) {
				pNode = new NodeInterface(sCmd, sDesc, nPrivilegeLevel); 
			} else {
				pNode = new Node(sCmd, sDesc, nPrivilegeLevel);
			}
			pParentNode->insert(pNode);
		}

		if (strlen(sDesc)) 
			pNode->setDescription(sDesc);

		sCmd = sNextCmd;
		sDesc = sNextDesc;
		pParentNode = pNode;
	} 

	// posledny command, oznacme ho ako vykonatelny
	if (pNode && cHandler) 
		pNode->setHandler(cHandler);	

	free(sOrigCmd);
	free(sOrigDesc);
	
	return 0;
}

CommandTree::CommandTree()
{
	m_pExecRoot = new Node("root","hidden",0);
	m_pConfRoot = new Node("root","hidden",0);
	m_pConfIfRoot = new Node("root","hidden",0);
	m_pConfLineRoot = new Node("root","hidden",0);
	m_pConfVlanRoot = new Node("root","hidden",0);
}

CommandTree::~CommandTree()
{
}

bool CommandTree::walk(WalkOperation op, Line *pLine, Command *pCommand)
{
	Node *pRoot;
	int nRet;
	char *sCommand;
	LineMode mode;

	pCommand->arguments.empty();

	mode = pLine->getMode();

	pRoot = getCommandTree(mode);
	sCommand = strdup(pCommand->get());
	nRet = pRoot->walk(op, pLine, pCommand, sCommand);
	free(sCommand);

	if ((nRet != ERR_OK) &&
		(op == WALK_EXEC) &&
		(mode != MODE_EXEC) &&
		(mode != MODE_CONF)) {
		// ak sme v pod-konfiguracnom rezime, skusme vykonat command aj
		// v globalnom configu, ak zlyhal
		pLine->setMode(MODE_CONF,pLine->getModeData());
		pRoot = getCommandTree(MODE_CONF);
		sCommand = strdup(pCommand->get());
		nRet = pRoot->walk(op, pLine, pCommand, sCommand);
		free(sCommand);

		// ak zlyhal aj tu, vratme povodny mode a fail :(
		if (nRet != ERR_OK)
			pLine->setMode(mode, pLine->getModeData());
	}

	if (nRet == ERR_OK)
		return true;

	pLine->write("\r\n", 2);
	switch (nRet) {
	case ERR_CMD_AMBIGOUS:		
		pLine->write("%% Ambiguous command");	
		break;
	case ERR_CMD_INCOMPLETE:	
		pLine->write("%% Incomplete command.");	
		break;
	case ERR_CMD_UNKNOWN:		
		pLine->write("%% Unknown command");		
		break;

	case ERR_CMD_INVALID_INPUT:
		pLine->write("%% Invalid input detected");
	}
	pLine->write("\r\n");
	return false;
}

bool CommandTree::exec(Line *pLine, Command *pCommand) {
	return walk(WALK_EXEC, pLine, pCommand);
}

bool CommandTree::expand(Line *pLine, Command *pCommand) {
	return walk(WALK_EXPAND, pLine, pCommand);
}

bool CommandTree::helper(Line *pLine, Command *pCommand) {
	return walk(WALK_HELPER, pLine, pCommand);
}


CommandTree::Node::Node(char *sCommand, char *sDescription, int nPrivilegeLevel) {
	m_sCommand = strdup(sCommand);
	m_sDescription = strdup(sDescription);
	m_nPrivilegeLevel = nPrivilegeLevel;

	m_pFirstChild = 0;
	m_pNext = 0;

	m_cHandler = 0;
}

CommandTree::Node::~Node() {
	free(m_sCommand);
	free(m_sDescription);
}

char *CommandTree::Node::splitNextCommand(char *sCommand, char* &sCommandEnd) {
	if (!sCommand)
		return 0;

	while (*sCommand == ' ') 
		sCommand++;

	sCommandEnd = sCommand = strchr(sCommand, ' ');
	while (sCommand && *sCommand == ' ') {
		*sCommand = 0;
		if (*(++sCommand) == 0)
			return 0;
	}

	return sCommand;
}

int CommandTree::Node::walk(WalkOperation op, Line *pLine, Command *pCommand, char *sCommand) {
	char *sNextCommand, *sCommandEnd;
	Node *pNode, *pMatch=0;
	int nMatches=0;
	
	// najdime retazec so zvyskom prikazu (zozerieme prve slovo)
	sNextCommand = splitNextCommand(sCommand, sCommandEnd);

	// mame co vyhodnocovat - najdime vyhovujuce deti
	pNode = m_pFirstChild;
	while (pNode) {
		if (pNode->match(sCommand, pLine->getPrivilegeLevel())) {
			if (!pMatch)
				pMatch = pNode;
			nMatches++;
		}
		pNode = pNode->m_pNext;
	}

	if (pMatch && pMatch->chainNextCommand(sCommand, sNextCommand)) {
		// join (unsplit)
		for(;sCommandEnd<sNextCommand; sCommandEnd++)
			*sCommandEnd = ' ';
		sNextCommand = splitNextCommand(sNextCommand, sCommandEnd);
	}

	if (!sNextCommand && op == WALK_HELPER) {
		if (pCommand->getLen() == 0) {
			return doFullHelper(pLine);
		} else {
			if (pCommand->get()[pCommand->getLen()-1] == ' ') {
				if (!pMatch)
					return doFullHelper(pLine);
				return pMatch->doFullHelper(pLine);
			} else {
				if (!pMatch)
					return ERR_OK;
				return pMatch->doContextHelper(pLine, sCommand);
			}
		}
	}

	if (!sNextCommand && op == WALK_EXPAND) {
		if (!pMatch)
			return ERR_OK;
		return pMatch->doExpand(pLine, pCommand, sCommand);
	}

	if (nMatches > 1) {
		// viac => chyba
		return ERR_CMD_AMBIGOUS;
	} else if (nMatches == 0) {
		// ziadne => chyba
		return ERR_CMD_UNKNOWN;
	} // else nMatches == 1

	// match sa mohol zmenit (po chainovani) -- overme to este raz
	if (pMatch->verify(pCommand, sCommand) != 0)
		return ERR_CMD_INVALID_INPUT;

	// prave jedno vykonajme alebo vnorme sa
	if (!sNextCommand && op == WALK_EXEC)
		return pMatch->doExec(pLine, pCommand);

	return pMatch->walk(op, pLine, pCommand, sNextCommand);
}

bool CommandTree::Node::match(char *sCommand, int nLinePrivilegeLevel) {
	if (strlen(sCommand) == 0)
		return false;

	if ((nLinePrivilegeLevel >= m_nPrivilegeLevel) &&
		(strnicmp(sCommand, m_sCommand, strlen(sCommand)) == 0)) 
		return true;
	return false;
}

CommandTree::Node *CommandTree::Node::find(char *sCommand) {
	Node *pFind = m_pFirstChild;
	
	while (pFind) {
		if (stricmp(pFind->m_sCommand, sCommand) == 0)
			return pFind;
		pFind = pFind->m_pNext;
	}

	return 0;
}

void CommandTree::Node::insert(Node *pNode) {
	Node *pPrev=0, *pFind=m_pFirstChild;
			
	if (!m_pFirstChild) {
		// jediny a prvy
		m_pFirstChild = pNode;
		return;
	}

	// zaradme ho lexikograficky medzi ostatne
	while (pFind) {
		if (stricmp(pFind->m_sCommand, pNode->m_sCommand) > 0) {
			if (pPrev) {
				pNode->m_pNext = pPrev->m_pNext;
				pPrev->m_pNext = pNode;
			} else {
				pNode->m_pNext = m_pFirstChild;
				m_pFirstChild = pNode;
			}
			return;
		}

		pPrev = pFind;
		pFind = pFind->m_pNext;
	}

	// posledny?
	pPrev->m_pNext = pNode;
}

int CommandTree::Node::doExpand(Line *pLine, Command *pCommand, char *sCommand) 
{
	Node *pNode, *pMatch=0;
	char *sBestMatch;
	int nThisMatchLen, nMatchLen=255, i, nAppendFrom, nMatches=0;

	// ak ma command na konci medzeru, ignorujeme
	if ((pCommand->get()[pCommand->getLen()-1] == ' ') ||
		(pCommand->getLen() == 0))
		return 0;

	pNode = this;
	while (pNode) {
		if (pNode->match(sCommand, pLine->getPrivilegeLevel())) {
			nMatches++;
			if (!pMatch) {
				// prvy match
				pMatch = pNode;
				sBestMatch = pNode->m_sCommand;
				nMatchLen = strlen(sBestMatch);
			} else {
				// ak je matchov viac, zistime, aka najdlhsia cast z nich sa zhoduje
				nThisMatchLen = strlen(sCommand);
				for (i=nThisMatchLen; i!=strlen(pNode->m_sCommand); i++) {
					if (tolower(pNode->m_sCommand[i]) == tolower(sBestMatch[i])) {
						nThisMatchLen++;
					} else {
						break;
					}
				}
				if (nThisMatchLen < nMatchLen) 
					nMatchLen = nThisMatchLen;
				if (nMatchLen == strlen(sCommand)) 
					// ak je najlepsia zhoda to co je uz napisane, 
					// nema zmysel hladat dalej
					break;
			}
		}
		pNode = pNode->m_pNext;
	}

	// prilepme na koniec commandu co je jasne
	if (nMatches > 0) {
		nAppendFrom = strlen(sCommand);
		for (i=nAppendFrom; i!=nMatchLen; i++) 
			pCommand->push(sBestMatch[i]);
		if (nMatches == 1)
			pCommand->push(' ');
	}

	return 0;
} 

int CommandTree::Node::doExec(Line *pLine, Command *pCommand) {
	if (m_cHandler) {
		(m_cHandler)(pLine, pCommand, (Interface*)pLine->getModeData(), (Vlan*)pLine->getModeData());
		return 0;
	}
	return ERR_CMD_INCOMPLETE;
}

int CommandTree::Node::doHelper(Line *pLine, Command *pCommand, char *sCommand, Node *pMatch, int nMatches) {
	bool bFullHelp = false;

	if (pCommand->getLen() == 0) {
		return doFullHelper(pLine);
	} else if (pCommand->get()[pCommand->getLen()-1] == ' ') {
		if (nMatches == 1)
			return doFullHelper(pLine);
	} else {
		return doContextHelper(pLine, sCommand);
	}

	return ERR_CMD_AMBIGOUS;
}


int CommandTree::Node::doFullHelper(Line *pLine) {
	Node *pNode;
	int nCmdLen, nPrivilegeLevel, nMaxLen;

	nPrivilegeLevel = pLine->getPrivilegeLevel();

	// urcime si najvacsiu sirku
	nMaxLen = 0;
	pNode = m_pFirstChild;
	while (pNode) {
		if (pNode->getPrivilegeLevel() <= nPrivilegeLevel) {
			nCmdLen = strlen(pNode->getCommand());
			if (nCmdLen > nMaxLen)
				nMaxLen = nCmdLen;
		}
		pNode = pNode->m_pNext;
	}

	// vypisme zoznam pod-prikazov a ich popisov
	pNode = m_pFirstChild;
	pLine->write("\r\n");
	while (pNode) {
		if (pNode->getPrivilegeLevel() <= nPrivilegeLevel) {
			nCmdLen = strlen(pNode->getCommand());
			pLine->write("  ");
			pLine->writeLen(pNode->getCommand(), nCmdLen);
			pLine->writeLen(SPACES, nMaxLen - nCmdLen + 2);
			pLine->write(pNode->getDescription());
			pLine->write("\r\n");
		}
		pNode = pNode->m_pNext;
	}

	return 0;
}

int CommandTree::Node::doContextHelper(Line *pLine, char *sCommand) {
	Node *pNode;
	int nMaxLen[4], nCol, nPrivilegeLevel, nCmdLen;

	nPrivilegeLevel = pLine->getPrivilegeLevel();

	// display possible commands (brief)
	memset(nMaxLen, 0, sizeof(int)*4);
	pLine->write("\r\n");

	// urcime si najsirsi command pre kazdy zo 4 stlpcov
	nCol = 0;
	pNode = this;
	while (pNode) {
		if (pNode->match(sCommand, nPrivilegeLevel)) {
			nCmdLen = strlen(pNode->getCommand());
			if (nCmdLen > nMaxLen[nCol])
				nMaxLen[nCol] = nCmdLen;
			nCol = (nCol+1)%4;
		}
		pNode = pNode->m_pNext;
	}

	// zobrazme commandy v stlpcoch po 4
	nCol = 0;
	pNode = this;
	while (pNode) {
		if (pNode->match(sCommand, nPrivilegeLevel)) {
			nCmdLen = strlen(pNode->getCommand());
			pLine->write(pNode->getCommand(), nCmdLen);
			if (nCol == 3)
				pLine->write("\r\n");
			else
				pLine->writeLen(SPACES, nMaxLen[nCol] + 2 - nCmdLen);
			nCol = (nCol+1)%4;
		}
		pNode = pNode->m_pNext;
	}	

	return 0;
}

int CommandTree::NodeLine::walk(WalkOperation op, Line *pLine, Command *pCommand, char *sCommand) {
	switch (op) {
	case WALK_EXEC:
		return doExec(pLine, pCommand, sCommand);
		
	case WALK_EXPAND:
		return 0;

	case WALK_HELPER:
		pLine->write("\r\n  LINE    <cr>\r\n");
		return 0;
	}

	return 0;
}

bool CommandTree::NodeLine::match(char *sCommand, int nLinePrivilegeLevel)
{
	if (nLinePrivilegeLevel >= m_nPrivilegeLevel)
		return true;
	return false;
}

int CommandTree::NodeLine::verify(Command *pCommand, char *sCommand)
{
	pCommand->arguments.add(sCommand);

	return 0;
}


int CommandTree::NodeLine::doExec(Line *pLine, Command *pCommand, char *sCommand)
{
	if (!m_cHandler) {
		if (m_pFirstChild && m_pFirstChild->getHandler()) {
			pCommand->arguments.add(sCommand);
			(m_pFirstChild->getHandler())(pLine, pCommand, (Interface*)pLine->getModeData(), (Vlan*)pLine->getModeData(), sCommand);
			return 0;
		}
		return ERR_CMD_UNKNOWN;
	}
	pCommand->arguments.add(sCommand);
	(m_cHandler)(pLine, pCommand, (Interface*)pLine->getModeData(), (Vlan*)pLine->getModeData(), sCommand);
	return 0;
}


bool CommandTree::NodeWord::match(char *sCommand, int nLinePrivilegeLevel)
{
	if ((nLinePrivilegeLevel >= m_nPrivilegeLevel)) 
		return true;
	return false;
}

int CommandTree::NodeWord::verify(Command *pCommand, char *sCommand)
{
	pCommand->arguments.add(sCommand);

	return 0;
}

bool CommandTree::NodeNumber::match(char *sCommand, int nLinePrivilegeLevel)
{
	bool bMatch = true;

	while (sCommand && *sCommand) {
		if (!isdigit(*(sCommand++)))
			bMatch = false;
	}

	if ((nLinePrivilegeLevel >= m_nPrivilegeLevel)) 
		return bMatch;
	return false;
}

int CommandTree::NodeNumber::verify(Command *pCommand, char *sCommand) {
	int nVal = atoi(sCommand), i;

	for (i=0; i!=strlen(sCommand); i++) {
		if (!isdigit(sCommand[i]))
			return i;
	}
	if (m_nMax >= nVal && m_nMin <= nVal) {
		// vyhovuje
		pCommand->arguments.add(nVal);
		return 0;
	}

	return 1;
}

bool CommandTree::NodeMAC::match(char *sCommand, int nLinePrivilegeLevel)
{
	if ((nLinePrivilegeLevel >= m_nPrivilegeLevel)) 
		return true;
	return false;
}

int CommandTree::NodeMAC::verify(Command *pCommand, char *sCommand)
{
	MAC addr;

	if (!String2MAC(sCommand, addr))
		return 1;

	pCommand->arguments.add(addr);

	return 0;
}

bool CommandTree::NodeInterface::match(char *sCommand, int nLinePrivilegeLevel)
{
	int nLen;
	char *sScan;

	printf("match interface: \"%s\"\n", sCommand);

	sScan = sCommand;
	while (sScan && *sScan!=0) {
		if (!isalpha(*sScan))
			break;
		++sScan;
	}
	nLen = sScan - sCommand;

	if (strnicmp(sCommand, "FastEthernet", nLen) == 0)
		return true;
	
	return false;
}

int CommandTree::NodeInterface::doContextHelper(Line *pLine, char *sCommand)
{
	//TODO:

	return 0;
}

bool CommandTree::NodeInterface::chainNextCommand(char *sCommand, char *sNextCommand)
{
	if (strchr(sCommand, '0') ||
		strchr(sCommand, '1'))
		return false;

	return true;
}

int CommandTree::NodeInterface::verify(Command *pCommand, char *sCommand) {
	int nBreakPos=0;
	bool bScanBreak=true;
	char *sScan, *sInt=0;
	char sInterfaceName[Interface::MAX_NAME_LEN];
	Interface *pInterface = 0;

	// najdime rozmedzie medzi textom a cislami/medzerami
	sScan = sCommand;
	while (sScan && *sScan!=0) {
		if (bScanBreak) {
			if (isalpha(*sScan))
				nBreakPos++;
			else
				bScanBreak = false;
		} 
		if (!bScanBreak) {
			if (isdigit(*sScan)) {
				sInt = sScan;
				break;
			} 
		}
		sScan++;
	}

	// zlepime si nazov interface-u a pozrieme, ci existuje
	if ((strnicmp(sCommand, "FastEthernet", nBreakPos) == 0) &&
		sInt) {
		sprintf(sInterfaceName,"FastEthernet%s", sInt);
		pInterface = g_interfaces.find(sInterfaceName);
		pCommand->arguments.add(pInterface);
	}

	if (!pInterface)
		return nBreakPos;

	return 0;
}
#pragma once

class Line;
class Command;

class CommandTree {
	typedef enum {
		WALK_EXEC,
		WALK_EXPAND,
		WALK_HELPER,
	} WalkOperation;
	typedef enum {
		ERR_OK = 0,
		ERR_CMD_INCOMPLETE=1,
		ERR_CMD_AMBIGOUS=2,
		ERR_CMD_UNKNOWN=3,
		ERR_CMD_INVALID_INPUT=4,
	};
	class Node {
	protected:
		char *m_sCommand;
		char *m_sDescription;
		Node *m_pFirstChild;
		Node *m_pNext;
		int m_nPrivilegeLevel;

		CommandHandler m_cHandler;

	public:
		Node(char *sCmd, char *sDesc, int nPrivilegeLevel);
		~Node();

		virtual int walk(WalkOperation op, Line *pLine, Command *pCommand, char *sCommand);
		char *splitNextCommand(char *sCommand, char* &sCommandEnd);
		virtual bool chainNextCommand(char *sCommand, char *sNextCommand) { return false; }

		virtual int doExec(Line *pLine, Command *pCommand);
		virtual int doExpand(Line *pLine, Command *pCommand, char *sCommand=0);
		virtual int doHelper(Line *pLine, Command *pCommand, char *sCommand, Node *pMatch, int nMatches);
		virtual int doFullHelper(Line *pLine);
		virtual int doContextHelper(Line *pLine, char *sCommand);
		
		Node *find(char *sCommand);
		virtual bool match(char *sCommand, int nLinePrivilegeLevel);
		virtual int verify(Command *pCommand, char *sCommand) { return 0; };

		void insert(Node *pNode);

		int getPrivilegeLevel() { return m_nPrivilegeLevel; }
		void setPrivilegeLevel(int nPrivilegeLevel) { m_nPrivilegeLevel = nPrivilegeLevel; }
		char *getCommand() { return m_sCommand; }
		char *getDescription() { return m_sDescription; }
		void setDescription(char *sDescription) { if (m_sDescription) free(m_sDescription); m_sDescription = strdup(sDescription); }
		void setHandler(CommandHandler cHander) { m_cHandler = cHander; }
		CommandHandler getHandler() { return m_cHandler; }
	};

	class NodeLine : public Node {
	public:
		NodeLine(char *sCmd, char *sDesc, int nPrivilegeLevel)
			:Node(sCmd, sDesc, nPrivilegeLevel) { };
		int walk(WalkOperation op, Line *pLine, Command *pCommand, char *sCommand);
		bool match(char *sCommand, int nLinePrivilegeLevel);
		int verify(Command *pCommand, char *sCommand);

		int doExec(Line *pLine, Command *pCommand, char *sCommand=0);
		int doExpand(Line *pLine, Command *pCommand, char *sCommand=0) { return 0; }
	};

	class NodeWord : public Node {
	public:
		NodeWord(char *sCmd, char *sDesc, int nPrivilegeLevel)
			:Node(sCmd, sDesc, nPrivilegeLevel) { };
		bool match(char *sCommand, int nLinePrivilegeLevel);
		int verify(Command *pCommand, char *sCommand);
		int doExpand(Line *pLine, Command *pCommand, char *sCommand=0) { return 0; }
	};

	class NodeNumber : public Node {
		int m_nMin, m_nMax;
	public:
		NodeNumber(char *sCmd, char *sDesc, int nPrivilegeLevel, int nMin, int nMax)
			:Node(sCmd, sDesc, nPrivilegeLevel) { m_nMin = nMin; m_nMax = nMax;	};
		bool match(char *sCommand, int nLinePrivilegeLevel);
		int verify(Command *pCommand, char *sCommand);
		int doExpand(Line *pLine, Command *pCommand, char *sCommand=0) { return 0; }
	};

	class NodeInterface : public Node {
	public:
		NodeInterface(char *sCmd, char *sDesc, int nPrivilegeLevel)
			: Node(sCmd, sDesc, nPrivilegeLevel) { }
		bool match(char *sCommand, int nLinePrivilegeLevel);
		int verify(Command *pCommand, char *sCommand);
		bool chainNextCommand(char *sCommand, char *sNextCommand);
		int doContextHelper(Line *pLine, char *sCommand);
	};

	class NodeMAC : public Node {
	public:
		NodeMAC(char *sCmd, char *sDesc, int nPrivilegeLevel)
			:Node(sCmd, sDesc, nPrivilegeLevel) { };
		bool match(char *sCommand, int nLinePrivilegeLevel);
		int verify(Command *pCommand, char *sCommand);
		int doExpand(Line *pLine, Command *pCommand, char *sCommand=0) { return 0; }
	};

	Node *getCommandTree(LineMode mode) {
		switch (mode) {
		case MODE_EXEC: return m_pExecRoot; 
		case MODE_CONF: return m_pConfRoot; 
		case MODE_CONF_IF: return m_pConfIfRoot; 
		case MODE_CONF_LINE: return m_pConfLineRoot; 
		case MODE_CONF_VLAN: return m_pConfVlanRoot; 
		}
		return 0;
	}
	Node *m_pExecRoot, *m_pConfRoot, *m_pConfIfRoot, *m_pConfLineRoot, *m_pConfVlanRoot;

public:
	CommandTree();
	~CommandTree();
	
	int registerCommand(LineMode mode, int nPrivilegeLevel, char *sCommand, char *sDecription, CommandHandler cExec=0, ...);

	bool exec(Line *pLine, Command *pCommand);
	bool expand(Line *pLine, Command *pCommand);
	bool helper(Line *pLine, Command *pCommand);

	bool walk(WalkOperation op, Line *pLine, Command *pCommand);
};

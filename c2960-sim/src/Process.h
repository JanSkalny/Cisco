#pragma once

class Frame;

class Process
{
public:
	Process(void);
	virtual ~Process(void);

	virtual int getNextEventTimeout() { return -1; }
	virtual HANDLE getEvent() { return 0; }

	// call hooks
	virtual bool onTimeout() { return true; }
	virtual bool onRecv(Frame *pFrame) { return true; }
	virtual bool onSend(Frame *pFrame) { return true; }
	virtual bool onUp(Interface *pInterface) { return true; }
	virtual bool onDown(Interface *pInterface) { return true; }
};

#pragma once

class Worker {
protected:
	pthread_t m_thread;
	bool m_bRunning;

public:
	Worker();

	bool start();

	void onExit(int);
	bool isRunning() { return m_bRunning; }

	virtual int work() = 0;
};


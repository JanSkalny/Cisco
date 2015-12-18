#include "stdafx.h"

#include "Worker.h"

void *WorkerCoatingFunc(void *pInstance)
{
	int nRet;

	Worker *pWorker = (Worker*)pInstance;

	nRet = pWorker->work();
	pWorker->onExit(nRet);

	return 0;
}

bool Worker::start() {
	if (pthread_create(&m_thread, NULL, WorkerCoatingFunc, (void*)this) != 0)
		return false;

	m_bRunning = true;

	return true;
}

void Worker::onExit(int nRet) 
{
   	m_bRunning = false;
	pthread_exit(&nRet);
}

Worker::Worker() {
	m_bRunning = false;
}


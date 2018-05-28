#include "IdleThrd.h"

IdleThread* IdleThread::idleThread = 0;

IdleThread::IdleThread() :
		Thread(0, 1, 'i') {
}

IdleThread::~IdleThread() {
}

void IdleThread::run() {
	while (1) {
		for (int i = 0; i < 10000; i++) {
			for (int j = 0; j < 10000; j++)
			 for (int k = 0; k< 10000;k++)	;
			if (i % 1000 == 0)
				cout << "IT::run idle" << endl;
		}
	}
}

IdleThread* IdleThread::getIdleThread() {
	if (idleThread == 0)
		idleThread = new IdleThread();
	return idleThread;
}

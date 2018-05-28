#include "IVTEntry.h"
#include <dos.h>
#include "include.h"
#include "PCB.h"
#include "CSwitch.h"
#include "KernelEv.h"
#include "IVTable.h"
#include "Event.h"
IVTable* IVTEntry::ivTable = new IVTable();

IVTEntry::IVTEntry(IVTNo ivtNo, void interrupt (*newRoutine)(...)) : ivtNo(ivtNo), event(0) {
	lock;
	#ifndef BCC_BLOCK_IGNORE
		oldRoutine = getvect(ivtNo);
		setvect(ivtNo, newRoutine);
	#endif
		IVTEntry::ivTable->setEntry(ivtNo, this);
	unlock;
}

IVTEntry::~IVTEntry() {
	lock;
	IVTEntry::ivTable->setEntry(ivtNo, 0);
	#ifndef BCC_BLOCK_IGNORE
	if(oldRoutine != 0) {
		setvect(ivtNo, oldRoutine);
		oldRoutine = 0;
	}
	#endif
	unlock;
}

void IVTEntry::signal() {
	lock;
	if(event!=0) event->signal();
	unlock;
}


void IVTEntry::runOldRoutine() {
	if(oldRoutine != 0) (*oldRoutine)();
}


#include "KernlThr.h"
#include "HelpStrc.h"
#include "CSwitch.h"
#include <dos.h>
#include "PCB.h"
#include "SCHEDULE.H"
#include "IdleThrd.h"
#include "Thread.h"
#include "include.h"
#include "Event.h"
#include "KernelEv.h"
#include "KernlSem.h"
#include "Semaphor.h"


volatile int KernelThread::inKernel = 1;
volatile int KernelThread::kernelThreadRequestedSwitch = 0;
KernelThread* KernelThread::kernelThread = 0;
volatile Helper* KernelThread::helper = 0;
volatile PCB* KernelThread::myPCB = new PCB();
volatile Function* KernelThread::functions = 0;
const int KernelThread::SWITCH_ENTRY = 60;

volatile unsigned tempStackSegmentKT = 0;
volatile unsigned tempStackPointerKT = 0;
volatile unsigned tempBasePointerKT = 0;
volatile unsigned tempCX = 0;
volatile unsigned tempDX = 0;

KernelThread::KernelThread() {

	functions = new Function[16];
	functions[0] = KernelThread::threadConstruct;
	functions[1] = KernelThread::threadStart;
	functions[2] = KernelThread::threadWaitToComplete;
	functions[3] = KernelThread::threadSleep;
	functions[4] = KernelThread::threadDispatch;
	functions[5] = KernelThread::threadResumeAll;
	functions[6] = KernelThread::threadDestruct;
	functions[7] = KernelThread::eventConstruct;
	functions[8] = KernelThread::eventWait;
	functions[9] = KernelThread::eventSignal;
	functions[10] = KernelThread::eventDestruct;
	functions[11] = KernelThread::semaphoreConstruct;
	functions[12] = KernelThread::semaphoreWait;
	functions[13] = KernelThread::semaphoreSignal;
	functions[14] = KernelThread::semaphoreValue;
	functions[15] = KernelThread::semaphoreDestruct;
}

KernelThread::~KernelThread() {}

KernelThread* KernelThread::getKernelThread() {
	if (kernelThread == 0)
		kernelThread = new KernelThread();
	return kernelThread;
}

void KernelThread::run() {
	while(1) {

		cout<<"KT::run"<<endl;
		functions[helper->function]();
		switchDomain();
	}
}

void interrupt KernelThread::switchDomain(...) {

	if(!inKernel) {
		#ifndef BCC_BLOCK_IGNORE
			asm{
				mov tempStackSegmentKT, ss
				mov tempStackPointerKT, sp
				mov tempBasePointerKT, bp
				mov tempCX, cx
				mov tempDX, dx
			}
			helper = (Helper*)MK_FP(tempCX, tempDX);
		#endif

		PCB::running->stackSegment = tempStackSegmentKT;
		PCB::running->stackPointer = tempStackPointerKT;
		PCB::running->basePointer = tempBasePointerKT;

		PCB::running->localLock = PCB::globalLock;

		tempStackSegmentKT = myPCB->stackSegment;
		tempStackPointerKT = myPCB->stackPointer;
		tempBasePointerKT = myPCB->basePointer;

		PCB::globalLock = myPCB->localLock;

		#ifndef BCC_BLOCK_IGNORE
			asm{
				mov ss, tempStackSegmentKT
				mov sp, tempStackPointerKT
				mov bp, tempBasePointerKT
			}
		#endif
	}
	else {
		#ifndef BCC_BLOCK_IGNORE
			asm{
				mov tempStackSegmentKT, ss
				mov tempStackPointerKT, sp
				mov tempBasePointerKT, bp
			}
		#endif

		myPCB->stackSegment = tempStackSegmentKT;
		myPCB->stackPointer = tempStackPointerKT;
		myPCB->basePointer = tempBasePointerKT;

		myPCB->localLock = PCB::globalLock;

		if(kernelThreadRequestedSwitch == 1) {
			if(PCB::running->status == PCB::RUNNING && PCB::running->thread != IdleThread::getIdleThread()) {
				PCB::running->setStatus(PCB::READY);
				Scheduler::put(PCB::running);
			}

			PCB::running = Scheduler::get();

			if(PCB::running == 0)
				PCB::running = PCB::getPCBbyId(PCB::getIdleThreadId());
			PCB::running->setStatus(PCB::RUNNING);
			PCB::globalLock = PCB::running->localLock;
			ContextSwitch::counter = PCB::running->timeSlice;

			tempStackSegmentKT = PCB::running->stackSegment;
			tempStackPointerKT = PCB::running->stackPointer;
			tempBasePointerKT = PCB::running->basePointer;

			ContextSwitch::counter = PCB::running->timeSlice;

			#ifndef BCC_BLOCK_IGNORE
				asm{
					mov ss, tempStackSegmentKT
					mov sp, tempStackPointerKT
					mov bp, tempBasePointerKT
				}
			#endif

			kernelThreadRequestedSwitch = 0;
		}

	}
	inKernel = !inKernel;
}

void KernelThread::threadConstruct() {
	cout<<"KT::threadConst"<<endl;
	Thread* thread = 0;
	#ifndef BCC_BLOCK_IGNORE
		thread = (Thread*)MK_FP(helper->stackSegment, helper->stackOffset);
	#endif
	new PCB(thread, helper->timeSlice, helper->stackSize);
	helper->id = PCB::ID - 1; //when inserting pcb his id will be id = ID++;
}

void KernelThread::threadDestruct() {
	PCB::pcbList.removeById(helper->id);
}

void KernelThread::threadStart() {
	PCB* myPCB = PCB::pcbList.getById(helper->id);
	myPCB->setStatus(PCB::READY);
	Scheduler::put(myPCB);
	myPCB = 0;
}

void KernelThread::threadWaitToComplete() {
	PCB::pcbList.getById(helper->id)->waitToComplete();
}

void KernelThread::threadSleep() {
	PCB::running->sleep(helper->timeSlice);
}

void KernelThread::threadDispatch() {
	kernelThreadRequestedSwitch = 1;
}

void KernelThread::threadResumeAll() {
	PCB::running->setStatus(PCB::RUNNING);
	PCB::pcbList.resumeAll();
}

void KernelThread::eventConstruct() {
	Event* event = 0;
	#ifndef BCC_BLOCK_IGNORE
		event = (Event*)MK_FP(helper->stackSegment, helper->stackOffset);
	#endif
	new KernelEv(helper->ivtNo);
	helper->id = KernelEv::ID - 1;
}

void KernelThread::eventDestruct() {
	KernelEv::kernelEventList.removeById(helper->id);
}

void KernelThread::eventWait() {
	KernelEv::kernelEventList.getById(helper->id)->wait();
}

void KernelThread::eventSignal() {
	KernelEv::kernelEventList.getById(helper->id)->signal();
}

void KernelThread::semaphoreConstruct() {
	Semaphore* semaphore = 0;
	#ifndef BCC_BLOCK_IGNORE
		semaphore = (Semaphore*)MK_FP(helper->stackSegment, helper->stackOffset);
	#endif
	new KernelSem(helper->init);
	helper->id = KernelSem::ID - 1;
}

void KernelThread::semaphoreDestruct() {
	delete KernelSem::kernelSemaphoreList.getById(helper->id);
	KernelSem::kernelSemaphoreList.removeById(helper->id);
}

void KernelThread::semaphoreWait() {
	int temp = 0;
	temp = KernelSem::kernelSemaphoreList.getById(helper->id)->wait();
	helper->init = temp;
}

void KernelThread::semaphoreValue() {
	int temp = 0;
	temp = KernelSem::kernelSemaphoreList.getById(helper->id)->val();
	helper->init = temp;
}

void KernelThread::semaphoreSignal() {
	KernelSem::kernelSemaphoreList.getById(helper->id)->signal();
}

void KernelThread::inic() {
#ifndef BCC_BLOCK_IGNORE
	asm pushf;
	asm cli;
	setvect(SWITCH_ENTRY, switchDomain);
	asm popf;
#endif
}

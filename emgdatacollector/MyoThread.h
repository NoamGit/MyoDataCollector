#include <windows.h>
#include <iostream>
#include "DataCollector.h"
#include <myo/myo.hpp>
#include <process.h>

#pragma once

class CMyoThread
{
	// Private Fields
private:
	HWND   m_hWnd;    // Window handle to the UI dialog (used for daq and completed messages)
	myo::Myo * pMYO;
	myo::Hub * pHUB;

protected:
	// Determines which type of user defined message to post
	enum	NotificationTypes { NOTIFY_SAMPLE, NOTIFY_THREAD_COMPLETED };

public:
	HANDLE m_myoThread; // Secondary thread handle
	HANDLE m_myoShutdownEvent; // Shutdown Event handle (causes thread to exit)
	DataCollector * m_DataCollector;

public:

	CMyoThread()
		: m_myoThread(NULL),
		m_myoShutdownEvent(::CreateEvent(NULL, TRUE, FALSE, NULL)),
		m_DataCollector(NULL)
	{

	}

	CMyoThread(myo::Hub t_hub, myo::Myo * t_myo, DataCollector t_dataCollector)
		: m_myoThread(NULL),
		m_myoShutdownEvent(::CreateEvent(NULL, TRUE, FALSE, NULL))
	{
		pHUB = &t_hub;
		m_DataCollector = &t_dataCollector;
		pMYO = t_myo;
	}

	~CMyoThread()
	{
		ShutdownThread();
		::CloseHandle(m_myoShutdownEvent);
	}

	// Public interface methods
public:

	//This function initializes the hub and the Myo and sets the destination path
	//for the data sheet.
	void SetPath(CSimpleString filePath){

		m_DataCollector = new DataCollector(filePath);
		// TODO fix the hub initilization. we need to be able to save the hub member paired with the myo for
		//further process
	}

	// Shuts down the thread in a save manner
	HRESULT ShutdownThread(){
		HRESULT hr = S_OK;

		// Close worker thread
		if (m_myoThread == NULL){
			// Signal the thread to exit
			::SetEvent(m_myoShutdownEvent);

			// thread may be suspended, so resume before shutting down
			::ResumeThread(m_myoThread);

			// Wait for the thread to exit. If it doesn't shut down
			// on its own, force it closed with Terminate thread
			if (WAIT_TIMEOUT == WaitForSingleObject(m_myoThread, 1000)){
				::TerminateThread(m_myoThread, -1000);
				hr = S_FALSE;
			}

			// Close the handle and NULL it out
			::CloseHandle(m_myoThread);
			m_myoThread = NULL;

		}

		// Reset the shutdown event
		::ResetEvent(m_myoShutdownEvent);

		return hr;
	}

	//+------------------------------------------------------
	// Public method to start a worker thread
	//+------------------------------------------------------
	HRESULT Start(HWND hWnd, myo::Hub * t_hub, myo::Myo * t_myo) {
		pHUB = t_hub;
		pMYO = t_myo;

		//pHUB->run(1);
		//myo::Hub hub("com.noamgit.emg-daq");
		////myo::Myo * myo = hub.waitForMyo(10000);
		////myo->setStreamEmg(myo::Myo::streamEmgEnabled);
		//hub.addListener(m_DataCollector);
		//pHUB = &hub;
		//pMYO = t_myo;

		HRESULT hr = S_OK;
		m_hWnd = hWnd;	
		
		if (SUCCEEDED(hr = ShutdownThread())){
			hr = CreateThread();
		
		}
		return hr;
	}

	//+------------------------------------------------------
	// Public method to stop a running thread
	//+------------------------------------------------------
	HRESULT Stop() {
		return ShutdownThread();
	}

private:

	// Creates the secondary display thread
	HRESULT CreateThread(){
		// Fire off the thread
		if (NULL == (m_myoThread = (HANDLE)_beginthreadex(		// if no error accurs
			NULL,												// void *security,
			0,													// unsigned stack_size,
			ThreadProc,											// unsigned ( __stdcall *start_address )( void * ),
			static_cast<LPVOID>(this),							// void *arglist,
			0,													// unsigned initflag,
			NULL)))												// unsigned *thrdaddr 
		{
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		return S_OK;
	}

	//+------------------------------------------------------
	// Called by the secondary thread to post a inc progress or 
	// thread completed user defined message
	//+------------------------------------------------------
	void NotifyUI(UINT uNotificationType){
		// Check if the hWnd is still valid before posting the message
		// Note: use PostMessage instead of SendMessage because PostMessage
		// performs an asynchronous post; whereas SendMessage sends synchronously
		// (sending synchronously would kind of defeat the purpose of using a
		// worker thread)

		if (::IsWindow(m_hWnd)){
			switch (uNotificationType)
			{
			case NOTIFY_SAMPLE:
				::PostMessage(m_hWnd, WM_USER_SAMPLE, 0, 0);
				break;
			case NOTIFY_THREAD_COMPLETED:
				::PostMessage(m_hWnd, WM_USER_THREAD_COMPLETED, 0, 0);
				break;
			default:
				ASSERT(0);
			}
		}
	}

	//+------------------------------------------------------
	// Secondary thread procedure
	//+------------------------------------------------------
	static UINT WINAPI ThreadProc(LPVOID lpContext){
	//	static UINT WINAPI ThreadProc(LPVOID lpContext){
		// Turn the passed in 'this' pointer back into a CProgressMgr instance

		CMyoThread* pMyoThread = reinterpret_cast<CMyoThread*>(lpContext);
		
		//myo::Hub hub2("com.noamgit.emg-daq");
		//myo::Myo * myo2 = pMyoThread->pHUB->waitForMyo(10000);
		//myo2->setStreamEmg(myo::Myo::streamEmgEnabled);
		//DataCollector collector; 
		//pMyoThread->pHUB->addListener(&collector);
		//pMyoThread->pMYO = pMyoThread->pHUB->waitForMyo(10000);
		//pMyoThread->pMYO->setStreamEmg(myo::Myo::streamEmgEnabled);
		

		for (UINT uCount = 0; uCount < 100; uCount++)
		{
			if (WAIT_OBJECT_0 == WaitForSingleObject(pMyoThread->GetShutdownEvent(), 0))
			{
				return 1;
			}
			
			//pMyoThread->pHUB->run(1000 / 20);

			// Send the progress message to the UI
			pMyoThread->NotifyUI(NOTIFY_SAMPLE);
		}

		// Send the thread completed message to the UI
		pMyoThread->NotifyUI(NOTIFY_THREAD_COMPLETED);

		return 0;
	}

	HANDLE& GetShutdownEvent(){
		return m_myoShutdownEvent;
	}
};
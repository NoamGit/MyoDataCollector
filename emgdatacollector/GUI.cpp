//01/16/2010 C. Germany Visual Studio 2008 MFC Template
/*
based on notes in original file. See Notes!
Conclusion: see post in thalamic labs developer forum.
Initiating in Thread is a problem - because myo is deleted in Threadprocess
Initiating in onSample is problem - hub is destructed before entering the 
*/

//-----------------------------------------------------------------------------------------
#include <afxwin.h>      //MFC core and standard components
#include "resource.h"    //main symbols
#include "DataCollector.h"
#include <afxcmn.h>		// Needed for Progress bar
#include <afxdlgs.h> 

#define _USE_MATH_DEFINES
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <time.h>
#include <sstream>

#include <myo/myo.hpp>
#include <windows.h>
#include "MyoThread.h"

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

//-----------------------------------------------------------------------------------------
//Globals
CEdit * pLABEL;
CEdit * pSTATUS;
CButton * pSTART;
CButton * pSTOP;
CButton * pSAVE;
CButton * pCONNECT;
CSliderCtrl * pSlider;
//-----------------------------------------------------------------------------------------

class GUI_FORM : public CDialog
{
public:
	GUI_FORM(CWnd* pParent = NULL)
		: CDialog(GUI_FORM::IDD, pParent),
		m_ThreadState(TS_STOPPED),
		m_MyoThread() // some initializations
	{    }

	// Dialog Data, name of dialog form
	enum{ IDD = IDD_MAINGUI };
	enum ThreadStatus { TS_STOPPED, TS_START };
	INT   m_ThreadState;

	// Globals Myo
	myo::Myo * pMYO;
	myo::Hub * pHUB;

	DataCollector * m_Collector = NULL;
	CFolderPickerDialog pathFolder;
	CMyoThread m_MyoThread;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) { CDialog::DoDataExchange(pDX); } //Wizard code
	//Called right after constructor. Initialize things here.
	virtual BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();
		SetupInterfacePointers();
		InitializeInterface();
		return true;
	}

public:
	DECLARE_MESSAGE_MAP() // macro code  - need for dialog generation
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedConnect();
	afx_msg void SaveAs();
	afx_msg void OnNMThemeChangedScrollbar1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMOutofmemoryHotkey1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnSample(WPARAM, LPARAM);
	afx_msg LRESULT OnThreadCompleted(WPARAM, LPARAM);

	void SetupInterfacePointers(){
		// Sets up all pointers to dialog controls
		pSTATUS = (CEdit *)GetDlgItem(CE_TEXT);
		pLABEL = (CEdit *)GetDlgItem(CE_LABEL);
		pSTART = (CButton *)GetDlgItem(CB_START);
		pSTOP = (CButton *)GetDlgItem(CB_STOP);
		pSAVE = (CButton *)GetDlgItem(CB_SAVE);
		pCONNECT = (CButton *)GetDlgItem(CB_CONNECT);
	}
			
	void InitializeInterface(){
		// Initializes  Dialog box
		pSTATUS->SetWindowText(L"Choose movement label and press 'a' to tag \n current movemet with that label");
		pLABEL->SetWindowText(L"Type label here..");

		// Create hub and myo members for future DAQ
		//myo::Hub hub("com.noamgit.emg-daq");
		//myo::Myo * myo; 
		//myo = hub.waitForMyo(1000);
		//myo->setStreamEmg(myo::Myo::streamEmgEnabled);
		//hub.addListener(m_Collector);
		//pMYO = myo;
		//pHUB = &hub;

		pSTART->EnableWindow(false);
		pSTOP->EnableWindow(false);

	}

//	afx_msg void OnBnClickedStart2();
};
//-----------------------------------------------------------------------------------------

class EmgCollectGui : public CWinApp
{
public:
	EmgCollectGui() {  }
public:
	virtual BOOL InitInstance()
	{
		CWinApp::InitInstance();
		//SetRegistryKey(_T("Hills Of Darkness"));
		GUI_FORM dlg;
		m_pMainWnd = &dlg;
		INT_PTR nResponse = dlg.DoModal(); // pop upp and display dialog
		return FALSE;
	} //close function
};

//-----------------------------------------------------------------------------------------
// MESSAGE MAP - Need a Message Map Macro for both CDialog and CWinApp
BEGIN_MESSAGE_MAP(GUI_FORM, CDialog)
	ON_BN_CLICKED(CB_SAVE, &GUI_FORM::SaveAs)
	ON_BN_CLICKED(CB_START, &GUI_FORM::OnBnClickedStart)
	ON_BN_CLICKED(CB_STOP, &GUI_FORM::OnBnClickedStop)
	ON_BN_CLICKED(CB_CONNECT, &GUI_FORM::OnBnClickedConnect)
	ON_MESSAGE(WM_USER_SAMPLE, OnSample)
	ON_MESSAGE(WM_USER_THREAD_COMPLETED, OnThreadCompleted)
END_MESSAGE_MAP()
//-----------------------------------------------------------------------------------------

EmgCollectGui theApp;  //Starts the Application

// ----------------------------------------------------------ACTION METHODS---------------------------------------------------------- //

void GUI_FORM::OnBnClickedStart()
{
	pSTART->EnableWindow(FALSE);
	pSTOP->EnableWindow(FALSE);

	// Create hub and myo members for future DAQ
	myo::Hub hub("com.noamgit.emg-daq");
	pHUB = &hub;
	pMYO = pHUB->waitForMyo(10000);
	pMYO->setStreamEmg(myo::Myo::streamEmgEnabled);
	pHUB->addListener(m_Collector);

	//pHUB->run(1);

 	pSTATUS->SetWindowText(L"DAQ starts");
	m_MyoThread.Start(GetSafeHwnd(), pHUB, pMYO);

	pSTOP->EnableWindow(TRUE);
}


void GUI_FORM::OnBnClickedStop()
{
	// TODO: Add your control notification handler code here
	pSTOP->EnableWindow(FALSE);

	pSTATUS->SetWindowText(L"DAQ stops");
	m_MyoThread.Stop();

	pSTART->EnableWindow(TRUE);
}

void GUI_FORM::OnBnClickedConnect(){
	// Searches and Connects to Myo
		pSTART->EnableWindow(true);
		pSTATUS->SetWindowText(L"Attempting to find a Myo...");
		try{
			//myo::Hub hub("com.noamgit.emg-daq");
			//pMYO = hub.waitForMyo(10000);
			//pHUB = &hub;
			pCONNECT->SetWindowText(_T("Disconnect"));
			pSTATUS->SetWindowText(L"Myo Connected!");
			//pMYO->setStreamEmg(myo::Myo::streamEmgEnabled);
			return;
		}
		catch(exception)
		{
			pSTATUS->SetWindowText(L"Myo Connection Failed...");
			return;
		}
}

void GUI_FORM::SaveAs()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Retruns and set address for file

	//CFolderPickerDialog folderDlg;
	GUI_FORM::pathFolder.DoModal();
		//AfxMessageBox(GUI_FORM::pathFolder.GetFolderPath());

	m_Collector = new DataCollector(GUI_FORM::pathFolder.GetPathName());
	//m_MyoThread.SetPath(GUI_FORM::pathFolder.GetPathName());
	//GUI_FORM::pathFolder = &folderDlg;
	//UpdateData(TRUE);
}

void GUI_FORM::OnNMThemeChangedScrollbar1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// This feature requires Windows XP or greater.
	// The symbol _WIN32_WINNT must be >= 0x0501.
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void GUI_FORM::OnNMOutofmemoryHotkey1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

// ---------------------------------------------------------- THREAD FUNCTIONS ---------------------------------------------------------- //


// Message received from the worker thread to continue DAQ
LRESULT GUI_FORM::OnSample(WPARAM, LPARAM)
{
	pHUB->run(1);
	return 1;
}

// Message received from the worker thread.
// Indicates the thread has completed
LRESULT GUI_FORM::OnThreadCompleted(WPARAM, LPARAM)
{
	// Reset the start button text and state
	return 1;
}
// ---------------------------------------------------------- HELPER FUNCTIONS ---------------------------------------------------------- //
//Start Draft
//
//// TODO: Add your control notification handler code here
//
//// We catch any exceptions that might occur below -- see the catch statement for more details.
//pSTOP->EnableWindow(true);
//try {
//	recordFlag = true;



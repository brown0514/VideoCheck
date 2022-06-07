//////////////////////////////////////////////////////////////////////////
//
// Application entry-point
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "videovalidator.h"
#include "SG_VideoValidator.h"
#include "findfile.h"
#include "sg_devices.h"

BOOL    InitializeApp();
void    CleanUp();
void	VideoSearch(CString path);


int VideoFileValid(const WCHAR *sURL);



// Global variables

SG_VideoValidator		g_VideoValidator;
FindFile *				m_pDBFindFile;

#define LOG_COLOR_DARKBLUE 9
#define LOG_COLOR_DARKGREEN 2
#define LOG_COLOR_WHITE 7
#define LOG_COLOR_GREEN 10
#define LOG_COLOR_YELLOW 14 
#define LOG_COLOR_MClientA 13
#define LOG_COLOR_CIAN 11

void setcolor(int textcol, int backcol)
{
	if ((textcol % 16) == (backcol % 16))textcol++;
	textcol %= 16; backcol %= 16;
	unsigned short wAttributes = ((unsigned)backcol << 4) | (unsigned)textcol;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, wAttributes);
}


CString SG_GetBaseFilePath(CString name)
{
	CString result = name;
	int c = name.ReverseFind(L'\\');
	if (c > -1)
	{
		result = name.Left(c);
	}
	return result;
}


#define LOGFILE			L"log.txt"
void ShowStatus(LPCWSTR lpText, ...)
{
	CString sMsg;
	va_list ptr;
	CTime Today = CTime::GetCurrentTime();

	va_start(ptr, lpText);
	sMsg.FormatV(lpText, ptr);
	va_end(ptr);

	static CString lastMsg{ L"" };
	if (sMsg == lastMsg) return;
	lastMsg = sMsg;

	//	CWnd *pWnd = AfxGetApp()->m_pMainWnd;
	//	::SetDlgItemText(pWnd->GetSafeHwnd(), IDC_LABEL_MAINSTATUS, sMsg.GetBuffer());

	CString sLine;
	sLine.Format(L"%s: %s", (LPCTSTR)Today.FormatGmt(L"%d.%m.%Y %H:%M:%S"), (LPCTSTR)sMsg);

	sMsg.ReleaseBuffer();

	wprintf(L"%s\n", sMsg.GetBuffer());
	FILE *fp;
	_wfopen_s(&fp, LOGFILE, L"a,ccs=UTF-8");
	if (fp)
	{
		fwprintf(fp,L"%s\n", sLine.GetBuffer());
		fclose(fp);

	}

	sMsg.ReleaseBuffer();
}

HANDLE ghSvcStopEvent = NULL;



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 1;
	static HDEVNOTIFY hDeviceNotify;
	static HWND hEditWnd;
	static ULONGLONG msgCount = 0;

	switch (uMsg)
	{
		case WM_CREATE:
		{
			if (!DoRegisterDeviceInterfaceToHwnd(
				GUID_DEVINTERFACE_USB_DEVICE,
				hwnd,
				&hDeviceNotify))
			{
				// Terminate on failure.
				ExitProcess(1);
			}
			break;
		}
		case WM_DEVICECHANGE:
		{
			PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
			TCHAR strBuff[256];
			CString newPath;
			// Output some messages to the window.
			switch (wParam)
			{
				case DBT_DEVICEARRIVAL:
				{
					/*msgCount++;
					StringCchPrintf(
						strBuff, 256,
						TEXT("Message %d: DBT_DEVICEARRIVAL\n"), (int)msgCount);
					CString newPath = DeviceChangeToDriveLetter(wParam, lParam);
					VideoSearch(newPath);*/
					break;
				}
				case DBT_DEVICEREMOVECOMPLETE:
					msgCount++;
					StringCchPrintf(
						strBuff, 256,
						TEXT("Message %d: DBT_DEVICEREMOVECOMPLETE\n"), (int)msgCount);
					break;
				case DBT_DEVNODES_CHANGED:
					msgCount++;
					StringCchPrintf(
						strBuff, 256,
						TEXT("Message %d: DBT_DEVNODES_CHANGED\n"), (int)msgCount);
					newPath = DeviceChangeToDriveLetter(wParam, lParam);
					VideoSearch(newPath);
					break;
				default:
					break;
			}
			//OutputMessage(hEditWnd, wParam, (LPARAM)strBuff);
			ShowStatus(L"%s", strBuff);
			CString DriveLetter = DeviceChangeToDriveLetter(wParam, lParam);
			ShowStatus(L"Device changed: %s",DriveLetter.GetString());
			break;
		}
		case WM_QUERYENDSESSION:
		{
			// Check `lParam` for which system shutdown function and handle events.
			// See https://docs.microsoft.com/windows/win32/shutdown/wm-queryendsession
			return TRUE; // Respect user's intent and allow shutdown.
		}
		case WM_ENDSESSION:
		{
			// Check `lParam` for which system shutdown function and handle events.
			// See https://docs.microsoft.com/windows/win32/shutdown/wm-endsession
			return 0; // We have handled this message.
		}
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
/////////////////////////////////////////////////////////////////////

void VideoSearch(CString path)
{
	int files_ok{ 0 }, files_err{ 0 }, files_no_audio{ 0 };
	int nfiles{ 0 };

	FindFileOptions_t opts;
	opts.filter = L"*.mp4;*.MP4;*.avi;*.AVI;*.mov;*.MOV;*.mpg;*.MPG;*.wmv;*.WMV;*.mts;*.MTS;*.lrv;*.LRV";
	opts.location = path.GetString();
	opts.recursive = true;
	opts.returnFolders = false;
	opts.terminateValue = FALSE;
	opts.logfunc = &ShowStatus;
	opts.listfunc = NULL;
	opts.listParam = NULL;

	m_pDBFindFile = new FindFile(GetForegroundWindow(), opts);
	m_pDBFindFile->search();
	SetEvent(ghSvcStopEvent);
	ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	while (-1)
	{
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		goto check_results;
	}
check_results:;
	m_pDBFindFile->cancel();
	nfiles = (int)m_pDBFindFile->filelist.size();
	if (nfiles)
		ShowStatus(L"Showing results from %d files found", nfiles);
	else
		ShowStatus(L"No files were found");

	for (int i = 0; i < nfiles; i++)
	{
		wstring path = m_pDBFindFile->filelist[i].path;
		wstring name = m_pDBFindFile->filelist[i].fileinfo.cFileName;
		wstring fullname = FindFile::combinePath(path, name);
		if ((i % 10) == 0) wprintf(L"File %d out of %d\n", i, nfiles);
		int result = VideoFileValid(fullname.c_str());
		switch (result)
		{
		case VIDEOVALIDATOR_VIDEO_OK:
			//					setcolor(LOG_COLOR_GREEN, 0);
			//					ShowStatus(L"File %s is good\n", fullname.c_str());
			//					setcolor(LOG_COLOR_WHITE,0);
			files_ok++;
			break;

		case VIDEOVALIDATOR_ERROR_OPENING:
			setcolor(LOG_COLOR_CIAN, 0);
			ShowStatus(L"File %s can't be opened\n", fullname.c_str());
			setcolor(LOG_COLOR_WHITE, 0);
			files_err++;
			break;

		case VIDEOVALIDATOR_CORRUPTED:
			setcolor(LOG_COLOR_CIAN, 0);
			ShowStatus(L"File %s is corrupted\n", fullname.c_str());
			setcolor(LOG_COLOR_WHITE, 0);
			files_err++;
			break;

		case VIDEOVALIDATOR_NO_AUDIO:
			setcolor(LOG_COLOR_YELLOW, 0);
			ShowStatus(L"File %s has no audio\n", fullname.c_str());
			setcolor(LOG_COLOR_WHITE, 0);
			files_no_audio++;
			break;

		case VIDEOVALIDATOR_CORRUPTED_NO_AUDIO:
			setcolor(LOG_COLOR_CIAN, 0);
			ShowStatus(L"File %s is corrupted and has no audio\n", fullname.c_str());
			setcolor(LOG_COLOR_WHITE, 0);
			files_err++;
			files_no_audio++;
			break;

		default:
			ShowStatus(L"File %s (unknown error)\n", fullname.c_str());
			break;
		}

	}
	ShowStatus(L"Completed %d files\nValid video files: \t\t\t%d\nInvalid files: \t\t\t%d\nNo Audio: \t\t\t%d",
		nfiles, files_ok, files_err, files_no_audio);
}

INT WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, INT)
{
//#ifdef _DEBUG
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	SetConsoleOutputCP(CP_UTF8);		// Output should support UNICODE
										// Currently doesn't work
//#endif
	setcolor(LOG_COLOR_YELLOW, 0);
	ShowStatus(L"Started\n");
	setcolor(LOG_COLOR_WHITE, 0);
	(void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (InitializeApp())
	{
		WNDCLASS sampleClass{ 0 };
		sampleClass.lpszClassName = TEXT("CtrlHandlerSampleClass");
		sampleClass.lpfnWndProc = WindowProc;

		if (!RegisterClass(&sampleClass))
		{
			printf("\nERROR: Could not register window class");
			return 2;
		}

		HWND hwnd = CreateWindowEx(
			0,
			sampleClass.lpszClassName,
			TEXT("Video Validator"),
			0,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			nullptr,
			nullptr,
			nullptr,
			nullptr
		);

		if (!hwnd)
		{
			ShowStatus(L"ERROR: Could not create window");
			return 3;
		}
		ShowWindow(hwnd, SW_HIDE);
		UpdateWindow(hwnd);

		MSG msg;
		int retV;
		while ((retV = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if (retV == -1)
			{
				//ErrorHandle
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		WCHAR szExeFileName[MAX_PATH];
		CString ExePath;
		GetModuleFileName(NULL, szExeFileName, MAX_PATH);
		ExePath = SG_GetBaseFilePath(szExeFileName);
		VideoSearch(ExePath);
	}
	
	CleanUp();
	system("pause");
	return 0;
}




//-------------------------------------------------------------------
// InitializeApp: Initializes the application.
//-------------------------------------------------------------------

BOOL InitializeApp()
{
	HRESULT hr = S_OK;

	// Initialize COM
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		// Initialize Media Foundation.
		hr = MFStartup(MF_VERSION);
	}


	return (SUCCEEDED(hr));
}


//-------------------------------------------------------------------
// Releases resources
//-------------------------------------------------------------------

void CleanUp()
{

	MFShutdown();
	CoUninitialize();
}





//-------------------------------------------------------------------
// VideoFileValid: Checks if a video file is valid or corrupted or no audio
//-------------------------------------------------------------------

int VideoFileValid(const WCHAR *sURL)
{
	return(g_VideoValidator.OpenFile(sURL));

}





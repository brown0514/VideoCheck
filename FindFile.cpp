//==========================================================================
/*

Secured Globe, Inc. 
FindFile

*/
//==========================================================================
#include "stdafx.h"
#include "FindFile.h"
#include "wildcard.h"

extern HANDLE ghSvcStopEvent;

FindFile::FindFile(HWND hWnd, FindFileOptions_t &opts)
{
	m_hWnd = hWnd;
    m_opts = opts;
	myThread = NULL;
}
//--------------------------------------------------------------------------

FindFile::~FindFile()
{
    clear();

}

//--------------------------------------------------------------------------

// Combines the two path's together and returns the combined path. This function
// eliminates the problem of combining directories when the left path may or
// may not contain a backslash. eg "c:\ and windows" or "c:\windows and system"
wstring FindFile::combinePath (wstring path1, wstring path2)
{
    if (path1.find_last_of(L"\\") != path1.length()-1 && path2 != L"")
        path1 += L"\\";
    path1 += path2;

    return path1;
}
//------------------------------------------------------------------------------

void FindFile::cancel()
{
	m_opts.terminateValue = TRUE;
}

// Re-initializes the FileFind object so that it can be reused without freeing
// and allocating memory every time
void FindFile::clear ()
{
    filelist.clear ();
    listsize = 0;

	if (myThread != NULL)
	{
		myThread->join();
		delete myThread;
		myThread = NULL;
	}
}
//---------------------------------------------------------------------------

// Searches the location directory for all files and returns
// true if more files may be available and false if that was the last one
bool FindFile::getFiles(HANDLE &searchHandle, WIN32_FIND_DATA &fileData, 
                         wstring path)
{
    int nValid;
	//m_opts.logfunc(L"getFiles initilized. Path = %s",path.c_str());

    if (searchHandle == NULL)
    {

        wstring pathToSearch = combinePath(path, L"*");

        searchHandle = FindFirstFile(pathToSearch.c_str(), &fileData);
        nValid = (searchHandle == INVALID_HANDLE_VALUE) ? 0 : 1;
    }
    else
    {
        nValid = FindNextFile(searchHandle, &fileData);
    }

    while (nValid)
    {
        // As long as this file is not . or .., we are done
        if (_wcsicmp(fileData.cFileName, L".") != 0 &&
			_wcsicmp(fileData.cFileName, L"..") != 0)
            return true;
		//m_opts.logfunc(L"getFiles continues...");

        nValid = FindNextFile(searchHandle, &fileData);
    }
	//m_opts.logfunc(L"getFiles ended");

    FindClose(searchHandle);
    searchHandle = NULL;

    return false;
}
//---------------------------------------------------------------------------

// Returns true if given file information matches requested criteria
bool FindFile::matchCriteria(WIN32_FIND_DATA &filedata)
{
    // Case 1. This is a directory. Check whether it is matched by the exclude
    // directory filter
    if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (wildcard::match(filedata.cFileName, m_opts.excludeDir))
            return false;
        return true;
    }

    // Case 2. This is a regular file

    // Check if it meets the filter
    if (!wildcard::match(filedata.cFileName, m_opts.filter))
        return false;

    // Check if it is in the exclude filter
    if (wildcard::match(filedata.cFileName, m_opts.excludeFile))
        return false;

    return true;
}


DWORD WINAPI FindFile::_ThreadProc(__in LPVOID lpParameter)
{
	wstring *s = (wstring*)lpParameter;
	scanPath(*s);

	if (m_hWnd != NULL)
		PostMessage(m_hWnd, WM_UPDATE, (WPARAM)filelist.size(), (LPARAM)listsize);
    SetEvent(ghSvcStopEvent);
	return 0;
}


//---------------------------------------------------------------------------

bool FindFile::HasCompleted()
{
	return (myThread->joinable());
}
// Finds all files as specified in the initial options
void FindFile::search ()
{
    clear();
	myThread = new std::thread(&FindFile::_ThreadProc, this, (LPVOID)&m_opts.location);//&t;
}
//---------------------------------------------------------------------------

// Scans a path for files as specified in the filter and stores them in the
// file list array. If a recursive options was specified, scanPath will
// continue to search for files in all subdirectories.
void FindFile::scanPath(wstring path)
{
    WIN32_FIND_DATA fileData;
    FileInformation fi;

    HANDLE searchHandle = NULL;

	//m_opts.logfunc(L"scanPath start '%s'", path.c_str());
    while (getFiles (searchHandle, fileData, path))
    {
		Sleep(1);
		//m_opts.logfunc(L"scanPath '%s'", path.c_str());

        // Abort on termination signal
        if (m_opts.terminateValue)
            break;

        // Skip this file/directory if not matching criteria
        if (!matchCriteria(fileData))
            continue;

        // If recursive option is set and this is a directory, then search in 
        // there too
        if (m_opts.recursive && 
            fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            scanPath (combinePath (path, fileData.cFileName));


		// If this is a directory and we don't wish to return directories, 
        // continue
        if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
            !m_opts.returnFolders)
            continue;

        fi.fileinfo = fileData;
        fi.path = path;
		//m_opts.listfunc((path+fi.fileinfo.cFileName).c_str(), m_opts.listParam);

        filelist.push_back (fi);
        listsize += fileData.nFileSizeLow + fileData.nFileSizeHigh * MAXDWORD;

		//m_opts.logfunc(L"%s %s",path.c_str(), fileData.cFileName);
		

	}
}
//---------------------------------------------------------------------------


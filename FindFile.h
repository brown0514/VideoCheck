//==========================================================================
/*

(c)2022 Secured GLobe, Inc.
*/
//==========================================================================

#define WM_UPDATE WM_USER + 0x400



// Specifies settings to use for searching for files
struct FindFileOptions_t
{
    bool recursive;         // Whether to look inside subdirectories
    bool returnFolders;     // Return folder names as results too

    bool terminateValue;   // Value to check to see whether search should be
                            // terminated

    wstring location;        // Where to search for files

    wstring filter;          // Filter for files to be included

    wstring excludeFile;     // Exclude filter for files
    wstring excludeDir;      // Exclude filter for directories

	logFunc logfunc;		 // A function to update

	listFunc listfunc;		 // A function to update
	void *listParam;		// Param attach to update function
};

// Holds information on a found file
struct FileInformation
{
    WIN32_FIND_DATA fileinfo;
    wstring path;
};

// A list of found files
typedef vector<FileInformation> FileList_t;


class FindFile
{
private:

	HWND m_hWnd;
    FindFileOptions_t m_opts;

    // Finds a single file and returns true if there are more to come
    bool getFiles(HANDLE &searchHandle, WIN32_FIND_DATA &filedata, wstring path);

    // Returns true if given file information matches requested criteria
    bool matchCriteria(WIN32_FIND_DATA &filedata);

	std::thread *myThread;


public:

    FileList_t filelist;        // List of files found in search
    __int64 listsize;           // Size in bytes of all files in found list

    FindFile(HWND hWnd, FindFileOptions_t &opts);
    ~FindFile ();

	void cancel();

    // Clears list of found files, file handle and so on
    void clear();

	DWORD WINAPI _ThreadProc(__in LPVOID lpParameter);


	bool HasCompleted();

    // Finds all files as specified in the initial options
    void search ();

    // Scans a path for files as according to 
    void scanPath(wstring path);

    // Concatenates 2 paths
    static wstring combinePath(wstring path1, wstring path2);

};


#pragma once
#include <windows.h>
#include <windowsx.h>

#include <strsafe.h>
#include <sstream>
#include <string>
#include <stdio.h>
#include <thread>
#include <vector>
#include <tchar.h> 
#include <strsafe.h>
#include <wchar.h>
#include <atlbase.h>
#include <atlstr.h>
#include <fcntl.h>
#include <io.h>
// Media Foundation 
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

// Shell
#include <shobjidl.h>
#include <shellapi.h> 

// Direct2D
#include <D2d1.h>
#include <D2d1helper.h>
#include <initguid.h>
#include <Usbiodef.h>

// Misc
#include <strsafe.h>
#include <assert.h>
#include <propvarutil.h>
#include <atltime.h>
#include <dbt.h>


using namespace std;

typedef void(*logFunc)(LPCWSTR lpText, ...);
typedef void(*listFunc)(LPCWSTR lpText, void *param);

//==========================================================================
/*

Freelancer Project - by Michael Haephrati, Secured Globe, Inc.


May 14, 2020

The wildcard part is based on an old Code Project article which was converted to support
UNICODE. The Repository of the updated code is: https://github.com/haephrati/FindFileWin

*/
//==========================================================================

#include <string>

using namespace std;

class wildcard
{
private:

    // check string for substring
    static bool find(const wchar_t *s1,const wchar_t *s2);

    // replacement for strncmp, allows for '?'
    static bool wc_cmp(const wchar_t *s1,const wchar_t *s2,int len);

    static bool EndCmp(wchar_t *src,const wchar_t *cmp);

    static bool StartCmp(wchar_t *src,const wchar_t *cmp);


public:

    // Allows multiple filters separated by ';'
    static bool match (wstring file, wstring filter);
    
    // the main wildcard compare function
    static bool wildcmp(wstring _findstr, wstring _cmp);
};




 

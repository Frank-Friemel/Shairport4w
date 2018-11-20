/*
 *
 *  Utils.h
 *
 */

#pragma once

#include <time.h>
#include <atlsync.h>
#include <comdef.h>
#include <regex>

#include "myMutex.h"

#ifndef MAKEULONGLONG
#define MAKEULONGLONG(ldw, hdw) ((ULONGLONG(hdw) << 32) | ((ldw) & 0xffffffff))
#endif

#ifndef LODWORD
#define LODWORD(l)           ((DWORD)(((ULONGLONG)(l)) & 0xffffffff))
#define HIDWORD(l)           ((DWORD)((((ULONGLONG)(l)) >> 32) & 0xffffffff))
#endif

extern	BOOL		bLogToFile;
extern	BOOL		bLogToConsole;
extern	std::string		strConfigName;

bool GetValueFromRegistry(HKEY hKey, PCSTR pValueName, std::string& strValue, PCSTR strKeyPath = NULL);
bool PutValueToRegistry(HKEY hKey, PCSTR pValueName, PCSTR pValue, PCSTR strKeyPath = NULL);
bool RemoveValueFromRegistry(HKEY hKey, PCSTR pValueName, PCSTR strKeyPath = NULL);
bool RemoveKeyFromRegistry(HKEY hKey, PCWSTR pKeyPath);
CComVariant GetValueFromRegistry(HKEY hKey, PCWSTR pValueName, CComVariant varDefault = CComVariant(), PCWSTR strKeyPath = NULL);
bool PutValueToRegistry(HKEY hKey, PCWSTR pValueName, CComVariant varValue, PCWSTR strKeyPath = NULL);

std::wstring ToString(const VARIANT& var);

std::wstring ErrorToString(ULONG err = ::GetLastError(), PCTSTR lpszFunction = NULL);
void ErrorMsg(HWND hwnd, PCTSTR lpszTitle, PCTSTR lpszFunction, DWORD dwErr);
int  ErrorMsg(HWND hwnd, HRESULT hrErr);


std::string MD5_Hex(const BYTE* pBlob, long sizeOfBlob, bool bUppercase = false);

inline HANDLE DuplicateHandle(HANDLE h)
{
    HANDLE hResult = NULL;

    if (!::DuplicateHandle(::GetCurrentProcess(), h, ::GetCurrentProcess(), &hResult, 0, FALSE, DUPLICATE_SAME_ACCESS))
        return NULL;
    return hResult;
}

bool Log(PCSTR pszFormat, ...);

#define _LOG(...)						\
	{ if (bLogToConsole || bLogToFile)	\
		Log(__VA_ARGS__);				\
	}

#ifdef _WS2IPDEF_

std::string IPAddrToString(struct sockaddr_in* in);
std::wstring IPAddrToStringW(struct sockaddr_in* in);

bool StringToIPAddr(const char* str, struct sockaddr_in* in, int laddr_size);
bool StringToIPAddrW(PCWSTR str, struct sockaddr_in* in, int laddr_size);

bool GetPeerIP(SOCKET sd, bool bV4, struct sockaddr_in* in);
void GetPeerIP(SOCKET sd, bool bV4, std::string& strIP);
void GetLocalIP(SOCKET sd, bool bV4, std::string& strIP);
int GetLocalIP(SOCKET sd, bool bV4, CTempBuffer<BYTE>& pIP);
bool GetPeerName(const char* strIP, bool bV4, std::wstring& strHostName);

#endif

bool FileExists(PCWSTR strPath, ATL::CString* pStrResult = NULL);
std::wstring GetModulePath(HMODULE hModule = NULL, PCWSTR strModuleName = NULL);

bool GetVersionInfo(PCWSTR pszFileName, LPWSTR strVersion, ULONG nBufSizeInChars);

#ifdef __ATLCTRLS_H__

Bitmap* BitmapFromResource(HINSTANCE hInstance, LPCTSTR strID, LPCTSTR strType = _T("PNG"));
Bitmap* BitmapFromBlob(const BYTE* p, long nBytes);

#endif

ULONGLONG GetNTPTimeStamp();
USHORT GetUniquePortNumber();
ULONG CreateRand(ULONG nMin = 0, ULONG nMax = MAXDWORD);

ULONGLONG ToDacpID(PCSTR strID);

inline ATL::CString GetLanguageAbbr()
{
	ATL::CString strLang = L"en";

	LANGID lid = GetUserDefaultLangID();

	switch(PRIMARYLANGID(lid))
	{
		case LANG_GERMAN:
		{
			strLang = L"de";
		}
		break;
	}
	return strLang;
}

struct ic_char_traits : public std::char_traits<char>
{
    static bool eq(char c1, char c2)
    {
        return ::tolower(c1) == ::tolower(c2);
    }
    static bool ne(char c1, char c2)
    {
        return ::tolower(c1) != ::tolower(c2);
    }
    static bool lt(char c1, char c2)
    {
        return ::tolower(c1) < ::tolower(c2);
    }
    static int compare(const char* s1, const char* s2, size_t n)
    {
        return _memicmp(s1, s2, n);
    }
    static const char* find(const char* first, size_t count, const char ch)
    {
        while (count--)
        {
            if (::tolower(*first) == ::tolower(ch))
            {
                return first;
            }
            ++first;
        }
        return NULL;
    }
};

typedef std::basic_string<char, ic_char_traits, std::allocator<char>> ic_string;

__inline const wchar_t* wmemichr(const wchar_t* first, wchar_t ch, size_t count)
{
    while (count--)
    {
        if (::tolower(*first) == ::tolower(ch))
        {
            return first;
        }
        ++first;
    }
    return NULL;
}

__inline int wmemicmp(const wchar_t* first, const wchar_t* second, size_t count)
{
    while (count--)
    {
        if (::towlower(*first) != ::towlower(*second))
        {
            return ::towlower(*first) < ::towlower(*second) ? -1 : 1;
        }
        ++first;
        ++second;
    }
    return 0;
}

struct ic_wchar_traits : public std::char_traits<wchar_t>
{
    static bool eq(wchar_t c1, wchar_t c2)
    {
        return ::towlower(c1) == ::towlower(c2);
    }
    static bool ne(wchar_t c1, wchar_t c2)
    {
        return ::towlower(c1) != ::towlower(c2);
    }
    static bool lt(wchar_t c1, wchar_t c2)
    {
        return ::towlower(c1) < ::towlower(c2);
    }
    static int compare(const wchar_t* s1, const wchar_t* s2, size_t n)
    {
        return wmemicmp(s1, s2, n);
    }
    static const wchar_t* find(const wchar_t* first, size_t count, const wchar_t ch)
    {
        return wmemichr(first, ch, count);
    }
};

typedef std::basic_string<wchar_t, ic_wchar_traits, std::allocator<wchar_t>> ic_wstring;


template <class T>
void TrimRight(T& s, typename T::const_pointer trimSet)
{
    T::size_type n = s.find_last_not_of(trimSet);

    if (n != std::string::npos)
        s = s.substr(0, n + 1);
    else
        s.erase();
}

template <class T>
void TrimLeft(T& s, typename T::const_pointer trimSet)
{
    T::size_type n = s.find_first_not_of(trimSet);

    if (n != std::string::npos)
        s = s.substr(n);
    else
        s.erase();
}

template <class T>
void Trim(T& s, typename T::const_pointer trimSet)
{
    TrimRight(s, trimSet);
    TrimLeft(s, trimSet);
}

template<class T>
inline typename T::const_pointer trimSetWhiteSpaces(T s = T())
{
    UNREFERENCED_PARAMETER(s);
    return sizeof(T::value_type) == sizeof(WCHAR) ? (typename T::const_pointer) L"\t \n\r" : (typename T::const_pointer) "\t \n\r";
}

template <class T>
inline void TrimRight(T& s)
{
    return TrimRight(s, trimSetWhiteSpaces(s));
}

template <class T>
inline void TrimLeft(T& s)
{
    return TrimLeft(s, trimSetWhiteSpaces(s));
}

template <class T>
inline void Trim(T& s)
{
    return Trim(s, trimSetWhiteSpaces(s));
}


template<class _Pr>
bool ParseRegEx(std::string strToParse, PCSTR strRegExp, _Pr _Pred)
{
    std::regex e(strRegExp);

    std::sregex_token_iterator i(strToParse.cbegin(), strToParse.cend(), e);
    std::sregex_token_iterator end;

    if (i != end)
    {
        do
        {
            if (!_Pred(*i))
                break;
        } while (++i != end);

        return true;
    }
    return false;
}

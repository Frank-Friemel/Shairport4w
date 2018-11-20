/*
 *
 *  utils.cpp
 *
 */

#include "stdafx.h"
#include <Strsafe.h>
#include <ATLComTime.h>

#include "utils.h"
#include "myCrypt.h"

typedef DWORD (APIENTRY *_GetFileVersionInfoSizeW)(
        PCWSTR lptstrFilename, /* Filename of version stamped file */
        LPDWORD lpdwHandle
        );    
typedef BOOL (APIENTRY *_GetFileVersionInfoW)(
        PCWSTR lptstrFilename, /* Filename of version stamped file */
        DWORD dwHandle,         /* Information from GetFileVersionSize */
        DWORD dwLen,            /* Length of buffer for info */
        LPVOID lpData
        );           

typedef BOOL (APIENTRY *_VerQueryValueW)(
        const LPVOID pBlock,
        LPWSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );


static CMyMutex		c_mtxLOG;
static USHORT		c_nUniquePortCounter	= 6000;
static CMyMutex		c_mtxUniquePortCounter;

std::string			strConfigName(APP_NAME_A);
BOOL			    bLogToFile		= FALSE;
BOOL			    bLogToConsole	= FALSE;

bool Log(PCSTR pszFormat, ...)
{
	if (bLogToConsole || bLogToFile)
	{
		size_t	nBuf	= 32768;
		char*	buf		= (char*)malloc(nBuf);
		bool	bResult = true;

		va_list ptr;
		va_start(ptr, pszFormat);

		c_mtxLOG.Lock();

		try
		{
			vsprintf_s(buf, nBuf, pszFormat, ptr);
			OutputDebugStringA(buf);
			va_end(ptr);

			if (bLogToFile)
			{
				char FilePath[MAX_PATH];
		
				ATLVERIFY(GetModuleFileNameA(NULL, FilePath, sizeof(FilePath) / sizeof(char)) > 0);
				FilePath[strlen(FilePath) - 3] ='l';
				FilePath[strlen(FilePath) - 2] ='o';
				FilePath[strlen(FilePath) - 1] ='g';

				FILE* fh = NULL;
					
				if (fopen_s(&fh, FilePath, "a") == ERROR_SUCCESS)
				{
					time_t szClock;

					time(&szClock);
					struct tm	newTime;
					localtime_s(&newTime, &szClock);

					strftime(FilePath, MAX_PATH, "%x %X ", &newTime);
					fwrite(FilePath, sizeof(char), strlen(FilePath), fh);
					fwrite(buf, sizeof(char), strlen(buf), fh);
					fclose(fh);
				}
				else
				{
					bResult = false;
				}
			}
		}
		catch (...)
		{
			// catch that silently
			bResult = false;
		}
		c_mtxLOG.Unlock();

		return bResult;
	}
	return true;
}

bool GetValueFromRegistry(HKEY hKey, PCSTR pValueName, std::string& strValue, PCSTR strKeyPath /*= NULL*/)
{
	HKEY	h		= NULL;
	bool	bResult = false;
	std::string	strKeyName("Software\\");

	strKeyName += strConfigName;

	if (strKeyPath)
		strKeyName = strKeyPath;

	if (RegOpenKeyExA(hKey, strKeyName.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS)
	{
		DWORD dwSize = 0;

		if (RegQueryValueExA(h, pValueName, NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS)
		{
			dwSize += 2;

			LPBYTE pBuf = new BYTE[dwSize];

			memset(pBuf, 0, dwSize);

			if (RegQueryValueExA(h, pValueName, NULL, NULL, pBuf, &dwSize) == ERROR_SUCCESS)
			{
				strValue	= (PCSTR)pBuf;
				bResult		= true;
			}
			delete pBuf;
		}
		RegCloseKey(h);
	}
	return bResult;
}

bool PutValueToRegistry(HKEY hKey, PCSTR pValueName, PCSTR pValue, PCSTR strKeyPath /*= NULL*/)
{
	HKEY	h;
	DWORD	disp	= 0;
	bool	bResult	= false;
	std::string	strKeyName("Software\\");

	strKeyName += strConfigName;
	
	if (strKeyPath)
		strKeyName = strKeyPath;

	if (RegCreateKeyExA(hKey, strKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &h, &disp) == ERROR_SUCCESS)
	{
		bResult = RegSetValueExA(h, pValueName, 0, REG_SZ, (CONST BYTE *)pValue, ((unsigned long)strlen(pValue)+1) * sizeof(char)) == ERROR_SUCCESS;
		RegCloseKey(h);
	}
	return bResult;
}

bool RemoveValueFromRegistry(HKEY hKey, PCSTR pValueName, PCSTR strKeyPath /*= NULL*/)
{
	HKEY	h;
	DWORD	disp	= 0;
	bool	bResult	= false;
	std::string	strKeyName("Software\\");

	strKeyName += strConfigName;
	
	if (strKeyPath)
		strKeyName = strKeyPath;

	if (RegCreateKeyExA(hKey, strKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &h, &disp) == ERROR_SUCCESS)
	{
		bResult = RegDeleteValueA(h, pValueName) == ERROR_SUCCESS;
		RegCloseKey(h);
	}
	return bResult;
}

typedef	DWORD (WINAPI *typeSHDeleteKeyW)(HKEY, PCWSTR);

bool RemoveKeyFromRegistry(HKEY hKey, PCWSTR pKeyPath)
{
	bool bResult = false;

	if (RegDeleteKeyW(hKey, pKeyPath) == ERROR_SUCCESS)
	{
		bResult = true;
	}
	else
	{
		HMODULE hSHLWAPILib = LoadLibraryW(L"SHLWAPI.DLL");
			  
		if (hSHLWAPILib)
		{
			typeSHDeleteKeyW pSHDeleteKey = (typeSHDeleteKeyW) GetProcAddress(hSHLWAPILib, "SHDeleteKeyW");

			if (pSHDeleteKey)
			{
				bResult = pSHDeleteKey(hKey, pKeyPath) == ERROR_SUCCESS ? true : false;
			}
			FreeLibrary(hSHLWAPILib);
		}
	}
	return bResult;
}

CComVariant GetValueFromRegistry(HKEY hKey, PCWSTR pValueName, CComVariant varDefault /*= CComVariant()*/, PCWSTR strKeyPath /*= NULL*/)
{
	USES_CONVERSION;

	HKEY		h			= NULL;
	CComVariant	varValue	= varDefault;
	std::wstring		strKeyName(L"Software\\");

	strKeyName += A2CW(strConfigName.c_str());

	if (strKeyPath)
		strKeyName = strKeyPath;

	if (RegOpenKeyExW(hKey, strKeyName.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS)
	{
		DWORD dwSize	= 0;
		DWORD dwType	= 0;

		if (RegQueryValueExW(h, pValueName, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
		{
			switch (dwType)
			{
				case REG_NONE:
				{
					varValue.Clear();
				}
				break;

				case REG_DWORD:
				{
					varValue.Clear();
					ATLASSERT(dwSize == sizeof(varValue.ulVal));

					if (RegQueryValueExW(h, pValueName, NULL, NULL, (LPBYTE)&varValue.ulVal, &dwSize) == ERROR_SUCCESS)
					{
						varValue.vt = VT_UI4;

						if (varDefault.vt != VT_EMPTY && varDefault.vt != varValue.vt)
						{
							ATLVERIFY(SUCCEEDED(varValue.ChangeType(varDefault.vt)));
						}
					}
					else
					{
						ATLASSERT(FALSE);
						varValue = varDefault;
					}
				}
				break;

				case REG_QWORD:
				{
					varValue.Clear();
					ATLASSERT(dwSize == sizeof(varValue.ullVal));

					if (RegQueryValueExW(h, pValueName, NULL, NULL, (LPBYTE)&varValue.ullVal, &dwSize) == ERROR_SUCCESS)
					{
						varValue.vt = VT_UI8;

						if (varDefault.vt != VT_EMPTY && varDefault.vt != varValue.vt)
						{
							ATLVERIFY(SUCCEEDED(varValue.ChangeType(varDefault.vt)));
						}
					}
					else
					{
						ATLASSERT(FALSE);
						varValue = varDefault;
					}
				}
				break;

				case REG_EXPAND_SZ:
				case REG_SZ:
				{
					dwSize += 2;

					LPBYTE pBuf = new BYTE[dwSize];

					if (pBuf)
					{
						memset(pBuf, 0, dwSize);

						if (RegQueryValueExW(h, pValueName, NULL, NULL, pBuf, &dwSize) == ERROR_SUCCESS)
						{
							varValue = (PCWSTR)pBuf;
						}
						else
						{
							ATLASSERT(FALSE);
						}
						delete pBuf;
					}
				}
				break;

				case REG_BINARY:
				{
					LPBYTE pBuf = new BYTE[dwSize];

					if (pBuf)
					{
						if (RegQueryValueExW(h, pValueName, NULL, NULL, pBuf, &dwSize) == ERROR_SUCCESS)
						{
							HGLOBAL hBlob = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, dwSize);  

							if (hBlob)
							{
								IStreamPtr pStream;

								if (SUCCEEDED(::CreateStreamOnHGlobal(hBlob, TRUE, &pStream)))
								{
									varValue.Clear();

									ULONG nWritten = 0;
									pStream->Write(pBuf, dwSize, &nWritten);
									ATLASSERT(dwSize == nWritten);
								
									LARGE_INTEGER	li0;											
									memset(&li0, 0, sizeof(li0));								
									pStream->Seek(li0, STREAM_SEEK_SET, NULL);
	
									if (SUCCEEDED(pStream->QueryInterface(&varValue.punkVal)))
										varValue.vt = VT_UNKNOWN;
								}
								else
								{
									GlobalFree(hBlob);
								}
							}
						}
						else
						{
							ATLASSERT(FALSE);
						}
						delete pBuf;
					}
				}
				break;
					
				default:
				{
					ATLASSERT(FALSE);
				}
				break;
			}
		}
		RegCloseKey(h);
	}
	return varValue;
}

bool PutValueToRegistry(HKEY hKey, PCWSTR pValueName, CComVariant varValue, PCWSTR strKeyPath /*= NULL*/)
{
	USES_CONVERSION;

	HKEY	h;
	DWORD	disp	= 0;
	bool	bResult	= false;
	std::wstring	strKeyName(L"Software\\");

	strKeyName += A2CW(strConfigName.c_str());
	
	if (strKeyPath)
		strKeyName = strKeyPath;

	if (RegCreateKeyExW(hKey, strKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &h, &disp) == ERROR_SUCCESS)
	{
		switch (varValue.vt)
		{
			case VT_EMPTY:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_NONE, NULL, 0) == ERROR_SUCCESS;
			}
			break;

			case VT_BSTR:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_SZ, (CONST BYTE *)varValue.bstrVal, ((unsigned long)wcslen(varValue.bstrVal)+1) * sizeof(WCHAR)) == ERROR_SUCCESS;
			}
			break;

			case VT_I4:
			case VT_UI4:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_DWORD, (CONST BYTE *)&varValue.ulVal, sizeof(varValue.ulVal)) == ERROR_SUCCESS;
			}
			break;

			case VT_I8:
			case VT_UI8:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_QWORD, (CONST BYTE *)&varValue.ullVal, sizeof(varValue.ullVal)) == ERROR_SUCCESS;
			}
			break;

			case VT_BOOL:
			{
				ULONG ulValue = varValue.boolVal != VARIANT_FALSE ? TRUE : FALSE;
				bResult = RegSetValueExW(h, pValueName, 0, REG_DWORD, (CONST BYTE *)&ulValue, sizeof(ulValue)) == ERROR_SUCCESS;
			}
			break;

			case VT_UNKNOWN:
			case VT_DISPATCH:
			{
				IStreamPtr pStream;

				if (SUCCEEDED(varValue.punkVal->QueryInterface(&pStream)))
				{
					ULONG	nLen = 0;

					LARGE_INTEGER	li0;											
					memset(&li0, 0, sizeof(li0));								
				
					ULARGE_INTEGER	liPos;										

					if (SUCCEEDED(pStream->Seek(li0, STREAM_SEEK_END, &liPos)))
					{																
						nLen = liPos.u.LowPart;									
						pStream->Seek(li0, STREAM_SEEK_SET, NULL);
					}																
					if (nLen > 0)
					{
						LPBYTE pBuf = new BYTE[nLen];

						if (pBuf)
						{
							ULONG nRead = 0;

							ATLVERIFY(SUCCEEDED(pStream->Read(pBuf, nLen, &nRead)));
							ATLASSERT(nRead == nLen);
							bResult = RegSetValueExW(h, pValueName, 0, REG_BINARY, pBuf, nRead) == ERROR_SUCCESS ? true : false;

							delete pBuf;
						}
					}
					else
						bResult = RegSetValueExW(h, pValueName, 0, REG_BINARY, NULL, 0) == ERROR_SUCCESS ? true : false;
				}
			}
			break;
		
			default:
			{
				ATLASSERT(FALSE);
			}
			break;
		}
		RegCloseKey(h);
	}
	return bResult;
}

std::wstring ToString(const VARIANT& var)
{
	std::wstring strResult;

	switch(var.vt)
	{
		case VT_EMPTY:
		case VT_NULL:
		break;

		case VT_BSTR:
		{
			USES_CONVERSION;
			strResult = OLE2CW(var.bstrVal);
		}
		break;

		case VT_I1:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%ld", (LONG)var.cVal);
			strResult = buf;
		}
		break;

		case VT_UI1:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%lu", (ULONG)var.bVal);
			strResult = buf;
		}
		break;

		case VT_I2:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%ld", (LONG)var.iVal);
			strResult = buf;
		}
		break;

		case VT_UI2:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%lu", (ULONG)var.uiVal);
			strResult = buf;
		}
		break;

		case VT_I4:
		case VT_INT:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%ld", (LONG)var.lVal);
			strResult = buf;
		}
		break;

		case VT_UI4:
		case VT_UINT:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%lu", (ULONG)var.ulVal);
			strResult = buf;
		}
		break;

		case VT_I8:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%I64d", var.llVal);
			strResult = buf;
		}
		break;

		case VT_UI8:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%I64u", var.ullVal);
			strResult = buf;
		}
		break;

		case VT_BOOL:
		{
			strResult = var.boolVal != VARIANT_FALSE ? L"true" : L"false";
		}
		break;

		case VT_R8:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%lf", var.dblVal);
			strResult = buf;
		}
		break;

		case VT_R4:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"%lf", (double)var.fltVal);
			strResult = buf;
		}
		break;

		case VT_DISPATCH:
		case VT_UNKNOWN:
		{
			WCHAR buf[64];
			swprintf_s(buf, 64, L"Pointer: %p", var.punkVal);
			strResult = buf;
		}
		break;

		case VT_VARIANT | VT_BYREF:
		{
			if (var.pvarVal)
				strResult = ToString(*var.pvarVal);
		}
		break;

		case VT_DATE:
		{
			SYSTEMTIME sysTime;
			
			if (::VariantTimeToSystemTime(var.date, &sysTime))
			{
				ATL::CTime time(sysTime);

				try
				{
					__time64_t t64 = time.GetTime();
					tm tmTemp;
					errno_t err = _localtime64_s(&tmTemp, &t64);

					WCHAR buf[64];

					if (err == 0 && wcsftime(buf, 64, L"%x %H:%M", &tmTemp))
						strResult = buf;
				}
				catch(...)
				{
					strResult.clear();
				}
			}
		}
		break;

		default:
		{
			CComVariant _var;

			if (SUCCEEDED(::VariantCopyInd(&_var, &var)))
			{
				if (SUCCEEDED(_var.ChangeType(VT_BSTR)))
				{
					USES_CONVERSION;
					strResult = OLE2CW(_var.bstrVal);
				}
			}
		}
		break;
	}
	return strResult;
}


std::string MD5_Hex(const BYTE* pBlob, long sizeOfBlob, bool bUppercase /*= false*/)
{
    std::string hex;
    CTempBuffer<BYTE> r;

    my_crypt::Hash hash(BCRYPT_MD5_ALGORITHM);

    ATLVERIFY(hash.Open());
    ATLVERIFY(hash.Update(pBlob, sizeOfBlob));

    const ULONG n = hash.Commit(r);

    if (n)
    {
        char buf[64];
        ULONG i = 0;

        do
        {
            if (bUppercase)
                sprintf_s(buf, 64, "%02lX", (ULONG)*(r + i));
            else
                sprintf_s(buf, 64, "%02lx", (ULONG)*(r + i));

            hex += buf;

        } while (++i < n);
    }
    else
    {
        ATLASSERT(FALSE);
    }
    return hex;
}

#ifdef _WS2IPDEF_

std::string IPAddrToString(struct sockaddr_in* in)
{
	USES_CONVERSION;
	return W2CA(IPAddrToStringW(in).c_str());
}

std::wstring IPAddrToStringW(struct sockaddr_in* in)
{
	WCHAR	buf[256];
	std::wstring	strResult = L"";

	switch(in->sin_family)
	{
		case AF_INET:
		{
			swprintf_s(buf, 256, L"%ld.%ld.%ld.%ld", (long)in->sin_addr.S_un.S_un_b.s_b1
											, (long)in->sin_addr.S_un.S_un_b.s_b2
											, (long)in->sin_addr.S_un.S_un_b.s_b3
											, (long)in->sin_addr.S_un.S_un_b.s_b4);
			strResult = buf;
		}
		break;

		case AF_INET6:
		{
			DWORD dwLen = sizeof(buf) / sizeof(buf[0]);

			if (WSAAddressToStringW((LPSOCKADDR)in, sizeof(struct sockaddr_in6), NULL, buf, &dwLen) == 0)
			{
				buf[dwLen] = 0;
				strResult = buf;
			}
		}
		break;
	}
	return strResult;
}

bool StringToIPAddr(const char* str, struct sockaddr_in* in, int laddr_size)
{
	USES_CONVERSION;
	return StringToIPAddrW(A2CW(str), in, laddr_size);
}

bool StringToIPAddrW(PCWSTR str, struct sockaddr_in* in, int laddr_size)
{
	bool bResult = false;

	if (str && wcslen(str) > 0)
	{
		ULONG	nBufLen		= (ULONG)(wcslen(str) + 1);
		WCHAR*	pBuf		= new WCHAR[nBufLen];

		if (pBuf)
		{
			wcscpy_s(pBuf, nBufLen, str);

			if (laddr_size == sizeof(struct sockaddr_in))
			{
				INT dwSize = laddr_size;

				bResult = WSAStringToAddressW(pBuf, AF_INET, NULL, (LPSOCKADDR)in, &dwSize) == 0 ? true : false;
			}
			else
			{
				ATLASSERT(laddr_size == sizeof(struct sockaddr_in6));

				INT dwSize = laddr_size;

				bResult = WSAStringToAddressW(pBuf, AF_INET6, NULL, (LPSOCKADDR)in, &dwSize) == 0 ? true : false;
			}
			delete pBuf;
		}
	}
	return bResult;
}

bool GetPeerIP(SOCKET sd, bool bV4, struct sockaddr_in* in)
{
	int laddr_size = sizeof(struct sockaddr_in);

	if (!bV4)
	{
		laddr_size = sizeof(struct sockaddr_in6);
	}
	return (0 == getpeername(sd, (sockaddr *)in, &laddr_size)) ? true : false;
}

void GetPeerIP(SOCKET sd, bool bV4, std::string& strIP)
{
	if (bV4)
	{
		struct sockaddr_in	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getpeername(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString(&laddr_in);
	}
	else
	{
		struct sockaddr_in6	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getpeername(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString((struct sockaddr_in*)&laddr_in);
	}
}

bool GetPeerName(const char* strIP, bool bV4, std::wstring& strHostName)
{
	if (bV4)
	{
		struct sockaddr_in	laddr_in;

		if (StringToIPAddr(strIP, (struct sockaddr_in*)&laddr_in, sizeof(struct sockaddr_in)))
		{
			WCHAR buf[NI_MAXHOST];

			if (GetNameInfoW((const SOCKADDR *)&laddr_in, sizeof(struct sockaddr_in), buf, _countof(buf), NULL, 0, NI_NAMEREQD|NI_NOFQDN) == 0)
			{
				strHostName = buf;
				return true;
			}
		}
	}
	else
	{
		std::string _strIP;

		if (strIP[0] == '[')
		{
			_strIP = strIP + 1;

			std::string::size_type n = _strIP.find('%');

			if (n != std::string::npos)
			{
				_strIP = _strIP.substr(0, n);
			}
			else
			{
				n = _strIP.find(']');

				if (n != std::string::npos)
				{
					_strIP = _strIP.substr(0, n);
				}
				else
					_strIP.clear();
			}
		}
		struct sockaddr_in6	laddr_in;

		if (	StringToIPAddr(strIP, (struct sockaddr_in*)&laddr_in, sizeof(struct sockaddr_in6))
			||	(!_strIP.empty() && StringToIPAddr(_strIP.c_str(), (struct sockaddr_in*)&laddr_in, sizeof(struct sockaddr_in6)))
			)
		{
			WCHAR buf[NI_MAXHOST];

			if (GetNameInfoW((const SOCKADDR *)&laddr_in, sizeof(struct sockaddr_in6), buf, _countof(buf), NULL, 0, NI_NAMEREQD|NI_NOFQDN) == 0)
			{
				strHostName = buf;
				return true;
			}
		}
	}
	return false;
}

void GetLocalIP(SOCKET sd, bool bV4, std::string& strIP)
{
	if (bV4)
	{
		struct sockaddr_in	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString(&laddr_in);
	}
	else
	{
		struct sockaddr_in6	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString((struct sockaddr_in*)&laddr_in);
	}
}

int GetLocalIP(SOCKET sd, bool bV4, CTempBuffer<BYTE>& pIP)
{
	int nResult = 0;

	if (bV4)
	{
		pIP.Allocate(4);

		struct sockaddr_in	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
		{
			nResult = 4;
			memcpy(pIP, &laddr_in.sin_addr.s_addr, nResult);
		}
	}
	else
	{
		pIP.Allocate(16);

		struct sockaddr_in6	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
		{
			nResult = 16;
			memcpy(pIP, &laddr_in.sin6_addr, nResult);
		}
	}
	return nResult;
}


#endif // _WS2IPDEF_

#ifdef __ATLCTRLS_H__

Bitmap* BitmapFromBlob(const BYTE* p, long nBytes)
{
	ATLASSERT(p && nBytes > 0);

	Bitmap* pResult = 0;

	HGLOBAL hBlob = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, nBytes);  

	if (hBlob)
	{
		IStream* pStream = NULL;

		if (SUCCEEDED(CreateStreamOnHGlobal(hBlob, TRUE, &pStream)))
		{
			ULONG nWritten = 0;
			pStream->Write(p, nBytes, &nWritten);
			ATLASSERT(nBytes == nWritten);

			pResult	= Bitmap::FromStream(pStream);

			pStream->Release();
		}
		else
		{
			GlobalFree(hBlob);
		}
	}
	return pResult;
}

Bitmap* BitmapFromResource(HINSTANCE hInstance, LPCTSTR strID, LPCTSTR strType /* = _T("PNG") */)
{
	Bitmap* pResult		= NULL;
	HRSRC	hResource	= FindResource(hInstance, strID, strType);

	if (hResource)
	{
		DWORD	nSize			= SizeofResource(hInstance, hResource);
	    HANDLE	hResourceMem	= LoadResource(hInstance, hResource);
	    BYTE*	pData			= (BYTE*)LockResource(hResourceMem);

		if (pData)
		{
			HGLOBAL hBuffer  = GlobalAlloc(GMEM_MOVEABLE, nSize);

			if (hBuffer) 
			{
				void* pBuffer = GlobalLock(hBuffer);
				
				if (pBuffer)
				{
					CopyMemory(pBuffer, pData, nSize);
					GlobalUnlock(hBuffer);

					IStream* pStream = NULL;
			
					if (SUCCEEDED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream)))
					{
						pResult = Bitmap::FromStream(pStream);
						pStream->Release();
					}
					else
						GlobalFree(hBuffer);
				}
				else
					GlobalFree(hBuffer);
			}
		}
	}
	return pResult;
}

#endif

bool GetVersionInfo(PCWSTR pszFileName, LPWSTR strVersion, ULONG nBufSizeInChars)
{
	HINSTANCE hVLib = LoadLibraryA("version.dll");

	if (hVLib == NULL)
		return false;

	_GetFileVersionInfoSizeW	pGetFileVersionInfoSizeW	= (_GetFileVersionInfoSizeW)GetProcAddress(hVLib, "GetFileVersionInfoSizeW");	
   	_GetFileVersionInfoW		pGetFileVersionInfoW		= (_GetFileVersionInfoW)	GetProcAddress(hVLib, "GetFileVersionInfoW");	
 	_VerQueryValueW				pVerQueryValueW				= (_VerQueryValueW)			GetProcAddress(hVLib, "VerQueryValueW");	

	DWORD			cbVerInfo;
	DWORD			dummy;

    cbVerInfo = pGetFileVersionInfoSizeW(pszFileName, &dummy);

    if (!cbVerInfo)
        return false;
    
    PBYTE pVerInfo = new BYTE[cbVerInfo];

    if (pVerInfo == NULL)
        return false;

	bool bResult = false;

    if (pGetFileVersionInfoW(pszFileName, 0, cbVerInfo, pVerInfo))
	{
		VS_FIXEDFILEINFO*	pFixedVer = NULL;
		WCHAR				szQueryStr[0x100];
		UINT				cbReturn	= 0;

		swprintf_s(szQueryStr, _countof(szQueryStr), L"\\");

		BOOL bFound	= pVerQueryValueW(pVerInfo, szQueryStr,
								(LPVOID *)&pFixedVer, &cbReturn);
		if (bFound)
		{
			bResult = true;

			swprintf_s(strVersion, nBufSizeInChars, L"%ld.%ld.%ld.%ld", (long)HIWORD(pFixedVer->dwFileVersionMS), (long)LOWORD(pFixedVer->dwFileVersionMS)
												 , (long)HIWORD(pFixedVer->dwFileVersionLS), (long)LOWORD(pFixedVer->dwFileVersionLS));
		}
		else
		{
			struct LANGANDCODEPAGE
			{
			  WORD wLanguage;
			  WORD wCodePage;
			} *lpTranslate = NULL;
			
			UINT	cbTranslate = 0;

			pVerQueryValueW(pVerInfo, 
				  L"\\VarFileInfo\\Translation",
				  (LPVOID*)&lpTranslate,
				  &cbTranslate);

			PWSTR pszVerRetVal	= NULL;

			for (long i = 0; i < (long)(cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++)
			{
				swprintf_s(szQueryStr, _countof(szQueryStr), L"\\StringFileInfo\\%04X%04X\\FileVersion",
							lpTranslate[i].wLanguage,
							lpTranslate[i].wCodePage);

				bFound = pVerQueryValueW(pVerInfo, szQueryStr,
										(LPVOID *)&pszVerRetVal, &cbReturn);
				if (bFound)
					break;
			}
			if (bFound && pszVerRetVal)
			{
				bResult = true;

				wcscpy_s(strVersion, nBufSizeInChars, pszVerRetVal);

				long lauf = 0;

				while(strVersion[lauf] != 0)
				{
					if (strVersion[lauf] == L',')
						strVersion[lauf] = L'.';
					lauf++;
				}
			}
		}
    }
    delete pVerInfo;

	return bResult;
}
									
#define DELTA_EP_IN_MICRO_SECS		9435484800000000ULL

ULONGLONG GetNTPTimeStamp()
{
	FILETIME			ft		= { 0 };
	ULONGLONG			tmpres	= 0;
 
	GetSystemTimeAsFileTime(&ft);
 
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
 
	// convert to micro secs
	tmpres /= 10ULL;  
	
	// offset from 1601 to 1970
	tmpres -= DELTA_EP_IN_MICRO_SECS; 

	return MAKEULONGLONG(tmpres % 1000000ULL, tmpres / 1000000ULL);  
}

USHORT GetUniquePortNumber()
{
	c_mtxUniquePortCounter.Lock();

	if (c_nUniquePortCounter > 12000)
		c_nUniquePortCounter = 6000;

	USHORT nResult = c_nUniquePortCounter++;

	c_mtxUniquePortCounter.Unlock();

	return nResult;
}

ULONG CreateRand(ULONG nMin /*= 0*/, ULONG nMax /*= MAXDWORD*/)
{
	ULONG nResult = MAKELONG(rand()+rand(), rand()+rand());

	while(nResult < nMin)
		nResult *= (rand()+1);

	while (nResult > nMax)
	{
		if (nResult &  0x80000000)
			nResult &= 0x7fffffff;
		else
			nResult >>= 1;
	}
	ATLASSERT(nResult >= nMin && nResult <= nMax);

	return nResult;
}

ULONGLONG ToDacpID(PCSTR strID)
{
	ULONGLONG nResult = 0;

	ATLASSERT(strID);

	long n = (long)strlen(strID);

	if (n > 0)
	{
		ULONGLONG f = 1;

		for (long i = n-1; i >= 0; --i)
		{
			char c = strID[i];

			if (c >= '0' && c <= '9')
				nResult += (f*(c - '0'));
			else
			{
				c = tolower(c);

				if (c >= 'a' && c <= 'f')
					nResult += (f*(c - 'a' + 10));
				else
					break;
			}
			f *= 16ull;
		}
	}
	return nResult;
}

bool FileExists(PCWSTR strPath, ATL::CString* pStrResult /*= NULL*/)
{
	WIN32_FIND_DATAW fd;

	HANDLE	h = FindFirstFileW(strPath, &fd);

	if (h == INVALID_HANDLE_VALUE)
		return false;
	else
	{
		if (pStrResult)
		{
			*pStrResult = fd.cFileName;
		}
		FindClose(h);
	}
	return true;
}

std::wstring GetModulePath(HMODULE hModule /*= NULL*/, PCWSTR strModuleName /* = NULL*/)
{
	WCHAR FilePath[MAX_PATH];
		
	ATLVERIFY(GetModuleFileNameW(hModule, FilePath, sizeof(FilePath)/sizeof(WCHAR)) > 0);

	if (strModuleName)
	{
		WCHAR* pS = wcsrchr(FilePath, L'\\');

		if (pS == NULL)
		{
			wcscpy_s(FilePath, MAX_PATH, strModuleName);
		}
		else
		{
			wcscpy_s(pS+1, MAX_PATH - (pS-FilePath) - 1, strModuleName);
		}
	}
	return std::wstring(FilePath);
}

std::wstring ErrorToString(ULONG err /*= ::GetLastError()*/, PCTSTR lpszFunction /*= NULL*/)
{
    std::wstring r;

    // Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf		= NULL;
    LPVOID lpDisplayBuf	= NULL;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // get the error message
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + (lpszFunction ? lstrlen(lpszFunction) : 0) + 40) * sizeof(TCHAR)); 
    
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        lpszFunction ? TEXT("%s: %s") : TEXT("%s%s"), 
        lpszFunction ? lpszFunction : _T(""), lpMsgBuf);

    r = CT2CW((LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);

    return r;
}

void ErrorMsg(HWND hwnd, PCTSTR lpszTitle, PCTSTR lpszFunction, DWORD dwErr) 
{ 
    std::wstring s = ErrorToString(dwErr, lpszFunction);

	::MessageBox(hwnd, s.c_str(), lpszTitle, MB_OK | (dwErr == ERROR_SUCCESS ? MB_ICONINFORMATION : MB_ICONEXCLAMATION));
}

int ErrorMsg(HWND hwnd, HRESULT hrErr)
{
	_com_error err(hrErr);
	LPCTSTR errMsg = err.ErrorMessage();

	if (errMsg)
	{
		int nStyle = MB_ICONERROR | MB_OK;

		if ((SUCCEEDED(hrErr) && (nStyle & MB_ICONERROR)))
		{
			nStyle &= ~MB_ICONERROR;
			nStyle |= MB_ICONINFORMATION;
		}
		std::wstring strMsg = errMsg;

		return ::MessageBoxW(hwnd, strMsg.c_str(), APP_NAME_W, nStyle);
	}
	return MB_OK;
}


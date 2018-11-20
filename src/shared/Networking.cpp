/*
 *
 *  Networking.cpp
 *
 */

#include "stdafx.h"
#include "Networking.h"
#include <IPTypes.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

static inline int _send(SOCKET sd, const void* pBuf, int nLen)
{
	fd_set set;
	fd_set eset;
	   
	FD_ZERO(&set);
	FD_ZERO(&eset);
	FD_SET(sd, &set);
	FD_SET(sd, &eset);

	int nResult = select(0, NULL, &set, &eset, NULL);

	if (nResult != SOCKET_ERROR)
	{
		if (nResult == 0)
		   return 0;

		if (FD_ISSET(sd, &set))
			nResult = ::send(sd, (const char*)pBuf, nLen, 0);
		else
		{
			if (WSAGetLastError() == WSAENOTSOCK)
			{
				nResult = SOCKET_ERROR;
			}
		}
	}
	return nResult;
}

static inline int _sendto(SOCKET sd, const void* pBuf, int nLen, const struct sockaddr* to, int tolen)
{
	fd_set set;
	fd_set eset;
	   
	FD_ZERO(&set);
	FD_ZERO(&eset);
	FD_SET(sd, &set);
	FD_SET(sd, &eset);

	int nResult = select(0, NULL, &set, &eset, NULL);

	if (nResult != SOCKET_ERROR)
	{
		if (nResult == 0)
		   return 0;

		if (FD_ISSET(sd, &set))
			nResult = ::sendto(sd, (const char*)pBuf, nLen, 0, to, tolen);
		else
		{
			if (WSAGetLastError() == WSAENOTSOCK)
			{
				nResult = SOCKET_ERROR;
			}
		}
	}
	return nResult;
}

static inline int _recv(SOCKET sd, void* pBuf, int nLen, bool bFrom = false, struct sockaddr* from = NULL, int* fromlen = NULL)
{
	fd_set set;
	fd_set eset;

	FD_ZERO(&set);
	FD_ZERO(&eset);
	FD_SET(sd, &set);
	FD_SET(sd, &eset);

	int nResult = select(0, &set, NULL, &eset, NULL);

	if (nResult != SOCKET_ERROR)
	{
		if (nResult == 0)
		   return 0;

		if (FD_ISSET(sd, &set))
		{
			if (bFrom)
				nResult = ::recvfrom(sd, (char*)pBuf, nLen, 0, from, fromlen);
			else
				nResult = ::recv(sd, (char*)pBuf, nLen, 0);
		}
		else
		{
			if (WSAGetLastError() == WSAENOTSOCK)
			{
				nResult = SOCKET_ERROR;
			}
		}
	}
	return nResult;
}

////////////////////////////////////////////////////////////////////////////////
// CSocketBase

CSocketBase::CSocketBase()
{
	m_sd		= INVALID_SOCKET;
	m_bBlock	= TRUE;
	m_nType		= -1;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void CSocketBase::Attach(SOCKET sd)
{
	ATLASSERT(m_sd == INVALID_SOCKET);
	m_sd = sd;
}

SOCKET CSocketBase::Detach()
{
    SOCKET sd = INVALID_SOCKET; 

    m_mtxSD.Lock();

	if (m_sd != INVALID_SOCKET)
    {
        sd = m_sd;
	    m_sd = INVALID_SOCKET;
    }
    m_mtxSD.Unlock();

	return sd;
}

CSocketBase::~CSocketBase()
{
	if (m_sd != INVALID_SOCKET)
	   Close();
}

BOOL CSocketBase::Create(int af, int type /* = SOCK_STREAM */)
{
	if (m_sd != INVALID_SOCKET)
		Close();

	m_nType		= type;
	m_sd		= socket(af, type, type == SOCK_STREAM ? 0 : IPPROTO_UDP);
	m_bBlock	= TRUE;

	return m_sd != INVALID_SOCKET ? TRUE : FALSE;
}

BOOL CSocketBase::Connect(const SOCKADDR_IN* pSockAddr, int nSizeofSockAddr)
{
	if (!IsValid())
	{
		if (!Create(pSockAddr->sin_family))
			return FALSE;
	}
	SetBlockMode(TRUE);
	return connect(m_sd, (struct sockaddr *)pSockAddr, nSizeofSockAddr) == SOCKET_ERROR ? FALSE : TRUE;
}

BOOL CSocketBase::Connect(const char* strHost, USHORT nPort_NetOrder, int* pPreferFamily /*= NULL*/, SOCKADDR_IN* pSockAddr /* = NULL */, PULONG pSockAddrLen /* = NULL*/, int type /* = SOCK_STREAM */)
{
	BOOL bResult = FALSE;

	std::list<int> listAF;

	if (pPreferFamily)
	{
		listAF.push_back(*pPreferFamily);
	}
	else
	{
		listAF.push_back(AF_INET);
		listAF.push_back(AF_INET6);
	}
	for (auto i = listAF.begin(); i != listAF.end(); ++i)
	{
		struct addrinfo		aiHints;
		struct addrinfo*	aiList = NULL;

		memset(&aiHints, 0, sizeof(aiHints));

		aiHints.ai_family	= *i;
		aiHints.ai_socktype	= type;
		aiHints.ai_protocol	= type == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP;

		char buf[256];

		sprintf_s(buf, 256, "%lu", (ULONG)ntohs(nPort_NetOrder));

		if ((getaddrinfo(strHost, buf, &aiHints, &aiList)) == 0) 
		{
			struct addrinfo* ai = aiList;

			while (ai)
			{
				if (pPreferFamily == NULL || *pPreferFamily == ai->ai_family)
				{
					if (Create(ai->ai_family, ai->ai_socktype))
					{
						if (ai->ai_socktype == SOCK_STREAM)
						{
							if (Connect((SOCKADDR_IN*)ai->ai_addr, (int) ai->ai_addrlen))
							{
								if (pSockAddr)
									memcpy(pSockAddr, ai->ai_addr, ai->ai_addrlen);
								if (pSockAddrLen)
									*pSockAddrLen = (ULONG)ai->ai_addrlen;
								bResult = TRUE;
								break;
							}
						}
						else
						{
							if (::bind(m_sd, ai->ai_addr, (int) ai->ai_addrlen) != SOCKET_ERROR)
							{
								if (pSockAddr)
									memcpy(pSockAddr, ai->ai_addr, ai->ai_addrlen);
								if (pSockAddrLen)
									*pSockAddrLen = (ULONG)ai->ai_addrlen;
								bResult = TRUE;
								break;
							}
						}
						Close();
					}
				}
				ai = ai->ai_next;
			}
			freeaddrinfo(aiList);
		}
		if (bResult)
			break;
	}
	return bResult;
}

BOOL CSocketBase::WaitForSocketState(long nState, DWORD dwTimeout /*= INFINITE*/, HANDLE hStopEvent /*= NULL*/)
{
    CHandle hEvent(::WSACreateEvent());

    if (!hEvent)
    {
        ATLASSERT(FALSE);
        return FALSE;
    }
    if (WSAEventSelect(m_sd, hEvent, nState|FD_CLOSE) == 0)
    {
        bool bClose = false;

        HANDLE h[2];

        h[0] = hEvent;
        h[1] = hStopEvent;

        if (::WaitForMultipleObjects(hStopEvent ? 2 : 1, h, FALSE, dwTimeout) == WAIT_OBJECT_0)
        {
            WSANETWORKEVENTS NetworkEvents = { 0 };

            if (WSAEnumNetworkEvents(m_sd, hEvent, &NetworkEvents) == 0)
            {
                if (NetworkEvents.lNetworkEvents & nState)
                {
                    ::ResetEvent(hEvent);
                }
                if (NetworkEvents.lNetworkEvents & FD_CLOSE)
                {
                    ::ResetEvent(hEvent);
                    bClose = true;
                }
            }
            else
                bClose = true;
        }
        else
            bClose = true;

        if (bClose)
        {
            return FALSE;
        }
        return TRUE;	
    }
    return FALSE;
}

BOOL CSocketBase::WaitForIncomingData(DWORD dwTimeout, HANDLE hStopEvent /* = NULL*/, PDWORD pByteCountPending /* = NULL*/)
{
	if (pByteCountPending)
		*pByteCountPending = 0;

	CHandle hEvent(::WSACreateEvent());

	if (!hEvent)
	{
		ATLASSERT(FALSE);
		return FALSE;
	}
	WSAEventSelect(m_sd, NULL, 0);

	SetBlockMode(FALSE);

	if (WSAEventSelect(m_sd, hEvent, FD_READ|FD_CLOSE) == 0)
	{
		bool bClose = false;

		HANDLE h[2];

		h[0] = hEvent;
		h[1] = hStopEvent;

		if (::WaitForMultipleObjects(hStopEvent ? 2 : 1, h, FALSE, dwTimeout) == WAIT_OBJECT_0)
		{
            WSANETWORKEVENTS	NetworkEvents = { 0 };

			if (WSAEnumNetworkEvents(m_sd, hEvent, &NetworkEvents) == 0)
			{
  				if (NetworkEvents.lNetworkEvents & FD_READ)
				{
					::ResetEvent(hEvent);

					DWORD dw = 0;

					ioctlsocket(m_sd, FIONREAD, &dw);

					if (dw > 0)
					{
						if (pByteCountPending)
							*pByteCountPending = dw;
					}
					else
					{
						bClose = true;
					}
				}
				else if (NetworkEvents.lNetworkEvents & FD_CLOSE)
				{
                    ::ResetEvent(hEvent);
					bClose = true;
				}
			}
			else
				bClose = true;
		}
		else
			bClose = true;

		WSAEventSelect(m_sd, NULL, 0);

		if (bClose)
		{
			return FALSE;
		}
		return TRUE;	
	}
	return FALSE;
}

void CSocketBase::Close(bool bGracefully /*= true*/)
{
    m_mtxSD.Lock();

    if (m_sd != INVALID_SOCKET)
	{
		if (IsLinger())
			SetLinger(FALSE);
		
		auto sd = m_sd;
		m_sd = INVALID_SOCKET;

        m_mtxSD.Unlock();
		
        int cr = closesocket(sd);
#ifdef _DEBUG

        if (cr != 0)
        {
            ULONG err = ::WSAGetLastError();

            ATLTRACE(L"SocketClose Error: %lu\n", err);
            ATLASSERT(cr == 0 || !bGracefully);
        }
#endif
	}
    else
    {
        m_mtxSD.Unlock();
    }
}
			 
void CSocketBase::SetLinger(BOOL b, DWORD dwLinger)
{
	LINGER l;

	l.l_onoff = (u_short) b;
	l.l_linger = (unsigned short)(dwLinger / 1000);

	setsockopt(m_sd, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
}

BOOL CSocketBase::IsLinger()
{
	LINGER	l;
	int		size = sizeof(l);

	getsockopt(m_sd, SOL_SOCKET, SO_LINGER, (char*)&l, &size);

	return l.l_onoff != 0;
}

void CSocketBase::SetBlockMode(BOOL b)
{
	DWORD dw = (b) ? FALSE : TRUE;

	ioctlsocket(m_sd, FIONBIO, &dw);
	m_bBlock = b;
}

BOOL CSocketBase::IsBlockMode()
{
	return m_bBlock;
}

BOOL CSocketBase::Send(const void* pBuf, int nLen)
{
	int n	= 0;
	int nW	= 0;

	while(n < nLen)
	{
		if ((nW = _send(m_sd, ((const char*)pBuf)+n, nLen-n)) == 0)
			return FALSE;
		if (nW == SOCKET_ERROR)
		{
			int nLastErr = WSAGetLastError();

			if (nLastErr == WSAEWOULDBLOCK)
			{
                if (WaitForSocketState(FD_WRITE))
				    continue;
			}
			return FALSE;
	   }
	   n += nW;
	}
	return n == nLen;
}

int CSocketBase::send(const void* pBuf, int nLen)
{
	return _send(m_sd, pBuf, nLen);
}

int CSocketBase::recv(void* pBuf, int nLen)
{
	return _recv(m_sd, pBuf, nLen);
}

BOOL CSocketBase::SendTo(const void* pBuf, int nLen, const struct sockaddr* to, int tolen)
{
	int n	= 0;
	int nW	= 0;

	while(n < nLen)
	{
		if ((nW = _sendto(m_sd, ((const char*)pBuf)+n, nLen-n, to, tolen)) == 0)
			return FALSE;
		if (nW == SOCKET_ERROR)
		{
			int nLastErr = WSAGetLastError();

			if (nLastErr == WSAEWOULDBLOCK)
			{
                if (WaitForSocketState(FD_WRITE))
				    continue;
			}
			return FALSE;
	   }
	   n += nW;
	}
	return n == nLen;
}

int CSocketBase::recvfrom(void* pBuf, int nLen, struct sockaddr* from /* = NULL */, int* fromlen /* = NULL*/)
{
	return _recv(m_sd, pBuf, nLen, true, from, fromlen);
}

ULONG CSocketBase::GetMACAddress(CTempBuffer<BYTE>& mac, int nIndex /*= 0*/)
{
	typedef  DWORD(WINAPI *__GetAdaptersInfo)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);

	__GetAdaptersInfo		pGetAdaptersInfo = NULL;

	HMODULE hMod = LoadLibraryA("Iphlpapi.dll");

	if (hMod)
	{
		pGetAdaptersInfo = (__GetAdaptersInfo)GetProcAddress(hMod, "GetAdaptersInfo");

		if (pGetAdaptersInfo)
		{
			PIP_ADAPTER_INFO	pAdapterInfo = NULL;
			ULONG				ulOutBufLen = 0;

			if (pGetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != NO_ERROR)
			{
				pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
			}

			if (pGetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
			{
				PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

				while (pAdapter)
				{
					if (pAdapter->AddressLength == 6)
					{
						if (nIndex-- == 0)
						{
							mac.Reallocate(6);
							memcpy(mac, pAdapter->Address, 6);

							FreeLibrary(hMod);
							return 6;
						}
					}
					pAdapter = pAdapter->Next;
				}
			}
		}
		FreeLibrary(hMod);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// CNetworkServer

CNetworkServer::CNetworkServer(time_t nKeepAlive /*= INFINITE*/)
{
    m_nKeepAlive            = nKeepAlive;
    m_nPort					= 0;
	m_bHandleClientRequests = true;

    if (m_nKeepAlive != INFINITE)
        m_dwTimer = 1000;
}

CNetworkServer::~CNetworkServer()
{
	if (IsRunning())
	   Cancel();
}

BOOL CNetworkServer::Bind(USHORT nPort, DWORD dwAddr)
{
	struct sockaddr_in laddr_in;

	memset(&laddr_in, 0, sizeof(laddr_in));

	laddr_in.sin_family		 = AF_INET;
	laddr_in.sin_addr.s_addr = dwAddr;
	laddr_in.sin_port		 = nPort;

	if (::bind(m_sd, (LPSOCKADDR)&laddr_in, sizeof(laddr_in)) == SOCKET_ERROR)
		return FALSE;

	m_nPort = nPort;
	return TRUE;
}

BOOL CNetworkServer::Bind(const SOCKADDR_IN6& in_addr)
{
	if (::bind(m_sd, (const struct sockaddr*)&in_addr, sizeof(in_addr)) == SOCKET_ERROR)
		return FALSE;
	m_nPort = in_addr.sin6_port;
	return TRUE;
}

BOOL CNetworkServer::Bind(const struct sockaddr* sa, int salen)
{
	if (::bind(m_sd, sa, salen) == SOCKET_ERROR)
		return FALSE;
	m_nPort = salen == sizeof(struct sockaddr_in6) ? ((struct sockaddr_in6*)sa)->sin6_port : ((struct sockaddr_in*)sa)->sin_port;
	return TRUE;
}

BOOL CNetworkServer::Listen(int backlog)
{
	return listen(m_sd, backlog) != SOCKET_ERROR ? TRUE : FALSE;
}

BOOL CNetworkServer::Run(USHORT nPort, DWORD dwAddr)
{
	if (IsRunning())
	   return FALSE;

	if (!Create(AF_INET))
		return FALSE;

	if (!Bind(nPort, dwAddr))
	{
		Close();
		return FALSE;
	}
	if (!Listen())
	{
		Close();
		return FALSE;
	}
	return Start();
}

BOOL CNetworkServer::Run(const SOCKADDR_IN6& in_addr)
{
    if (IsRunning())
	   return FALSE;

	if (!Create(AF_INET6))
		return FALSE;

	if (!Bind(in_addr))
	{
		Close();
		return FALSE;
	}
	if (!Listen())
	{
		Close();
		return FALSE;
	}
	return Start();
}

BOOL CNetworkServer::Run(const struct sockaddr* sa, int salen, int type /* = SOCK_STREAM */)
{
    if (IsRunning())
	   return FALSE;

	if (!Create(sa->sa_family, type))
		return FALSE;

	if (!Bind(sa, salen))
	{
		Close();
		return FALSE;
	}
	if (type == SOCK_STREAM)
	{
		if (!Listen())
		{
			Close();
			return FALSE;
		}
	}
	return Start();
}

void CNetworkServer::OnTimer()
{
    time_t to = ::time(NULL);

    std::list<SOCKET> listSD;

    m_cMutex.Lock();

    for (auto i = m_clients.begin(); i != m_clients.end(); ++i)
    {
        if ((to - i->second->m_ts) * 1000 >= m_nKeepAlive)
            listSD.push_back(i->first);
    }
    m_cMutex.Unlock();

    for (auto i = listSD.begin(); i != listSD.end(); ++i)
        CancelClient(*i);
}

bool CNetworkServer::OnStart()
{
	ATLASSERT(m_hEvent);

	if (m_nType == SOCK_STREAM && IsBlockMode())
	   SetBlockMode(FALSE);

	m_hEvent.Close();
	m_hEvent.Attach(::WSACreateEvent());

	if (m_hEvent == WSA_INVALID_EVENT)
	{
		ATLASSERT(FALSE);
		return false;
	}
	if (WSAEventSelect(m_sd, m_hEvent, m_nType == SOCK_STREAM ? FD_ACCEPT : FD_READ) != 0)
	{
		ATLASSERT(FALSE);
		return false;
	}
	return true;
}

void CNetworkServer::OnEvent()
{
    WSANETWORKEVENTS NetworkEvents = { 0 };

    // prevent the socket sd from being closed
    m_mtxSD.Lock();

	if (IsValid() && WSAEnumNetworkEvents(m_sd, m_hEvent, &NetworkEvents) == 0)
	{
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
		{
			ResetEvent(m_hEvent);

			SOCKET sd = accept(m_sd, NULL, NULL);

			if (sd != INVALID_SOCKET)
			{
				WSAEventSelect(sd, NULL, 0);

				DWORD dw = TRUE;

				if (ioctlsocket(sd, FIONBIO, &dw) == SOCKET_ERROR)
				{
					ATLASSERT(FALSE);
				}
				Accept(sd);
			}
		}
  		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
			ResetEvent(m_hEvent);

			if (m_nType == SOCK_STREAM)
			{
				DWORD dw = 0;

				ioctlsocket(m_sd, FIONREAD, &dw);

				if (dw > 0)
					OnRequest(m_sd);
			}
			else
				OnRequest(m_sd);
		}
	}
    m_mtxSD.Unlock();
}

void CNetworkServer::Accept(SOCKET sd)
{
	m_cMutex.Lock();

	auto i = m_clients.find(sd);
	
	if (i != m_clients.end())
	{
		CCThread* pC = i->second;
		m_clients.erase(i);

		m_cMutex.Unlock();

		OnDisconnect(sd);
		pC->Stop();
		delete pC;
	}
	else
		m_cMutex.Unlock();

	if (OnAccept(sd))
	{
		CCThread* pClient = new CCThread(m_bHandleClientRequests);

		pClient->m_pServer	= this;
		pClient->Attach(sd);

		m_cMutex.Lock();
		m_clients[sd] = pClient;
		m_cMutex.Unlock();

		pClient->Start();
	}
	else
	{
		CSocketBase sock;

		sock.Attach(sd);
		sock.Close();
	}
}

BOOL CNetworkServer::Cancel()
{
	if (!IsRunning())
	   return FALSE;

 	m_cMutex.Lock();

	auto i = m_clients.begin();
	
	while (i != m_clients.end())
	{
		CCThread* pC = i->second;

		m_clients.erase(i);
	 	m_cMutex.Unlock();

		pC->Stop();
		OnDisconnect(pC->m_sd);

		delete pC;

	 	m_cMutex.Lock();
        i = m_clients.begin();
	}
 	m_cMutex.Unlock(); 

	Stop();
	Close();
	return TRUE;
}

void CNetworkServer::CancelClient(SOCKET sd)
{
 	m_cMutex.Lock();

	auto i = m_clients.find(sd);
	
	if (i != m_clients.end())
	{
		CCThread* pC = i->second;

		if (pC->IsOwnThread())
		{
			pC->Stop();
			pC = NULL;
		}
		else
        {
			m_clients.erase(i);
        }
	 	m_cMutex.Unlock();

		if (pC)
		{
			pC->Stop();
			OnDisconnect(pC->m_sd);

			delete pC;
		}
	}
	else
    {
 		m_cMutex.Unlock();
    }
}

bool CNetworkServer::CCThread::OnStart()
{
	ATLASSERT(m_pServer);
	ATLASSERT(m_hEvent);

	m_nThreadID = ::GetCurrentThreadId();

	m_hEvent.Close();
	m_hEvent.Attach(::WSACreateEvent());

	if (m_hEvent == WSA_INVALID_EVENT)
	{
		ATLASSERT(FALSE);
		return false;
	}
	if (WSAEventSelect(m_sd, m_hEvent, (m_bListenForIncomingData ? FD_READ : 0)|FD_CLOSE) != 0)
	{
		ATLASSERT(FALSE);
		return false;
	}
	return true;
}

void CNetworkServer::CCThread::OnEvent()
{
    WSANETWORKEVENTS	NetworkEvents = { 0 };
	bool				bClose = false;

    // prevent the socket sd from being closed
    m_mtxSD.Lock();
    
    if (IsValid() && WSAEnumNetworkEvents(m_sd, m_hEvent, &NetworkEvents) == 0)
	{
  		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
            m_ts = ::time(NULL);

			ResetEvent(m_hEvent);

			DWORD dw = 0;

			ioctlsocket(m_sd, FIONREAD, &dw);

			if (dw > 0)
            {
				m_pServer->OnRequest(m_sd);
            }
			else
			{
				bClose = true;
			}
		}
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			ResetEvent(m_hEvent);
			bClose = true;
		}
	}
	else
    {
		bClose = true;
    }
    m_mtxSD.Unlock();
    
    if (bClose || IsStopping())
	{
		bool bFound = false;

		m_pServer->m_cMutex.Lock();

		for (auto i = m_pServer->m_clients.begin(); i != m_pServer->m_clients.end(); ++i)
		{
			if (i->second == this)
			{
				m_pServer->m_clients.erase(i);
				bFound = true;
				break;
			}
		}
		m_pServer->m_cMutex.Unlock();

		if (bFound)
		{
            if (IsValid())
            {
			    m_pServer->OnDisconnect(m_sd);
			    Close(false);
            }
			SetEvent(m_hStopEvent);
			m_bSelfDelete = true;
		}
	}
}



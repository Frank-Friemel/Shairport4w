/*
 *
 *  Networking.h
 *
 */
#pragma once

#include <in6addr.h>
#include <ws2ipdef.h>

#include "myMutex.h"
#include "myThread.h"


////////////////////////////////////////////////////////////////////////////////
// CSocketBase

class CSocketBase
{
public:
	CSocketBase();
	virtual ~CSocketBase();

	BOOL Create(int af = AF_INET, int type = SOCK_STREAM);
	void Attach(SOCKET sd);
	SOCKET Detach();
	void Close(bool bGracefully = true);

	inline bool IsValid()
	{
		return m_sd != INVALID_SOCKET ? true : false;
	}
	int  recv(void* pBuf, int nLen);	
	BOOL Send(const void* pBuf, int nLen);	
	int  recvfrom(void* pBuf, int nLen, struct sockaddr* from = NULL, int* fromlen = NULL);	
	BOOL SendTo(const void* pBuf, int nLen, const struct sockaddr* to, int tolen);	

	void SetBlockMode(BOOL b);
	BOOL IsBlockMode();

	BOOL Connect(const SOCKADDR_IN* pSockAddr, int nSizeofSockAddr);
	BOOL Connect(const char* strHost, USHORT nPort_NetOrder, int* pPreferFamily = NULL, SOCKADDR_IN* pSockAddr = NULL, PULONG pSockAddrLen = NULL, int type = SOCK_STREAM);
	BOOL WaitForIncomingData(DWORD dwTimeout, HANDLE hStopEvent = NULL, PDWORD pByteCountPending = NULL);

	void SetLinger(BOOL b, DWORD dwLinger = 0);
	BOOL IsLinger();

	int  send(const void* pBuf, int nLen);

	static ULONG GetMACAddress(CTempBuffer<BYTE>& mac, int nIndex = 0);

protected:
    BOOL WaitForSocketState(long nState, DWORD dwTimeout = INFINITE, HANDLE hStopEvent = NULL);

public:
	SOCKET	    m_sd;
	BOOL	    m_bBlock;
	int		    m_nType;
protected:
    CMyMutex    m_mtxSD;
};

////////////////////////////////////////////////////////////////////////////////
// CNetworkServer

class CNetworkServer : public CSocketBase, protected CMyThread
{
public:
	CNetworkServer(time_t nKeepAlive = INFINITE);
	~CNetworkServer();

	BOOL Run(USHORT nPort, DWORD dwAddr = INADDR_ANY);
	BOOL Run(const SOCKADDR_IN6& in_addr);
	BOOL Run(const struct sockaddr* sa, int salen, int type = SOCK_STREAM);

	BOOL Cancel();
	void CancelClient(SOCKET sd);

	inline ULONG GetClientCount() const
	{
		ULONG nResult = 0;

		m_cMutex.Lock();
		nResult = (ULONG) m_clients.size();
		m_cMutex.Unlock();

		return nResult;
	}
public:
	USHORT	m_nPort; // network order

protected:
	BOOL Bind(USHORT nPort, DWORD dwAddr = INADDR_ANY);
	BOOL Bind(const SOCKADDR_IN6& in_addr);
	BOOL Bind(const struct sockaddr* sa, int salen);

	BOOL Listen(int backlog = SOMAXCONN);
	void Accept(SOCKET sd);

	virtual bool OnAccept(SOCKET sd)
	{
        UNREFERENCED_PARAMETER(sd);
		return true;
	}
	virtual void OnDisconnect(SOCKET sd)
	{
        UNREFERENCED_PARAMETER(sd);
    }
	virtual void OnRequest(SOCKET sd) = 0;

	virtual void OnEvent();
	virtual bool OnStart();
    virtual void OnTimer();

	class CCThread : public CMyThread, public CSocketBase
	{
		public:
			CCThread(bool bListenForIncomingData = true)
			{
				m_bListenForIncomingData = bListenForIncomingData;
                m_ts = ::time(NULL);
			}
		protected:
			virtual void OnEvent();
			virtual bool OnStart();
		public:
			CNetworkServer*	m_pServer;
			DWORD			m_nThreadID;
			bool			m_bListenForIncomingData;
            time_t          m_ts;
	};
	friend class CCThread;

protected:
	std::map<SOCKET, CCThread*>	m_clients;
	mutable CMyMutex		    m_cMutex;
	bool					    m_bHandleClientRequests;
    time_t                      m_nKeepAlive;
};


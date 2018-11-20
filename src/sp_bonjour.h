/*
 *
 *  sp_bonjour.h
 *
 */

#pragma once

#include "myThread.h"
#include "../Bonjour/dns_sd.h"

extern bool InitBonjour();
extern void DeInitBonjour();

class CRegisterServiceEvent;

class CServiceResolveCallback
{
public:
	virtual bool OnServiceResolved(CRegisterServiceEvent* pServiceEvent, const char* strHost, USHORT nPort) = 0;
};

class CDnsSD_Thread : protected CMyThread
{
public:
	CDnsSD_Thread();
	~CDnsSD_Thread();

public:
	void ProcessResult(DNSServiceRef sdref, DWORD dwTimeout);
	void Stop();

protected:
	virtual bool OnStart();
	virtual void OnStop();
	virtual void OnEvent();
	virtual void OnTimer();

public:
	bool			m_bTimedout;
protected:
	DNSServiceRef	m_sdref;
	CSocketBase		m_socket;
};

class CDnsSD_Register : public CDnsSD_Thread
{
public:
	CDnsSD_Register()
	{
		m_nErrorCode	= 0;
		m_hErrorEvent	= ::CreateEventA(NULL, FALSE, FALSE, NULL);
		ATLASSERT(m_hErrorEvent);
	}
	~CDnsSD_Register()
	{
		if (m_hErrorEvent)
			CloseHandle(m_hErrorEvent);
	}
public:
	bool Start(BYTE* addr, PCSTR strApName, BOOL bNoMetaInfo, BOOL bPassword, USHORT nPort);
public:
	HANDLE					m_hErrorEvent;
	DNSServiceErrorType		m_nErrorCode;
};

class IDNSServiceBrowseReply
{
public:
	virtual void OnServiceRegistered(CRegisterServiceEvent* pEvent) = 0;
};

class CDnsSD_BrowseForService : public CDnsSD_Thread
{
public:
	virtual void OnDNSServiceBrowseReply(
									DNSServiceRef                       sdRef,
									DNSServiceFlags                     flags,
									uint32_t                            interfaceIndex,
									DNSServiceErrorType                 errorCode,
									const char*                         serviceName,
									const char*                         regtype,
									const char*                         replyDomain
									);
	bool Start(const char* strRegType, HWND hWnd, UINT nMsg);
	bool Start(const char* strRegType, IDNSServiceBrowseReply* pCallback);

protected:
	UINT					m_nMsg;
	HWND					m_hWnd;
	IDNSServiceBrowseReply*	m_pCallback;
};

class CDnsSD_ServiceQueryRecord : public CDnsSD_Thread
{
public:
	bool Start(CRegisterServiceEvent* pRSEvent, uint32_t interfaceIndex, const char* fullname, uint16_t	rrtype, uint16_t rrclass);
};

class CRegisterServiceEvent : public CMyMutex
{
public:
	CRegisterServiceEvent()
	{
		m_bRegister			= false;
		m_pCallback			= NULL;
		m_nInterfaceIndex	= 0;
		m_nPort				= 0;
		m_bCallbackResult	= false;
	}
	CRegisterServiceEvent(const CRegisterServiceEvent& i)
	{
		m_pCallback			= NULL;
		operator=(i);
	}
	bool Resolve(CServiceResolveCallback* pCallback, DWORD dwTimeout = 2500);
	bool QueryHostAddress();

	CRegisterServiceEvent& operator=(const CRegisterServiceEvent& i)
	{
		m_bRegister				= i.m_bRegister;
		m_strService			= i.m_strService;
		m_strRegType			= i.m_strRegType;
		m_strReplyDomain		= i.m_strReplyDomain;
		m_nInterfaceIndex		= i.m_nInterfaceIndex;
		m_strHost				= i.m_strHost;
		m_nPort					= i.m_nPort;
		m_strFullname			= i.m_strFullname;

		return *this;
	}
	inline bool IsValid()
	{
		return m_bRegister && !m_strService.empty();
	}
	bool operator==(const CRegisterServiceEvent& i)
	{
		return m_strService == i.m_strService ? true : false;
	}
public:
	bool						m_bRegister;
	ic_string					m_strService;
	ic_string					m_strRegType;
	ic_string					m_strReplyDomain;
	ULONG						m_nInterfaceIndex;
	ic_string					m_strHost;
	USHORT						m_nPort; // net order
	ic_string					m_strTXT;
	ic_string					m_strFullname;
public:
	// transient members
	CServiceResolveCallback*	m_pCallback;	
	bool						m_bCallbackResult;
	CDnsSD_ServiceQueryRecord	m_threadQueryRecord;
	struct sockaddr_in			m_sa4;
};

/*
 *
 *  sp_bonjour.cpp
 *
 */

#include "stdafx.h"

static HMODULE			hDnsSD			= NULL;
static CMyMutex			mtxDnsSD;

bool InitBonjour()
{
	mtxDnsSD.Lock();

	if (hDnsSD == NULL)
		hDnsSD = LoadLibraryA("dnssd.dll");

	mtxDnsSD.Unlock();

	return hDnsSD ? true : false;
}

void DeInitBonjour()
{
	mtxDnsSD.Lock();

	if (hDnsSD)
	{
		FreeLibrary(hDnsSD);
		hDnsSD	= NULL;
	}
	mtxDnsSD.Unlock();
}

static char* dns_parse_domain_name(char *p, char **x)
{
	uint8_t *v8;
	uint16_t *v16, skip;
	uint16_t i, j, dlen, len;
	int more, compressed;
	char *name, *start;

	start = *x;
	compressed = 0;
	more = 1;
	name = (char*)malloc(1);
	name[0] = '\0';
	len = 1;
	j = 0;
	skip = 0;

	while (more == 1)
	{
		v8 = (uint8_t *)*x;
		dlen = *v8;

		if ((dlen & 0xc0) == 0xc0)
		{
			v16 = (uint16_t *)*x;
			*x = p + (ntohs(*v16) & 0x3fff);
			if (compressed == 0) skip += 2;
			compressed = 1;
			continue;
		}

		*x += 1;
		if (dlen > 0)
		{
			len += dlen;
			name = (char*)realloc(name, len);
		}
	
		for (i = 0; i < dlen; i++)
		{
			name[j++] = **x;
			*x += 1;
		}
		name[j] = '\0';
		if (compressed == 0) skip += (dlen + 1);
		
		if (dlen == 0) more = 0;
		else
		{
			v8 = (uint8_t *)*x;
			if (*v8 != 0)
			{
				len += 1;
				name = (char*)realloc(name, len);
				name[j++] = '.';
				name[j] = '\0';
			}
		}
	}
	
	*x = start + skip;

	return name;
}

static void DNSSD_API MyRegisterServiceReply
    (
	DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    const char*                         name,
    const char*                         regtype,
    const char*                         domain,
    void*                               context
    )
{
	_LOG("MyRegisterServiceReply: Name: %s, RegType: %s, Domain: %s, ErrorCode: 0x%lx\n"
		, name		!= NULL ? name		: "<null>"
		, regtype	!= NULL ? regtype	: "<null>"
		, domain	!= NULL ? domain	: "<null>"
		, (ULONG)errorCode);

	if (context)
	{
		CDnsSD_Register* pContext = (CDnsSD_Register*) context;
		pContext->m_nErrorCode = errorCode;
		ATLVERIFY(::SetEvent(pContext->m_hErrorEvent));
	}
}

static DNSServiceErrorType StaticDNSServiceRegister
    (
    DNSServiceRef*                      sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char*                         name,         /* may be NULL */
    const char*                         regtype,
    const char*                         domain,       /* may be NULL */
    const char*                         host,         /* may be NULL */
    uint16_t                            port,
    uint16_t                            txtLen,
    const void*                         txtRecord,    /* may be NULL */
    DNSServiceRegisterReply             callBack,     /* may be NULL */
    void*                               context       /* may be NULL */
    )
{
	typedef DNSServiceErrorType (DNSSD_API *_typeDNSServiceRegister)
																(
																DNSServiceRef*                      sdRef,
																DNSServiceFlags                     flags,
																uint32_t                            interfaceIndex,
																const char*                         name,         /* may be NULL */
																const char*                         regtype,
																const char*                         domain,       /* may be NULL */
																const char*                         host,         /* may be NULL */
																uint16_t                            port,
																uint16_t                            txtLen,
																const void*                         txtRecord,    /* may be NULL */
																DNSServiceRegisterReply             callBack,      /* may be NULL */
																void*                               context       /* may be NULL */
																);

	_typeDNSServiceRegister pDNSServiceRegister = (_typeDNSServiceRegister) GetProcAddress(hDnsSD, "DNSServiceRegister");

	if (pDNSServiceRegister)
	{
		return pDNSServiceRegister( sdRef,
								    flags,
								    interfaceIndex,
								    name,       
								    regtype,
								    domain,     
								    host,       
								    port,
								    txtLen,
								    txtRecord,  
								    callBack,    
								    context);
	}
	return kDNSServiceErr_Unsupported;
}

static void StaticDNSServiceRefDeallocate(DNSServiceRef& sdRef)
{
	typedef void (DNSSD_API *_typeDNSServiceRefDeallocate)(DNSServiceRef sdRef);

	_typeDNSServiceRefDeallocate pDNSServiceRefDeallocate = (_typeDNSServiceRefDeallocate) GetProcAddress(hDnsSD, "DNSServiceRefDeallocate");

	if (pDNSServiceRefDeallocate)
	{
		pDNSServiceRefDeallocate(sdRef);
		sdRef = NULL;
	}
	else
	{
		ATLASSERT(FALSE);
	}
}

static DNSServiceErrorType RegisterService(DNSServiceRef* sdRef, const char* nam, const char* typ, const char* dom, uint16_t port, char** argv, DNSServiceRegisterReply callBack = NULL, CDnsSD_Register* pContext = NULL)
{
	char		txt[2048];
	uint16_t	n	= 0;

	memset(txt, 0, sizeof(txt));
	
	if (nam[0] == '.' && nam[1] == 0) 
		nam = "";
	if (dom)
	{
		if (dom[0] == '.' && dom[1] == 0) 
			dom = "";
	}
	for(;;)
	{
		const char*	p = *argv++;

		if (p == NULL)
			break;

		if (strlen(p) > 0)
		{
			txt[n++] = (char)strlen(p);

			strcpy_s(txt + n, sizeof(txt)-n, p);
			n += (uint16_t)strlen(p);
		}
	}
	return StaticDNSServiceRegister(sdRef, 0, kDNSServiceInterfaceIndexAny, nam, typ, dom, NULL, port, n, txt, callBack, pContext);
}

static int StaticDNSServiceRefSockFD(DNSServiceRef sdRef)
{
	typedef int (DNSSD_API *_typeDNSServiceRefSockFD)(DNSServiceRef sdRef);

	_typeDNSServiceRefSockFD pDNSServiceRefSockFD = (_typeDNSServiceRefSockFD) GetProcAddress(hDnsSD, "DNSServiceRefSockFD");

	if (pDNSServiceRefSockFD)
		return pDNSServiceRefSockFD(sdRef);
	else
	{
		ATLASSERT(FALSE);
	}
	return (int) INVALID_SOCKET;
}

static DNSServiceErrorType StaticDNSServiceProcessResult(DNSServiceRef sdRef)
{
	typedef DNSServiceErrorType (DNSSD_API *_typeDNSServiceProcessResult)(DNSServiceRef sdRef);

	_typeDNSServiceProcessResult pDNSServiceProcessResult = (_typeDNSServiceProcessResult) GetProcAddress(hDnsSD, "DNSServiceProcessResult");

	if (pDNSServiceProcessResult)
		return pDNSServiceProcessResult(sdRef);
	else
	{
		ATLASSERT(FALSE);
	}
	return kDNSServiceErr_Unsupported;
}

void DNSSD_API MyDNSServiceQueryRecordReply
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char*                         fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    uint16_t                            rdlen,
    const void*                         rdata,
    uint32_t                            ttl,
    void *                              context
    )
{
	CRegisterServiceEvent* pEvent = (CRegisterServiceEvent*) context;

	switch (rrtype)
	{
		case kDNSServiceType_A:
		{
			if (sizeof(pEvent->m_sa4) >= rdlen)
			{
				pEvent->Lock();
				memcpy(&pEvent->m_sa4.sin_addr, rdata, rdlen);
				pEvent->Unlock();
			}
		}
		break;

		case kDNSServiceType_SRV:
		{
			USES_CONVERSION;

			char* x		= ((char*)rdata) + 3 * sizeof(uint16_t);
			char* name	= dns_parse_domain_name(x, &x);
			
			pEvent->Lock();
			pEvent->m_strHost = name;
			pEvent->Unlock();

			free(name);
		}
		break;
	}
}

static DNSServiceErrorType StaticDNSServiceQueryRecord
    (
    DNSServiceRef*                      sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char*                         fullname,
    uint16_t	                        rrtype,
    uint16_t							rrclass,
    DNSServiceQueryRecordReply          callBack,
    void*                               context  /* may be NULL */
    )
{
	typedef DNSServiceErrorType (DNSSD_API *_typeDNSServiceQueryRecord)
												(
												DNSServiceRef                       *sdRef,
												DNSServiceFlags                     flags,
												uint32_t                            interfaceIndex,
												const char                          *fullname,
												uint16_t	                        rrtype,
												uint16_t							rrclass,
												DNSServiceQueryRecordReply          callBack,
												void                                *context  
												);

	_typeDNSServiceQueryRecord pDNSServiceQueryRecord = (_typeDNSServiceQueryRecord) GetProcAddress(hDnsSD, "DNSServiceQueryRecord");

	if (pDNSServiceQueryRecord)
	{
		return pDNSServiceQueryRecord(sdRef,
								 flags,
								 interfaceIndex,
								 fullname,
								 rrtype,
								 rrclass,
								 callBack,
								 context);
	}
	else
	{
		ATLASSERT(FALSE);
	}
	return kDNSServiceErr_Unsupported;
}

static DNSServiceErrorType StaticDNSServiceResolve
    (
    DNSServiceRef*                      sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char*                         name,
    const char*                         regtype,
    const char*                         domain,
    DNSServiceResolveReply              callBack,
    void*                               context  /* may be NULL */
    )
{
	typedef DNSServiceErrorType (DNSSD_API *_typeDNSServiceResolve)
												(
												DNSServiceRef*                      sdRef,
												DNSServiceFlags                     flags,
												uint32_t                            interfaceIndex,
												const char*                         name,
												const char*                         regtype,
												const char*                         domain,
												DNSServiceResolveReply              callBack,
												void*                               context  
												);

	_typeDNSServiceResolve pDNSServiceResolve = (_typeDNSServiceResolve) GetProcAddress(hDnsSD, "DNSServiceResolve");

	if (pDNSServiceResolve)
		return pDNSServiceResolve(sdRef,
								 flags,
								 interfaceIndex,
								 name,
								 regtype,
								 domain,
								 callBack,
								 context);
	else
	{
		ATLASSERT(FALSE);
	}
	return kDNSServiceErr_Unsupported;
}

static void DNSSD_API MyDNSServiceResolveReply
    (
		DNSServiceRef                       sdRef,
		DNSServiceFlags                     flags,
		uint32_t                            interfaceIndex,
		DNSServiceErrorType                 errorCode,
		const char*                         fullname,
		const char*                         hosttarget,
		uint16_t                            port,
		uint16_t                            txtLen,
		const unsigned char*                txtRecord,
		void*                               context
    )
{
	ATLASSERT(context);

	CRegisterServiceEvent* pEvent = (CRegisterServiceEvent*) context;

	ATLASSERT(pEvent->m_pCallback);

	if (txtRecord && txtLen > 0)
		pEvent->m_strTXT		= ic_string((const char*)txtRecord, txtLen);
	else
		pEvent->m_strTXT.clear();

	ic_string strHostTarget = hosttarget;

	if (fullname)
	{
		pEvent->m_strFullname = fullname;
	}
	pEvent->m_bCallbackResult = pEvent->m_pCallback->OnServiceResolved(pEvent, strHostTarget.c_str(), port);
}

bool CRegisterServiceEvent::Resolve(CServiceResolveCallback* pCallback, DWORD dwTimeout /*= 2500*/)
{
	DNSServiceRef sd = NULL;

	m_pCallback = pCallback;

	if (kDNSServiceErr_NoError == StaticDNSServiceResolve(&sd, 0, m_nInterfaceIndex, m_strService.c_str()
															, m_strRegType.c_str(), m_strReplyDomain.c_str()
															, MyDNSServiceResolveReply, this))
	{
		CDnsSD_Thread ProcessResultHelper;
		
		ProcessResultHelper.ProcessResult(sd, dwTimeout);

		if (ProcessResultHelper.m_bTimedout)
		{
			ATLTRACE("DNSServiceResolve \"%s\" timed out!\n", m_strService.c_str());
			ATLASSERT(!m_bCallbackResult);
		}
		StaticDNSServiceRefDeallocate(sd);

		m_pCallback = NULL;
		return m_bCallbackResult;
	}
	m_pCallback = NULL;
	return false;
}

bool CRegisterServiceEvent::QueryHostAddress()
{
	return m_threadQueryRecord.Start(this, m_nInterfaceIndex, m_strFullname.c_str(), kDNSServiceType_SRV, kDNSServiceClass_IN) ? true : false;
}

static DNSServiceErrorType StaticDNSServiceBrowse(DNSServiceRef *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char*                         regtype,
    const char*                         domain,    /* may be NULL */
    DNSServiceBrowseReply               callBack,
    void*                               context    /* may be NULL */)
{
	typedef DNSServiceErrorType (DNSSD_API *_typeDNSServiceBrowse)(DNSServiceRef *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char*                         regtype,
    const char*                         domain,    /* may be NULL */
    DNSServiceBrowseReply               callBack,
    void*                               context    /* may be NULL */);

	_typeDNSServiceBrowse pDNSServiceBrowse = (_typeDNSServiceBrowse) GetProcAddress(hDnsSD, "DNSServiceBrowse");

	if (pDNSServiceBrowse)
		return pDNSServiceBrowse(sdRef, flags, interfaceIndex, regtype, domain, callBack, context);
	else
	{
		ATLASSERT(FALSE);
	}
	return kDNSServiceErr_Unsupported;
}

CDnsSD_Thread::CDnsSD_Thread()
{
	m_sdref			= NULL;
	m_bTimedout		= false;
}

CDnsSD_Thread::~CDnsSD_Thread()
{
	Stop();
}

void CDnsSD_Thread::ProcessResult(DNSServiceRef sdref, DWORD dwTimeout)
{
	ATLASSERT(sdref);
	ATLASSERT(dwTimeout != INFINITE);

	m_bTimedout		= false;
	m_sdref			= sdref;
	m_dwTimer		= dwTimeout;

	ATLVERIFY(__super::Start());
	::WaitForSingleObject(m_hThread, INFINITE);
	m_sdref = NULL;

	Stop();
}

void CDnsSD_Thread::Stop()
{
	__super::Stop();

	if (m_sdref)
	{
		StaticDNSServiceRefDeallocate(m_sdref);
		m_sdref = NULL;
	}
}

void CDnsSD_Thread::OnTimer()
{
	m_bTimedout = true;
	SetEvent(m_hStopEvent);
}

bool CDnsSD_Thread::OnStart()
{
	ATLASSERT(m_sdref);

	int sd = StaticDNSServiceRefSockFD(m_sdref);

	if (sd == INVALID_SOCKET)
	{
		ATLASSERT(FALSE);
		return false;
	}
	WSAEventSelect(sd, NULL, 0);

	DWORD dw = TRUE;

	if (ioctlsocket(sd, FIONBIO, &dw) == SOCKET_ERROR)
	{
		ATLASSERT(FALSE);
		return false;
	}
    m_hEvent.Close();
    m_hEvent.Attach(::WSACreateEvent());

	if (m_hEvent == WSA_INVALID_EVENT)
	{
		::closesocket(sd);
		ATLASSERT(FALSE);
		return false;
	}
	if (WSAEventSelect(sd, m_hEvent, FD_READ|FD_CLOSE) != 0)
	{
		::closesocket(sd);
		ATLASSERT(FALSE);
		return false;
	}
	m_socket.Attach(sd);
	return true;
}

void CDnsSD_Thread::OnStop()
{
	ATLVERIFY(WSACloseEvent(m_hEvent.Detach()));
	m_socket.Detach();
}

void CDnsSD_Thread::OnEvent()
{
	WSANETWORKEVENTS	NetworkEvents;
	bool				bClose = false;

	if (WSAEnumNetworkEvents(m_socket.m_sd, m_hEvent, &NetworkEvents) == 0)
	{
  		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
			ResetEvent(m_hEvent);

			DNSServiceErrorType err = StaticDNSServiceProcessResult(m_sdref);

			if (err != kDNSServiceErr_NoError)
			{
				_LOG("DNSServiceProcessResult failed with code %ld!\n", (long)err);
			}
			if (m_dwTimer != INFINITE)
				SetEvent(m_hStopEvent);
		}
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			ResetEvent(m_hEvent);
			bClose = true;
		}
	}
	if (bClose)
	{
		_LOG("DNS-SD closed the connection!\n");
		SetEvent(m_hStopEvent);
	}
}

bool CDnsSD_Register::Start(BYTE* addr, PCSTR strApName, BOOL bNoMetaInfo, BOOL bPassword, USHORT nPort)
{
	char buf[512];

	sprintf_s(buf, 512, "%02X%02X%02X%02X%02X%02X@%s"	, addr[0]
														, addr[1]
														, addr[2]
														, addr[3]
														, addr[4]
														, addr[5]
														, strApName); 

														char* argv[] = { "tp=UDP", "sm=false", "sv=false", "ek=1", "et=0,1", bNoMetaInfo ? "" : "md=0,1,2", "cn=0,1", "ch=2", "ss=16", "sr=44100", bPassword ? "pw=false" : "pw=true", "vn=3", "txtvers=1", NULL };

	if (kDNSServiceErr_NoError == RegisterService(&m_sdref, buf, "_raop._tcp", ".", nPort, argv, MyRegisterServiceReply, this))
	{
		if (CMyThread::Start())
		{
			if (::WaitForSingleObject(m_hErrorEvent, 5000) == WAIT_TIMEOUT)
			{
				m_nErrorCode = kDNSServiceErr_Timeout;
				
				// unexpected
				ATLASSERT(FALSE);
			}
			return kDNSServiceErr_NoError == m_nErrorCode ? true : false;
		}
	}
	return false;
}

static void DNSSD_API MyDNSServiceBrowseReply
							(
								DNSServiceRef                       sdRef,
								DNSServiceFlags                     flags,
								uint32_t                            interfaceIndex,
								DNSServiceErrorType                 errorCode,
								const char*                         serviceName,
								const char*                         regtype,
								const char*                         replyDomain,
								void*                               context
							)
{
	ATLASSERT(context);
	((CDnsSD_BrowseForService*) context)->OnDNSServiceBrowseReply(sdRef, flags, interfaceIndex, errorCode, serviceName, regtype, replyDomain);
}
	
void CDnsSD_BrowseForService::OnDNSServiceBrowseReply(
								DNSServiceRef                       sdRef,
								DNSServiceFlags                     flags,
								uint32_t                            interfaceIndex,
								DNSServiceErrorType                 errorCode,
								const char*                         serviceName,
								const char*                         regtype,
								const char*                         replyDomain
								)
{
	if (serviceName && (m_pCallback || (m_hWnd && ::IsWindow(m_hWnd))))
	{
		CRegisterServiceEvent* pEvent = new CRegisterServiceEvent;

		pEvent->m_nInterfaceIndex	= interfaceIndex;
		pEvent->m_strService		= serviceName;
		pEvent->m_strRegType		= regtype		? regtype		: "";
		pEvent->m_strReplyDomain	= replyDomain	? replyDomain	: "";
		pEvent->m_bRegister			= (flags & kDNSServiceFlagsAdd) ? true : false;

		if (m_pCallback)
			m_pCallback->OnServiceRegistered(pEvent);
		else
			::PostMessage(m_hWnd, m_nMsg, 0, (LPARAM)pEvent);
	}
}

bool CDnsSD_BrowseForService::Start(const char* strRegType, HWND hWnd, UINT nMsg)
{
	m_nMsg			= nMsg;
	m_hWnd			= hWnd;
	m_pCallback		= NULL;

	if (kDNSServiceErr_NoError == StaticDNSServiceBrowse(&m_sdref, 0, 0, strRegType, NULL, MyDNSServiceBrowseReply, this))
	{
		return CMyThread::Start() ? true : false;
	}
	return false;
}

bool CDnsSD_BrowseForService::Start(const char* strRegType, IDNSServiceBrowseReply* pCallback)
{
	m_nMsg			= 0;
	m_hWnd			= NULL;
	m_pCallback		= pCallback;

	ATLASSERT(m_pCallback);

	if (kDNSServiceErr_NoError == StaticDNSServiceBrowse(&m_sdref, 0, 0, strRegType, NULL, MyDNSServiceBrowseReply, this))
	{
		return CMyThread::Start() ? true : false;
	}
	return false;
}

bool CDnsSD_ServiceQueryRecord::Start(CRegisterServiceEvent* pRSEvent, uint32_t interfaceIndex, const char* fullname, uint16_t rrtype, uint16_t rrclass)
{
	if (!IsRunning())
	{
		if (kDNSServiceErr_NoError == StaticDNSServiceQueryRecord(&m_sdref, 0, interfaceIndex, fullname, rrtype, rrclass, MyDNSServiceQueryRecordReply, pRSEvent))
		{
			return CMyThread::Start() ? true : false;
		}
	}
	return false;
}

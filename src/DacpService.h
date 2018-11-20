/*
 *
 *  DacpService.h 
 *
 */

#pragma once

class CDacpService
{
public:
	CDacpService(void);
	CDacpService(const ic_string& strHost, USHORT nPort, CRegisterServiceEvent* pEvent)
	{
		m_strDacpHost	= strHost;
		m_nDacpPort		= nPort;

		if (pEvent)
			m_Event			= *pEvent;
	}
	CDacpService(const CDacpService& i)
	{
		operator=(i);
	}
	~CDacpService(void);

	CDacpService& operator=(const CDacpService& i)
	{
		m_strDacpHost	= i.m_strDacpHost;
		m_nDacpPort		= i.m_nDacpPort;
		m_Event			= i.m_Event;

		return *this;
	}

public:
	ic_string							m_strDacpHost;
	USHORT								m_nDacpPort;		// net order
	CRegisterServiceEvent				m_Event;
};


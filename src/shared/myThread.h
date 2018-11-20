/*
 *
 *  myThread.h 
 *
 */

#pragma once

#include <process.h>
#include <processthreadsapi.h>
#include <time.h>

/////////////////////////////////////////////////////////////////////////
// CCoInitializeScope
class CCoInitializeScope
{
public:
	CCoInitializeScope(DWORD dwCoInit = COINIT_MULTITHREADED)
	{
		m_hr = ::CoInitializeEx(NULL, dwCoInit);
		ATLASSERT(SUCCEEDED(m_hr));
	}
	~CCoInitializeScope()
	{
		if (SUCCEEDED(m_hr))
			CoUninitialize();
	}
public:
	HRESULT	m_hr;
};

/////////////////////////////////////////////////////////////////////////
// CMyThread

class CMyThread
{
public:
	CMyThread(bool bManualResetEvent = false, PCTSTR strEventName = NULL)
	{
        m_nThreadID     = 0;
		m_bSelfDelete	= false;
		m_dwTimer		= INFINITE;

		m_hStopEvent.Create(NULL, TRUE, FALSE, NULL);
        m_hRescheduleEvent.Create(NULL, FALSE, FALSE, NULL);

		if (strEventName)
		{
			SECURITY_DESCRIPTOR sd;
			InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

			SetSecurityDescriptorDacl(&sd, TRUE, NULL, TRUE);

            SECURITY_ATTRIBUTES sa = { 0 };

			sa.nLength				= sizeof(sa);
			sa.lpSecurityDescriptor = &sd;
			sa.bInheritHandle		= FALSE;

			m_hEvent.Create(&sa, !!bManualResetEvent, FALSE, strEventName);

			if (!m_hEvent)
				m_hEvent.Open(SYNCHRONIZE, FALSE, strEventName);
		}
		else
		{
			m_hEvent.Create(NULL, !!bManualResetEvent, FALSE, NULL);
		}
		ATLASSERT(m_hStopEvent);
        ATLASSERT(m_hRescheduleEvent);
        ATLASSERT(m_hEvent);
	}
	virtual ~CMyThread()
	{
	    Stop();
	}
public:
	bool Start()
	{
		Stop();

		m_hStopEvent.Reset();
        m_nThreadID = 0;

        m_hThread.Attach((HANDLE) _beginthreadex(NULL, 0, _Run, this, m_bSelfDelete ? CREATE_SUSPENDED : 0, NULL));

		if (m_hThread)
		{
            InterlockedCompareExchange((volatile LONG*) &m_nThreadID, (LONG)::GetThreadId(m_hThread), 0);
            ATLASSERT(m_nThreadID == ::GetThreadId(m_hThread));
		
            if (m_bSelfDelete)
				::ResumeThread(m_hThread);
			return true;
		}
		return false;
	}
	bool Start(HANDLE hEvent)
	{
		ATLASSERT(hEvent);

		if (hEvent)
		{
            Stop();
            
            m_hEvent.Close();
            
            if (::DuplicateHandle(::GetCurrentProcess(), hEvent, ::GetCurrentProcess(), &m_hEvent.m_h, 0, FALSE, DUPLICATE_SAME_ACCESS))
			    return Start();
		}
		return false;
	}
	void Stop()
	{
		if (m_nThreadID)
		{
            bool volatile bSelfDelete = m_bSelfDelete;

			m_hStopEvent.Set();

            if (!bSelfDelete && ::GetCurrentThreadId() != m_nThreadID)
            {
                IsRunning(INFINITE);
                m_hThread.Close();
                m_nThreadID = 0;
            }
		}
	}
	inline bool IsRunning(DWORD dwWait = 0) const
	{
        ATLASSERT(!IsOwnThread());
		return m_hThread && ::WaitForSingleObject(m_hThread, dwWait) != WAIT_OBJECT_0;
	}
	inline bool IsStopping(DWORD dwWait = 0) const
	{
		return ::WaitForSingleObject(m_hStopEvent, dwWait) == WAIT_OBJECT_0;
	}
    inline bool IsOwnThread() const
    {
        return m_nThreadID && ::GetCurrentThreadId() == m_nThreadID ? true : false;
    }
    inline void SetTimer(DWORD dwTimer)
    {
        if (m_dwTimer != dwTimer)
        {
            m_dwTimer = dwTimer;

            if (m_nThreadID && ::GetCurrentThreadId() != m_nThreadID)
                m_hRescheduleEvent.Set();
        }
    }
    inline void KillTimer()
    {
        SetTimer(INFINITE);
    }
    inline DWORD GetTimer() const
    {
        return m_dwTimer;
    }
protected:
	virtual bool OnStart()
	{
		return true;
	}
	virtual void OnStop()
	{
	}
	virtual void OnTimer()
	{
	}
	virtual void OnEvent()
	{
	}
protected:
	static unsigned int __stdcall _Run(LPVOID pParam)
	{
		CMyThread* pThis = (CMyThread*)pParam;

        InterlockedCompareExchange((volatile LONG*) &pThis->m_nThreadID, (LONG)::GetCurrentThreadId(), 0);
        ATLASSERT(pThis->m_nThreadID == ::GetThreadId(::GetCurrentThread()));

        pThis->Run();

        if (pThis->m_bSelfDelete)
        {
            pThis->m_hThread.Close();
            pThis->m_nThreadID = 0;
            delete pThis;
        }
        _endthreadex(0);
        return 0;
    }
	inline void Run()
	{
		if (OnStart())
		{
			bool bRun = true;

			do
			{
				switch(::WaitForMultipleObjects(m_hEvent ? 3 : 2, &m_hStopEvent.m_h, FALSE, m_dwTimer))
				{
					case WAIT_OBJECT_0:
					{
						OnStop();
						bRun = false;
					}
					break;

                    case (WAIT_OBJECT_0+1):
                    {
                        // reschedule timer
                    }
                    break;
                    
                    case WAIT_TIMEOUT:
					{
						OnTimer();
					}
					break;

					default:
					{
						OnEvent();
					}
					break;
				}
			} while (bRun);
		}
	}
protected:
	CHandle					m_hThread;
	CEvent					m_hStopEvent;
    CEvent                  m_hRescheduleEvent;
    CEvent					m_hEvent;
    ULONG                   m_nThreadID;
	bool					m_bSelfDelete;
	DWORD					m_dwTimer;
};



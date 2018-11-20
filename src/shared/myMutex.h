/*
 *
 *  myMutex.h 
 *
 */

#pragma once

#include <atlsync.h>


///////////////////////////////////////////////////////////////////////
// AutoSync

template<class T>
class AutoSyncT
{
public:
    AutoSyncT(T* pObjectToLock, bool bExclusive = true)
    {
        m_pObjectToLock = NULL;

        if (pObjectToLock)
            Lock(pObjectToLock, bExclusive);
    }
    AutoSyncT(T& ObjectToLock, bool bExclusive = true)
    {
        m_pObjectToLock = NULL;
        Lock(&ObjectToLock, bExclusive);
    }
    ~AutoSyncT()
    {
        Unlock();
    }
public:
    inline void Lock(T* pObjectToLock, bool bExclusive = true)
    {
        ATLASSERT(!m_pObjectToLock);
        m_pObjectToLock = pObjectToLock;
        ATLASSERT(m_pObjectToLock);
        m_pObjectToLock->Lock(bExclusive);
    }
    inline void Lock(T& ObjectToLock, bool bExclusive = true)
    {
        Lock(&ObjectToLock, bExclusive)
    }
    inline void Unlock()
    {
        if (m_pObjectToLock)
        {
            m_pObjectToLock->Unlock();
            m_pObjectToLock = NULL;
        }
    }
public:
    T*	m_pObjectToLock;
};

///////////////////////////////////////////////////////////////////////
// CNoLock

class CNoLock
{
public:
    typedef AutoSyncT<CNoLock> AutoSync;

public:
    void Lock(bool bExclusive = true)
    {
        UNREFERENCED_PARAMETER(bExclusive);
    }
    void Unlock()
    {
    }
    bool TryLock(bool bExclusive = true)
    {
        UNREFERENCED_PARAMETER(bExclusive);
        return true;
    }
};

///////////////////////////////////////////////////////////////////////
// CMyMutex

class CMyMutex : public ATL::CCriticalSection
{
public:
    typedef AutoSyncT<CMyMutex> AutoSync;

public:
	inline void Lock(bool bExclusive = true)
	{
		UNREFERENCED_PARAMETER(bExclusive);
		Enter();
	}
	inline void Unlock()
	{
		Leave();
	}
    bool TryLock(bool bExclusive = true)
    {
        UNREFERENCED_PARAMETER(bExclusive);
        return !!TryEnter();
    }
};



///////////////////////////////////////////////////////////////////////
// myQueue.h

#pragma once

#include "myMutex.h"

///////////////////////////////////////////////////////////////////////
// CMyQueue

template<class T, class __LockClass = CMyMutex>
class CMyQueue : public std::list<T>
{
public:
    CMyQueue(bool bManualResetEvent = false, size_t nMinFill = 1) : m_bManualResetEvent(bManualResetEvent), m_nMinFill(nMinFill)
    {
        m_bIsFlushing = false;
        ATLVERIFY(m_hEvent.Create(NULL, !!bManualResetEvent, FALSE, NULL));
    }
public:
    inline bool queue(const T& item)
    {
        __LockClass::AutoSync sync(m_mtx, true);

        if (!m_bIsFlushing)
        {
            push_back(item);

            if (__super::size() >= m_nMinFill)
                m_hEvent.Set();
            return true;
        }
        return false;
    }
    inline bool unqueue(T& item)
    {
        __LockClass::AutoSync sync(m_mtx, true);

        if (!__super::empty())
        {
            item = front();
            pop_front();

            if (m_bManualResetEvent && __super::size() < m_nMinFill && !m_bIsFlushing)
                m_hEvent.Reset();
            return true;
        }
        return false;
    }
    inline bool peek(T& item) const
    {
        __LockClass::AutoSync sync(m_mtx, false);

        if (!__super::empty())
        {
            item = front();
            return true;
        }
        return false;
    }
    inline bool unqueue()
    {
        __LockClass::AutoSync sync(m_mtx, true);

        if (!__super::empty())
        {
            T volatile item = front();
            pop_front();

            if (m_bManualResetEvent && __super::size() < m_nMinFill && !m_bIsFlushing)
                m_hEvent.Reset();
            sync.Unlock();
            return true;
        }
        return false;
    }
    inline size_t size(void) const
    {
        __LockClass::AutoSync sync(m_mtx, false);
        return __super::size();
    }
    inline size_t count(void) const
    {
        return size();
    }
    inline bool flushed() const
    {
        __LockClass::AutoSync sync(m_mtx, false);
        return (m_bIsFlushing && _super::empty()) ? true : false;
    }
    inline void flush(bool bFlush = true)
    {
        __LockClass::AutoSync sync(m_mtx, true);
        m_bIsFlushing = bFlush;

        if (bFlush)
            m_hEvent.Set();
    }
    inline bool flushing() const
    {
        __LockClass::AutoSync sync(m_mtx, false);
        return m_bIsFlushing;
    }
    inline bool empty() const
    {
        __LockClass::AutoSync sync(m_mtx, false);
        return __super::empty() ? true : false;
    }
    inline void clear()
    {
        __LockClass::AutoSync sync(m_mtx, true);
        m_hEvent.Reset();
        __super::clear();
        m_bIsFlushing = false;
    }
    inline void Lock(bool bExcusive = true) const
    {
        m_mtx.Lock(bExcusive);
    }
    inline void Unlock() const
    {
        m_mtx.Unlock();
    }
    inline size_t GetMinFill() const
    {
        __LockClass::AutoSync sync(m_mtx, false);
        return m_nMinFill;
    }
    inline void SetMinFill(size_t nMinFill)
    {
        __LockClass::AutoSync sync(m_mtx, true);
        
        m_nMinFill = nMinFill;

        if (__super::size() >= m_nMinFill)
            m_hEvent.Set();
    }
public:
    CEvent							m_hEvent;
    const bool                      m_bManualResetEvent;
protected:
    size_t                          m_nMinFill;
    mutable __LockClass				m_mtx;
    bool							m_bIsFlushing;
};


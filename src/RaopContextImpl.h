////////////////////////////////////////////////////////////////////////////
// RaopContextImpl.h

#pragma once


class CHairTunes;

class CRaopContext : public IRaopContext
{
public:
	CRaopContext()
	{
		m_bIsStreaming	= false;
		m_bV4			= true;
		_m_bmInfo		= NULL;
		m_nBitmapBytes	= 0;
		m_rtpStart		= 0;
		m_rtpEnd		= 0;

		Announce();
	}
	CRaopContext(bool bV4, const char* strPeerIP, const char* strLocalIP)
	{
		m_bIsStreaming	= false;
		m_bV4			= bV4;
		_m_bmInfo		= NULL;
		m_nBitmapBytes	= 0;
		m_rtpStart		= 0;
		m_rtpEnd		= 0;

		if (strPeerIP)
			m_strPeerIP = strPeerIP;

		if (strLocalIP)
			m_strLocalIP = strLocalIP;

		Announce();
	}
	~CRaopContext()
	{
		if (m_pDecoder)
			m_pDecoder.reset();

		if (_m_bmInfo)
			delete _m_bmInfo;
	}
	void Lock(bool bExclusive)
	{
		m_mtx.Lock(bExclusive);
	}
	void Lock()
	{
		m_mtx.Lock(true);
	}
	void Unlock()
	{
		m_mtx.Unlock();
	}
	bool TryLock()
	{
		return m_mtx.TryLock() ? true : false;
	}
	void Announce()
	{
		Lock();

		m_lfFreq = 44100.0l;

		_m_nTimeStamp = ::time(NULL);
		_m_nTimeTotal = 0;
		_m_nTimeCurrentPos = 0;

		_m_nDurHours = 0;
		_m_nDurMins = 0;
		_m_nDurSecs = 0;

		if (_m_bmInfo)
		{
			delete _m_bmInfo;
			_m_bmInfo		= NULL;
			m_nBitmapBytes	= 0;
		}
		Unlock();
	}
	void ResetSongInfos()
	{
		Lock();

		_m_str_daap_songalbum.Empty();
		_m_str_daap_songartist.Empty();
		_m_str_daap_trackname.Empty();

		Unlock();
	}
	time_t GetTimeStamp()
	{
		Lock();
		time_t tResult = _m_nTimeStamp;
		Unlock();

		return tResult;
	}
	void PutTimeStamp(time_t tValue)
	{
		Lock();
		_m_nTimeStamp = tValue;
		Unlock();
	}
	time_t GetTimeTotal()
	{
		Lock();
		time_t tResult = _m_nTimeTotal;
		Unlock();

		return tResult;
	}
	void PutTimeTotal(time_t tValue)
	{
		Lock();
		_m_nTimeTotal = tValue;
		Unlock();
	}
	time_t GetTimeCurrentPos()
	{
		Lock();
		time_t tResult = _m_nTimeCurrentPos;
		Unlock();

		return tResult;
	}
	void PutTimeCurrentPos(time_t tValue)
	{
		Lock();
		_m_nTimeCurrentPos = tValue;
		Unlock();
	}
	long GetDurHours()
	{
		Lock();
		long nResult = _m_nDurHours;
		Unlock();

		return nResult;
	}
	void PutDurHours(long nValue)
	{
		Lock();
		_m_nDurHours = nValue;
		Unlock();
	}
	long GetDurMins()
	{
		Lock();
		long nResult = _m_nDurMins;
		Unlock();

		return nResult;
	}
	void PutDurMins(long nValue)
	{
		Lock();
		_m_nDurMins = nValue;
		Unlock();
	}
	long GetDurSecs()
	{
		Lock();
		long nResult = _m_nDurSecs;
		Unlock();

		return nResult;
	}
	void PutDurSecs(long nValue)
	{
		Lock();
		_m_nDurSecs = nValue;
		Unlock();
	}
	ATL::CString GetSongalbum()
	{
		Lock();
		ATL::CString strResult = _m_str_daap_songalbum;
		Unlock();

		return strResult;
	}
	long	GetSongalbumCount()
	{
		Lock();
		long nResult = (long)_m_str_daap_songalbum.GetLength();
		Unlock();

		return nResult;
	}
	void PutSongalbum(ATL::CString strValue)
	{
		Lock();
		_m_str_daap_songalbum = strValue;
		Unlock();
	}
	ATL::CString GetSongartist()
	{
		Lock();
		ATL::CString strResult = _m_str_daap_songartist;
		Unlock();

		return strResult;
	}
	long	GetSongartistCount()
	{
		Lock();
		long nResult = (long)_m_str_daap_songartist.GetLength();
		Unlock();

		return nResult;
	}
	void PutSongartist(ATL::CString strValue)
	{
		Lock();
		_m_str_daap_songartist = strValue;
		Unlock();
	}
	ATL::CString GetSongtrack()
	{
		Lock();
		ATL::CString strResult = _m_str_daap_trackname;
		Unlock();

		return strResult;
	}
	long	GetSongtrackCount()
	{
		Lock();
		long nResult = (long)_m_str_daap_trackname.GetLength();
		Unlock();

		return nResult;
	}
	void PutSongtrack(ATL::CString strValue)
	{
		Lock();
		_m_str_daap_trackname = strValue;
		Unlock();
	}
	void GetSongalbum(PWSTR pBuf, ULONG nMax)
	{
		Lock();
		wcsncpy_s(pBuf, nMax, _m_str_daap_songalbum, _TRUNCATE);
		Unlock();
	}
	void PutSongalbum(PCWSTR strValue)
	{
		if (strValue)
			PutSongalbum(ATL::CString(strValue));
	}
	void GetSongartist(PWSTR pBuf, ULONG nMax)
	{
		Lock();
		wcsncpy_s(pBuf, nMax, _m_str_daap_songartist, _TRUNCATE);
		Unlock();
	}
	void PutSongartist(PCWSTR strValue)
	{
		if (strValue)
			PutSongartist(ATL::CString(strValue));
	}
	void GetSongtrack(PWSTR pBuf, ULONG nMax)
	{
		Lock();
		wcsncpy_s(pBuf, nMax, _m_str_daap_trackname, _TRUNCATE);
		Unlock();
	}
	void PutSongtrack(PCWSTR strValue)
	{
		if (strValue)
			PutSongtrack(ATL::CString(strValue));
	}
	Bitmap* GetBitmap()
	{
		// you need to lock yourself
		return _m_bmInfo;
	}
	void PutBitmap(Bitmap* bmValue)
	{
		ATLASSERT(_m_bmInfo == NULL);

		Lock();
		_m_bmInfo = bmValue;

		if (_m_bmInfo == NULL)
			m_nBitmapBytes = 0;
		Unlock();
	}
	BYTE*	GetBitmapBytes()
	{
		return m_binBitmap;
	}
	long	GetBitmapByteCount()
	{
		return m_nBitmapBytes;
	}
	bool GetHICON(HICON* phIcon)
	{
		bool bResult = false;

		Lock();

		if (_m_bmInfo)
			bResult = _m_bmInfo->GetHICON(phIcon) == Gdiplus::Ok ? true : false;
		Unlock();

		return bResult;
	}
	void DeleteImage()
	{
		Lock();

		if (_m_bmInfo)
		{
			delete _m_bmInfo;
			_m_bmInfo = NULL;
		}
		m_nBitmapBytes = 0;

		Unlock();
	}
	void Duplicate(std::shared_ptr<CRaopContext>& pRaopContext)
	{
		ATLASSERT(!pRaopContext);
		pRaopContext = std::make_shared<CRaopContext>();

		if (pRaopContext)
		{
			Lock();

			pRaopContext->m_bV4 = m_bV4;
			pRaopContext->m_lfFreq = m_lfFreq;

			Unlock();
		}
	}
	unsigned long long&		GetRtpStart()
	{
		return m_rtpStart;
	}
	unsigned long long&		GetRtpCur()
	{
		return m_rtpCur;
	}
	unsigned long long&		GetRtpEnd()
	{
		return m_rtpEnd;
	}
	double&					GetFreq()
	{
		return m_lfFreq;
	}
public:

	CMyMutex				    m_mtx;
	CHttp					    m_CurrentHttpRequest;
	CTempBuffer<BYTE>		    m_Aesiv;
    ULONG			            m_sizeAesiv;
	CTempBuffer<BYTE>		    m_Rsaaeskey;
	ULONG					    m_sizeRsaaeskey;
	std::string				    m_strFmtp;
	std::string				    m_strCport;
	std::string				    m_strTport;
	bool					    m_bIsStreaming;
	std::string				    m_strNonce;
	bool					    m_bV4;
	ic_string				    m_strPeerIP;
	ic_string				    m_strLocalIP;
	SOCKADDR_IN6			    m_Peer;
	std::list<std::string>	    m_listFmtp;
	double					    m_lfFreq;
	std::shared_ptr<CHairTunes>	m_pDecoder;
	CTempBuffer<BYTE>		    m_binBitmap;
	long					    m_nBitmapBytes;
	unsigned long long		    m_rtpStart;
	unsigned long long		    m_rtpCur;
	unsigned long long		    m_rtpEnd;
	ATL::CString			    m_strUserAgent;
	ATL::CString			    m_strPeerHostName;

	__declspec(property(get = GetTimeStamp, put = PutTimeStamp))				time_t			m_nTimeStamp;
	__declspec(property(get = GetTimeTotal, put = PutTimeTotal))				time_t			m_nTimeTotal;
	__declspec(property(get = GetTimeCurrentPos, put = PutTimeCurrentPos))		time_t			m_nTimeCurrentPos;
	__declspec(property(get = GetDurHours, put = PutDurHours))					long			m_nDurHours;
	__declspec(property(get = GetDurMins, put = PutDurMins))					long			m_nDurMins;
	__declspec(property(get = GetDurSecs, put = PutDurSecs))					long			m_nDurSecs;
	__declspec(property(get = GetSongalbum, put = PutSongalbum))				ATL::CString	m_str_daap_songalbum;
	__declspec(property(get = GetSongartist, put = PutSongartist))				ATL::CString	m_str_daap_songartist;
	__declspec(property(get = GetSongtrack, put = PutSongtrack))				ATL::CString	m_str_daap_trackname;
	__declspec(property(get = GetBitmap, put = PutBitmap))						Bitmap*			m_bmInfo;

protected:
	volatile time_t																_m_nTimeStamp;			// in Secs
	volatile time_t																_m_nTimeTotal;			// in Secs
	volatile time_t																_m_nTimeCurrentPos;		// in Secs
	volatile long																_m_nDurHours;
	volatile long																_m_nDurMins;
	volatile long																_m_nDurSecs;
	Bitmap*	volatile															_m_bmInfo;
	ATL::CString																_m_str_daap_songalbum;
	ATL::CString																_m_str_daap_songartist;
	ATL::CString																_m_str_daap_trackname;
};
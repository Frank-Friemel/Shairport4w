#include "stdafx.h"
#include <mmsystem.h>
#include <mmreg.h>
#include <Mmdeviceapi.h>
#include <FunctionDiscoveryKeys_devpkey.h>

#include "AudioPlayer.h"
#include "utils.h"


#pragma comment(lib, "Winmm.lib")


////////////////////////////////////////////////////////////////
// CWaveQueueItem

class CWaveQueueItem
{
public:
	CWaveQueueItem()
	{
		m_nLen		= 0;
		InitNew();
	}
	inline bool IsPlayed() const
	{
		return m_bPlayed;
	}
	inline bool IsDone() const
	{
		return m_WaveHeader.dwFlags & WHDR_DONE ? true : false;
	}
	inline void InitNew()
	{
		memset(&m_WaveHeader, 0, sizeof(m_WaveHeader));
		m_bPlayed	= false;
	}
	bool Prepare(HWAVEOUT hWaveOut);
	void UnPrepare(HWAVEOUT hWaveOut);
	bool Play(HWAVEOUT hWaveOut);
	void WaitIfPlayed();
	void Mute();

public:
	CTempBuffer<BYTE>			m_Blob;
	ULONG						m_nLen;

private:
	WAVEHDR						m_WaveHeader;
	bool						m_bPlayed;
};

///////////////////////////////////////////////////////////
// CWavePlayThread

class CWavePlayThread : protected CMyThread
{
public:
	typedef std::function<void(BYTE* pStream, ULONG dwLen)>	callback_t;

	CWavePlayThread(UINT nDeviceID = WAVE_MAPPER);
	~CWavePlayThread();

	void Init(ULONG nFreq = (ULONG)SAMPLE_FREQ, ULONG nChannels = NUM_CHANNELS, ULONG nSampleSizeBits = SAMPLE_SIZE);
	bool Start(callback_t callback, size_t nThreshold = 16);
	void Stop();

	void		Pause();
	inline bool IsPaused() const
	{
		return m_bPaused;
	}
	void		Mute(bool bMute);
	inline bool IsMuted() const
	{
		return m_bMuted;
	}
	void SetVolume(WORD wVolume);
	WORD GetVolume();

private:
	bool Alloc(std::shared_ptr<CWaveQueueItem>& pItem);
	void Free(std::shared_ptr<CWaveQueueItem>& pItem);

protected:
	virtual bool OnStart();
	virtual void OnStop();
	virtual void OnEvent();

protected:
	HWAVEOUT				m_hWaveOut;

	typedef CMyQueue< std::shared_ptr<CWaveQueueItem>, CNoLock >	CWaveQueueItemList;

	CWaveQueueItemList		m_Queue;
	CWaveQueueItemList		m_Pool;

	CHandle					m_hStarted;
	UINT					m_nDeviceID;
	size_t					m_nThreshold;
	volatile bool			m_bPaused;
	volatile bool			m_bMuted;
	callback_t				m_callback;

public:
	WAVEFORMATEX			m_format;
	UINT					m_nRealDeviceID;
};


/////////////////////////////////////////////////////////////////////////////////
// CWaveQueueItem

bool CWaveQueueItem::Prepare(HWAVEOUT hWaveOut)
{
	bool bResult = true;

	if ((m_WaveHeader.dwFlags & WHDR_PREPARED) == 0)
	{
		m_WaveHeader.dwBufferLength	= m_nLen;
		m_WaveHeader.lpData			= (LPSTR)(BYTE*) m_Blob;

		if (MMSYSERR_NOERROR != waveOutPrepareHeader(hWaveOut, &m_WaveHeader, sizeof(m_WaveHeader)))
		{
			bResult = false;
			memset(&m_WaveHeader, 0, sizeof(m_WaveHeader));
		}
	}
	else
	{
		m_WaveHeader.dwBufferLength	= m_nLen;
	}
	m_bPlayed	= false;

	return bResult;
}


void CWaveQueueItem::UnPrepare(HWAVEOUT hWaveOut)
{
	if (m_WaveHeader.dwFlags & WHDR_PREPARED)
	{
		while (waveOutUnprepareHeader(hWaveOut, &m_WaveHeader, sizeof(m_WaveHeader)) != MMSYSERR_NOERROR)
			Sleep(5);
		InitNew();
	}
}

bool CWaveQueueItem::Play(HWAVEOUT hWaveOut)
{
	m_WaveHeader.dwFlags &= ~WHDR_DONE;

	if (waveOutWrite(hWaveOut, &m_WaveHeader, sizeof(WAVEHDR)) == MMSYSERR_NOERROR)
	{
		m_bPlayed = true;
		return true;
	}
	return false;
}

void CWaveQueueItem::WaitIfPlayed()
{
	if (m_bPlayed)
	{
		while (!(m_WaveHeader.dwFlags & WHDR_DONE))
		{
			Sleep(5);
		}
	}
}

void  CWaveQueueItem::Mute()
{
	memset(m_Blob, 0, m_nLen);
}

/////////////////////////////////////////////////////////////////////////////////
// CWavePlayThread

CWavePlayThread::CWavePlayThread(UINT nDeviceID /*= WAVE_MAPPER*/)
{
	m_hWaveOut		= NULL;
	m_nDeviceID		= nDeviceID;
	m_nRealDeviceID	= 0;
	m_nThreshold	= 16;
	Init();
	m_bPaused		= false;
	m_bMuted		= false;
}

CWavePlayThread::~CWavePlayThread()
{
	Stop();
}

void CWavePlayThread::Init(ULONG nFreq /*= 44100*/, ULONG nChannels /*= 2*/, ULONG nSampleSizeBits /*= 16*/)
{
	memset(&m_format, 0, sizeof(m_format));

	m_format.cbSize				= 0;
	m_format.wFormatTag			= WAVE_FORMAT_PCM;
	m_format.nSamplesPerSec		= nFreq;
	m_format.nChannels			= (WORD) nChannels;
	m_format.wBitsPerSample		= (WORD) nSampleSizeBits;
	m_format.nBlockAlign		= (WORD) (nChannels * (nSampleSizeBits >> 3));
	m_format.nAvgBytesPerSec	= nFreq * m_format.nBlockAlign;

	m_Pool.clear();
}

bool CWavePlayThread::Alloc(std::shared_ptr<CWaveQueueItem>& pItem)
{
	if (pItem)
		pItem.reset();

	m_Pool.unqueue(pItem);

	if (!pItem)
	{
		pItem = std::make_shared<CWaveQueueItem>();

		if (!pItem)
			return false;

		pItem->m_nLen = 4096;

		if (!pItem->m_Blob.Allocate(pItem->m_nLen))
			return false;
	}
	memset(pItem->m_Blob, 0, pItem->m_nLen);
	return true;
}

void CWavePlayThread::Free(std::shared_ptr<CWaveQueueItem>& pItem)
{
	ATLASSERT(pItem);
	pItem->InitNew();
	m_Pool.queue(pItem);
	pItem.reset();
}

bool CWavePlayThread::Start(callback_t callback, size_t nThreshold /*= 16*/)
{
	if (IsRunning())
		return true;
	bool bResult = false;

	m_nThreshold	= nThreshold;
	m_callback		= callback;

	ATLASSERT(m_callback);

	// create a manual reset event
	ATLASSERT(!m_hStarted);
	m_hStarted.Attach(::CreateEvent(NULL, TRUE, FALSE, NULL));
	
	CHandle hEvent(::CreateEvent(NULL, TRUE, FALSE, NULL));

	if (__super::Start(hEvent))
	{
		// sync with OnStart
		::WaitForSingleObject(m_hStarted, INFINITE);

		// succeeded?
		bResult = m_hWaveOut != NULL ? true : false;

		if (!bResult)
			Stop();
	}
	m_hStarted.Close();

	return bResult;
}

void CWavePlayThread::Stop()
{
	__super::Stop();
}

bool CWavePlayThread::OnStart()
{
	bool bResult = false;

	ATLASSERT(m_hWaveOut == NULL);
	m_bPaused	= false;
	m_bMuted	= false;

	if (MMSYSERR_NOERROR != waveOutOpen(&m_hWaveOut, m_nDeviceID, &m_format, (DWORD_PTR)(HANDLE)m_hEvent, NULL, CALLBACK_EVENT))
	{
		m_hWaveOut = NULL;
	}
	else
	{
		// suspend player, so we can control when to start
		if (waveOutPause(m_hWaveOut) == MMSYSERR_NOERROR)
		{
			::ResetEvent(m_hEvent);
			m_bPaused = true;
		}
		else
		{
			// unexpected
			ATLASSERT(FALSE);
			::SetEvent(m_hEvent);
		}
		bResult = true;

		if (MMSYSERR_NOERROR != waveOutGetID(m_hWaveOut, &m_nRealDeviceID)) 
		{
			m_nRealDeviceID = m_nDeviceID;
		}
	}
	::SetEvent(m_hStarted);
	return bResult;
}


void CWavePlayThread::OnStop()
{
	if (m_hWaveOut)
	{
		// waveOutReset will abort all queued items
		ATLVERIFY(waveOutReset(m_hWaveOut) == MMSYSERR_NOERROR);

		std::shared_ptr<CWaveQueueItem> pItem;

		// wait for abortion to complete
		while(m_Queue.unqueue(pItem))
		{
			pItem->WaitIfPlayed();
			pItem->UnPrepare(m_hWaveOut);
			pItem.reset();
		}
		m_Pool.clear();
		ATLVERIFY(waveOutClose(m_hWaveOut) == MMSYSERR_NOERROR);
		
		m_hWaveOut	= NULL;
		m_bPaused	= false;
	}
}

void CWavePlayThread::OnEvent()
{
	::ResetEvent(m_hEvent);

	std::shared_ptr<CWaveQueueItem> pItem;

	while (m_Queue.peek(pItem))
	{
		if (pItem->IsDone())
		{
			m_Queue.unqueue();
			Free(pItem);
		}
		else
		{
			pItem.reset();
			break;
		}
	}
	if (!m_bPaused)
	{
		while (m_nThreshold >= m_Queue.size())
		{
			if (!Alloc(pItem))
				break;
			m_callback(pItem->m_Blob, pItem->m_nLen);

			if (pItem->Prepare(m_hWaveOut))
			{
				m_Queue.queue(pItem);
			}
			else
			{
				ATLASSERT(FALSE);
			}
		}
		for (auto i = m_Queue.begin(); i != m_Queue.end(); ++i)
		{
			if (!(*i)->IsPlayed())
			{
				if (m_bMuted)
				{
					(*i)->Mute();
				}
				(*i)->Play(m_hWaveOut);
			}
		}		
	}
}

void CWavePlayThread::Pause()
{
	if (m_hWaveOut != NULL)
	{
		if (m_bPaused)
		{
			m_bPaused = false;

			// signal worker thread to fill the queue
			::SetEvent(m_hEvent);
			Sleep(0);

			ATLVERIFY(waveOutRestart(m_hWaveOut) == MMSYSERR_NOERROR);
		}
		else
		{
			if (waveOutPause(m_hWaveOut) == MMSYSERR_NOERROR)
			{
				m_bPaused = true;
				::ResetEvent(m_hEvent);
			}
		}
	}
}

void CWavePlayThread::Mute(bool bMute)
{
	m_bMuted = bMute;
}

void CWavePlayThread::SetVolume(WORD wVolume)
{
	ATLASSERT(m_hWaveOut != NULL);

	DWORD dwVolume = ((DWORD) wVolume) | (((DWORD) wVolume) << 16);
	ATLVERIFY(waveOutSetVolume(m_hWaveOut, dwVolume) == MMSYSERR_NOERROR);
}

WORD CWavePlayThread::GetVolume()
{
	ATLASSERT(m_hWaveOut != NULL);

	DWORD dwVolume = 0;
	ATLVERIFY(waveOutGetVolume(m_hWaveOut, &dwVolume) == MMSYSERR_NOERROR);
	return (WORD) (dwVolume & 0x0000ffff);
}

//////////////////////////////////////////////////////////
// CAudioPlayer

CAudioPlayer::CAudioPlayer()
{
}

CAudioPlayer::~CAudioPlayer()
{
	Close();
}

void CAudioPlayer::Mute(bool bMute)
{
	if (m_threadWavePlay)
		m_threadWavePlay->Mute(bMute);
}

bool CAudioPlayer::IsMuted() const
{
	if (m_threadWavePlay)
		return m_threadWavePlay->IsMuted();
	return false;
}

bool CAudioPlayer::Init(HANDLE hDataSource, UINT nDeviceID /*= WAVE_MAPPER*/, ULONG nFreq /*= 44100*/, ULONG nChannels /*= 2*/, ULONG nSampleSizeBits /*= 16*/)
{
	if (!hDataSource)
	{
		::SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	Close();

	m_hAudioData.Attach(DuplicateHandle(hDataSource));

	if (!m_hAudioData)
		return false;
	
	m_threadWavePlay = std::make_shared<CWavePlayThread>(nDeviceID);

	if (!m_threadWavePlay)
	{
		Close();
		return false;
	}
	m_threadWavePlay->Init(nFreq, nChannels, nSampleSizeBits);
	return m_threadWavePlay->Start([this](BYTE* pStream, ULONG dwLen) -> void { OnPlayAudio(pStream, dwLen); });
}

void CAudioPlayer::Play()
{
	if (m_threadWavePlay && m_threadWavePlay->IsPaused())
		m_threadWavePlay->Pause();
}

void CAudioPlayer::Pause()
{
	if (m_threadWavePlay && !m_threadWavePlay->IsPaused())
		m_threadWavePlay->Pause();
}

void CAudioPlayer::Close()
{
	if (m_threadWavePlay)
	{
		m_threadWavePlay->Stop();
		m_threadWavePlay.reset();
	}
	if (m_hAudioData)
		m_hAudioData.Close();
}

void CAudioPlayer::OnPlayAudio(BYTE* pStream, ULONG dwLen)
{
	if (dwLen)
	{
		memset(pStream, 0, dwLen);

		int		nTry		= 5;
		DWORD	dwRead		= 0;
		DWORD	dwReadTotal	= 0;

		// read PCM data from handle directly to the sound buffer in mem
		while (nTry > 0 && dwLen && ::ReadFile(m_hAudioData, pStream + dwReadTotal, dwLen, &dwRead, NULL))
		{
			if (dwRead == 0)
			{
				// the input handle may be a pipe ... give it some time to deliver the data (over all 25ms)
				nTry--;
				Sleep(5);
			}
			else
			{
				nTry		= 5;
				dwLen		-= dwRead;
				dwReadTotal += dwRead;
			}
		}
	}
}

void CAudioPlayer::GetFullAudioDeviceByShortName(std::wstring& strDevname, CComVariant* pVarAudioID /* = NULL */)
{
    if (strDevname.empty())
        return;

    size_t nLength = strDevname.length();

    std::wstring strBestMatch;

    IMMDeviceEnumerator* pEnumerator = NULL;

    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator)))
    {
        IMMDeviceCollection* pCollection = NULL;

        if (SUCCEEDED(pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &pCollection)))
        {
            UINT count = 0;

            pCollection->GetCount(&count);

            for (UINT i = 0; i < count; ++i)
            {
                IMMDevice* pEndPoint = NULL;

                if (SUCCEEDED(pCollection->Item(i, &pEndPoint)))
                {
                    IPropertyStore* pProps = NULL;

                    if (SUCCEEDED(pEndPoint->OpenPropertyStore(STGM_READ, &pProps)))
                    {
                        PROPVARIANT varName;
                        PropVariantInit(&varName);

                        if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName)))
                        {
                            if (nLength <= wcslen(varName.pwszVal))
                            {
                                if (_wcsnicmp(varName.pwszVal, strDevname.c_str(), nLength) == 0)
                                {
                                    if (wcslen(varName.pwszVal) > strBestMatch.length())
                                    {
                                        strBestMatch = varName.pwszVal;

                                        if (pVarAudioID)
                                        {
                                            PWSTR pStrID = NULL;

                                            if (SUCCEEDED(pEndPoint->GetId(&pStrID)))
                                            {
                                                *pVarAudioID = pStrID;
                                                CoTaskMemFree(pStrID);
                                            }
                                        }
                                    }
                                }
                            }
                            PropVariantClear(&varName);
                        }
                        pProps->Release();
                    }
                    pEndPoint->Release();
                }
            }
            pCollection->Release();
        }
        pEnumerator->Release();
    }
    if (!strBestMatch.empty())
        strDevname = strBestMatch;
    if (pVarAudioID && pVarAudioID->vt == VT_EMPTY)
        *pVarAudioID = strDevname.c_str();
}

int CAudioPlayer::GetWaveMapperID(CComVariant varSoundcardID, CComVariant* pVarAudioID /* = NULL */)
{
    int nResult = (int)WAVE_MAPPER;

    switch (varSoundcardID.vt)
    {
        case VT_EMPTY:
        {
        }
        break;

        case VT_BSTR:
        {
            USES_CONVERSION;

            PCWSTR strSoundcardID = OLE2CW(varSoundcardID.bstrVal);

            if (strSoundcardID && wcslen(strSoundcardID))
            {
                int nID = (int)wcslen(strSoundcardID);

                WAVEOUTCAPSW caps;

                UINT n = waveOutGetNumDevs();

                for (UINT i = 0; i < n; ++i)
                {
                    USES_CONVERSION;

                    if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
                        break;

                    ATL::CString strDevname(caps.szPname);

                    if (nID >= strDevname.GetLength() && wcsncmp(strSoundcardID, strDevname, strDevname.GetLength()) == 0)
                    {
                        nResult = (int)i;
                        break;
                    }
                }
                if (nResult == (int)WAVE_MAPPER)
                {
                    // then strSoundcardName is MapperID+1
                    nResult = _wtoi(strSoundcardID) - 1;
                }
            }
        }
        break;

        default:
        {
            nResult = (int)varSoundcardID.lVal;
        }
        break;
    }
    if (pVarAudioID)
    {
        WAVEOUTCAPSW caps;

        if (waveOutGetDevCapsW(nResult, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            std::wstring strDevname(caps.szPname);

            GetFullAudioDeviceByShortName(strDevname, pVarAudioID);
        }
    }
    return nResult;
}

void CAudioPlayer::GetDeviceList(std::map< UINT, std::pair<std::wstring, CComVariant> >& mapDevices)
{
    WAVEOUTCAPSW caps;

    UINT n = waveOutGetNumDevs();

    // i is the "WaveMapperID"
    for (UINT i = 0; i < n; ++i)
    {
        USES_CONVERSION;

        if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
            break;

        std::wstring strDevname(caps.szPname);

        CComVariant varSoundcardID;

        GetFullAudioDeviceByShortName(strDevname, &varSoundcardID);

        if (!strDevname.empty() && varSoundcardID.vt != VT_EMPTY)
        {
            if (std::find_if(mapDevices.begin(), mapDevices.end(), [&](auto i)->bool { return i.second.second == varSoundcardID ? true : false; }) == mapDevices.end())
                mapDevices[i] = std::pair<std::wstring, CComVariant>(strDevname, varSoundcardID);
        }
    }
}
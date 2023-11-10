/*
 *
 *  HairTunes.h
 *
 */

#pragma once

#include "RaopDefs.h"
#include "AudioPlayer.h"
#include "myQueue.h"
#include <atomic>

#define BUFFER_FRAMES	512
#define START_FILL		282
#define	MIN_FILL		0
#define	MAX_FILL		500

class CRaopContext;
struct alac_file;

class CHairTunes : protected CMyThread, protected IRtpRequestHandler
{
public:
	class Volume final
	{
	public:
		Volume()
		{
			m_volume = 1.0l;
		}

		short Transform(short sample) const
		{
			return static_cast<short>(static_cast<double>(sample) * m_volume);
		}

		void SetVolume(double lfValue)
		{
			const double lfVolume = pow(10.0l, 0.05l * lfValue);
			
			m_volume = lfVolume < 1.0l ? lfVolume : 1.0l;
		}

		Volume(const Volume&) = delete;
		Volume& operator=(const Volume&) = delete;

	private:
		std::atomic<double> m_volume;
	};

public:
    CHairTunes(const std::shared_ptr<Volume>& volume);
    ~CHairTunes();

    static int			GetStartFill();
    static void			SetStartFill(int nValue);

    static CComVariant	GetSoundId();
    static void			SetSoundId(const CComVariant& varValue);


public:
    bool Start(std::shared_ptr<CRaopContext> pContext);
    void Stop();

    void Flush();
    void SetPause(bool bValue);

    USHORT GetServerPort();			// host order
    USHORT GetControlPort();		// host order
    USHORT GetTimingPort();			// host order

    int StuffBuffer(const short* inptr, short* outptr, int nFrameSize, double bf_playback_rate);

    void AllowAudioReconnect(std::function<void(void)> funcOnPlayingCallback, bool bValue = true)
    {
        m_mtxOnPlayingCallback.Lock();
        m_funcOnPlayingCallback = funcOnPlayingCallback;
        m_mtxOnPlayingCallback.Unlock();
    }

protected:
    virtual bool OnStart();
    virtual void OnStop();
    virtual void OnEvent();

protected:
	const std::shared_ptr<Volume>		m_Volume;

    CAudioPlayer                        m_AudioPlayer;
    CHandle                             m_hPcmPipe;
    std::function<void(void)>			m_funcOnPlayingCallback;
    CMyMutex							m_mtxOnPlayingCallback;

    int									m_nControlport;
    int									m_nTimingport;

    CTempBuffer<BYTE>					m_OutBuf;

    my_crypt::Aes					    m_AES;

	std::map<int, int>					m_mapFmtp;
	int									m_nSamplingRate;
	int									m_nFrameSize;
	bool								m_bFlush;
	bool								m_bPause;

	alac_file*							m_decoder_info;

	std::shared_ptr<CRtpEndpoint>		m_RtpClient_Data;
	std::shared_ptr<CRtpEndpoint>		m_RtpClient_Control;
	std::shared_ptr<CRtpEndpoint>		m_RtpClient_Timing;

protected:
    CMyQueue< std::shared_ptr<CRtpPacket> >	m_Queue;

	typedef struct
	{
		double hist[2];
		double a[2];
		double b[3];
	} biquad_t;

	double								m_bf_playback_rate;
	double								m_bf_est_drift;   
	biquad_t							m_bf_drift_lpf;
	double								m_bf_est_err;
	double								m_bf_last_err;
	biquad_t							m_bf_err_lpf;
	biquad_t							m_bf_err_deriv_lpf;

	double								m_desired_fill;
	int									m_fill_count;
	std::weak_ptr<CRaopContext>		    m_pRaopContext;

protected:
	virtual void OnRequest(CRtpEndpoint* pRtpEndpoint, std::shared_ptr<CRtpPacket>& packet);

	inline void QueuePacket(std::shared_ptr<CRtpPacket>& p);
	inline int AlacDecode(unsigned char* pDest, const unsigned char* pBuf, int len);

	void BF_EstReset();
	void BF_EstUpdate(int fill);

private:
	static void biquad_init(biquad_t* bq, double a[], double b[]);
	static void biquad_lpf(biquad_t* bq, double freq, double Q, double sampling_rate, double frame_size);
	static double biquad_filt(biquad_t* bq, double in);

protected:
	class _CResendThread : public CMyThread
	{
	protected:
		virtual bool OnStart()
		{
			m_mtxCheckMap.Lock();
			m_mapCheck.clear();
			m_dwTimer = INFINITE;
			m_mtxCheckMap.Unlock();

			return true;
		}
		virtual void OnStop()
		{
			if (m_RtpClient_Control)
				m_RtpClient_Control.reset();
		}
		virtual void OnTimer()
		{
			std::list<USHORT> listSeqNrToResend;

			m_mtxCheckMap.Lock();

			ATLASSERT(!m_mapCheck.empty() || m_dwTimer == INFINITE);

			for (auto i = m_mapCheck.begin(); i != m_mapCheck.end(); )
			{
				if (i->second > 0)
				{
					i->second--;
					
					listSeqNrToResend.push_back(i->first);
					++i;
				}
				else
				{
					ATLASSERT(i->second == 0);
					m_mapCheck.erase(i++);
				}
			}
			if (m_mapCheck.empty())
				m_dwTimer = INFINITE;

			m_mtxCheckMap.Unlock();

			for (auto i = listSeqNrToResend.begin(); i != listSeqNrToResend.end(); ++i)
			{
				if (!_RequestResend(*i, 1))
				{
					m_mtxCheckMap.Lock();

					m_mapCheck.erase(*i);

					if (m_mapCheck.empty())
						m_dwTimer = INFINITE;

					m_mtxCheckMap.Unlock();
				}
			}
		}
	public:
		inline void Push(USHORT nSeq, short nCount)
		{
			ATLASSERT(nCount > 0);

			m_mtxCheckMap.Lock();

			USHORT nSeqDest = nSeq+nCount;

			for (USHORT n = nSeq; n < nSeqDest; ++n)
			{
				// retry up to 4 times
				m_mapCheck[n] = 4;
			}
			if (!m_mapCheck.empty())
			{
				// send resend request every 15 ms
				m_dwTimer = 15;
				SetEvent(m_hEvent);
			}
			m_mtxCheckMap.Unlock();

			_RequestResend(nSeq, nCount);
		}
		inline bool Pull(USHORT nSeq)
		{
			bool bResult = false;

			m_mtxCheckMap.Lock();

			auto i = m_mapCheck.find(nSeq);

			if (i != m_mapCheck.end())
			{
				bResult = true;
				m_mapCheck.erase(i);
	
				if (m_mapCheck.empty())
					m_dwTimer = INFINITE;
			}
			m_mtxCheckMap.Unlock();

			return bResult;
		}
		inline void Remove(USHORT nSeq)
		{
			if (m_dwTimer != INFINITE)
			{
				m_mtxCheckMap.Lock();

				for (auto i = m_mapCheck.begin(); i != m_mapCheck.end(); )
				{
					short nSeqDiff = nSeq - i->first;

					if (nSeqDiff >= 0)
						m_mapCheck.erase(i++);
					else
						++i;
				}
				if (m_mapCheck.empty())
					m_dwTimer = INFINITE;
				m_mtxCheckMap.Unlock();
			}
		}
	protected:
		bool _RequestResend(USHORT nSeq, short nCount);

	public:
		std::shared_ptr<CRtpEndpoint>	m_RtpClient_Control;
		USHORT						    m_nControlport;
		
	protected:
		CMyMutex					    m_mtxCheckMap;
		std::map<USHORT, int>			m_mapCheck;
	};
protected:
	_CResendThread					    m_threadResend;
};



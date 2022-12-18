/*
 *
 *  HairTunes.cpp
 *
 */

#include "StdAfx.h"
#include "HairTunes.h"

#include "stdint_win.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include "RAOPDefs.h"
#include "alac.h"

#define FRAME_BYTES		(m_nFrameSize<<2)
#define OUTFRAME_BYTES	((m_nFrameSize+3)<<2)
#define CONTROL_A		(1e-4)
#define CONTROL_B		(1e-1)

static LONG			c_nStartFill			= START_FILL;
static CComVariant	c_varSoundCardID;	

static USHORT		c_nUniquePortCounter	= 6000;
static CMyMutex		c_mtxUniquePortCounter;

CHairTunes::CHairTunes(const std::shared_ptr<Volume>& volume) 
	: m_Queue(true)
	, m_Volume(volume)
{
	ATLASSERT(m_Volume);

    m_decoder_info      = NULL;

	m_bFlush			= false;
	m_bPause			= false;

	m_bf_playback_rate	= 1.0l;
	m_bf_est_drift		= 0.0l; 
	m_bf_est_err		= 0.0l;
	m_bf_last_err		= 0.0l;

	memset(&m_bf_drift_lpf, 0, sizeof(CHairTunes::biquad_t));
	memset(&m_bf_err_lpf, 0, sizeof(CHairTunes::biquad_t));
	memset(&m_bf_err_deriv_lpf, 0, sizeof(CHairTunes::biquad_t));
}

CHairTunes::~CHairTunes()
{
	Stop();
}

int CHairTunes::GetStartFill()
{
	return (int)c_nStartFill;
}

void CHairTunes::SetStartFill(int nValue)
{
	InterlockedExchange(&c_nStartFill, nValue);
}

CComVariant CHairTunes::GetSoundId()
{
	return c_varSoundCardID;
}

void CHairTunes::SetSoundId(const CComVariant& varValue)
{
	c_varSoundCardID = varValue;
}

int CHairTunes::StuffBuffer(const short* inptr, short* outptr, int nFrameSize, double bf_playback_rate) 
{
	int		i;
	int		stuffsamp	= nFrameSize;
	int		stuff		= 0;

	if (nFrameSize > 1)
	{
		double	p_stuff		= 1.0l - pow(1.0l - fabs(bf_playback_rate - 1.0l), nFrameSize);

		if (rand() < (p_stuff * RAND_MAX)) 
		{
			stuff		= bf_playback_rate > 1.0l ? -1 : 1;
			stuffsamp	= rand() % (nFrameSize - 1);
		}
	}
	for (i = 0; i < stuffsamp; i++) 
	{   
		// the whole frame, if no stuffing
		*outptr++ = m_Volume->Transform(*inptr++);
		*outptr++ = m_Volume->Transform(*inptr++);
	}
	if (stuff) 
	{
		if (stuff == 1) 
		{
			// interpolate one sample
			*outptr++ = m_Volume->Transform(((long)inptr[-2] + (long)inptr[0]) >> 1);
			*outptr++ = m_Volume->Transform(((long)inptr[-1] + (long)inptr[1]) >> 1);
		} 
		else if (stuff == -1) 
		{
			inptr++;
			inptr++;
		}
		for (i = stuffsamp; i < nFrameSize + stuff; i++) 
		{
			*outptr++ = m_Volume->Transform(*inptr++);
			*outptr++ = m_Volume->Transform(*inptr++);
		}
	}
	return nFrameSize + stuff;
}

int CHairTunes::AlacDecode(unsigned char* pDest, const unsigned char* pBuf, int len)
{
	unsigned char	buf[RAOP_PACKET_MAX_SIZE];
    unsigned char*	packet = buf;

	if (len > RAOP_PACKET_MAX_SIZE)
		packet = new unsigned char[len];

    int aeslen = len & ~0xf;

    BYTE iv[16];

    ATLASSERT(m_AES.m_IvLen == sizeof(iv));

    memcpy(iv, m_AES.m_Iv, sizeof(iv));
    m_AES.Decrypt(pBuf, packet, aeslen, iv, sizeof(iv));

    memcpy(packet+aeslen, pBuf+aeslen, len-aeslen);  

	int outsize;

    decode_frame(m_decoder_info, packet, pDest, &outsize);

    ATLASSERT(outsize <= FRAME_BYTES);

	if (packet != buf)
		delete packet;
	return outsize;
}

void CHairTunes::BF_EstReset() 
{
    biquad_lpf(&m_bf_drift_lpf    , 1.0l / 180.0l, 0.3l , m_nSamplingRate, m_nFrameSize);
    biquad_lpf(&m_bf_err_lpf      , 1.0l / 10.0l , 0.25l, m_nSamplingRate, m_nFrameSize);
    biquad_lpf(&m_bf_err_deriv_lpf, 1.0l / 2.0l  , 0.2l , m_nSamplingRate, m_nFrameSize);
    
	m_fill_count			= 0;
    m_desired_fill			= 0.0l;
    m_bf_playback_rate		= 1.0l;
    m_bf_est_err			= 0.0l;
	m_bf_last_err			= 0.0l;
}

void CHairTunes::BF_EstUpdate(int fill) 
{
	if (m_fill_count < 1000)
	{
        m_desired_fill += ((double)fill) / 1000.0l;
        m_fill_count++;
        return;
    }
    double buf_delta = fill - m_desired_fill;
    
	m_bf_est_err = biquad_filt(&m_bf_err_lpf, buf_delta);
    
	double err_deriv = biquad_filt(&m_bf_err_deriv_lpf, m_bf_est_err - m_bf_last_err);

	double adj_error = m_bf_est_err*CONTROL_A;

    m_bf_est_drift = biquad_filt(&m_bf_drift_lpf, CONTROL_B*(adj_error + err_deriv) + m_bf_est_drift);
    m_bf_playback_rate = 1.0l + adj_error + m_bf_est_drift;

    m_bf_last_err = m_bf_est_err;
}

bool CHairTunes::Start(std::shared_ptr<CRaopContext> pContext)
{
	ATLASSERT(pContext);
	m_pRaopContext = pContext;

	m_nControlport	= atoi(pContext->m_strCport.c_str());
	m_nTimingport	= atoi(pContext->m_strTport.c_str());

    if (!m_AES.Open(pContext->m_Rsaaeskey, pContext->m_sizeRsaaeskey, pContext->m_Aesiv, pContext->m_sizeAesiv))
        return false;

	m_mapFmtp.clear();

	char fmtpstr[512];
	strcpy_s(fmtpstr, 512, pContext->m_strFmtp.c_str());

    char* ctxTok = NULL;

	char*	arg = strtok_s(fmtpstr, " \t", &ctxTok);
	int		i	= 0;

	while (arg)
	{
		m_mapFmtp[i++] = atoi(arg);
		arg = strtok_s(NULL, " \t", &ctxTok);
	}
	ATLASSERT(m_decoder_info == NULL);

    alac_file* alac;

    m_nFrameSize	= m_mapFmtp[1]; 
    m_nSamplingRate = m_mapFmtp[11];

    int sample_size = m_mapFmtp[3];

    if (sample_size != SAMPLE_SIZE)
	{
		_LOG("only 16-bit samples supported!");
		return false;
	}
    alac = create_alac(sample_size, NUM_CHANNELS);

    if (!alac)
        return false;

    alac->setinfo_max_samples_per_frame = m_nFrameSize;
    alac->setinfo_7a					= m_mapFmtp[2];
    alac->setinfo_sample_size			= sample_size;
    alac->setinfo_rice_historymult		= m_mapFmtp[4];
    alac->setinfo_rice_initialhistory	= m_mapFmtp[5];
    alac->setinfo_rice_kmodifier		= m_mapFmtp[6];
    alac->setinfo_7f					= m_mapFmtp[7];
    alac->setinfo_80					= m_mapFmtp[8];
    alac->setinfo_82					= m_mapFmtp[9];
    alac->setinfo_86					= m_mapFmtp[10];
    alac->setinfo_8a_rate				= m_mapFmtp[11];

    m_decoder_info = alac;

    allocate_buffers(alac);	

	m_bFlush = false;
	m_bPause = false;
    m_Queue.m_hEvent.Reset();
	BF_EstReset();

	m_OutBuf.Reallocate(OUTFRAME_BYTES);

	if (!IsRunning())              
	{
		if (!CMyThread::Start(m_Queue.m_hEvent))
		{
			Stop();
			return false;
		}
	}
	if (!CRtpEndpoint::CreateEndpoint("Control", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Control))
	{
		Stop();
		return false;
	}
	m_threadResend.m_nControlport		= m_nControlport;
	m_threadResend.m_RtpClient_Control	= m_RtpClient_Control;

	if (!CRtpEndpoint::CreateEndpoint("Sender.Data", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Data))
	{
		Stop();
		return false;
	}
	if (!CRtpEndpoint::CreateEndpoint("Sender.Timing", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Timing))
	{
		Stop();
		return false;
	}
	return true;
}

USHORT CHairTunes::GetServerPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Data)
	{
		nResult = ntohs(m_RtpClient_Data->m_nPort);
	}
	return nResult;
}

USHORT CHairTunes::GetControlPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Control)
	{
		nResult = ntohs(m_RtpClient_Control->m_nPort);
	}
	return nResult;
}

USHORT CHairTunes::GetTimingPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Timing)
	{
		nResult = ntohs(m_RtpClient_Timing->m_nPort);
	}
	return nResult;
}

void CHairTunes::Stop()
{
	if (m_RtpClient_Timing)
		m_RtpClient_Timing->Cancel();
	if (m_RtpClient_Control)
		m_RtpClient_Control->Cancel();
	if (m_RtpClient_Data)
		m_RtpClient_Data->Cancel();

	m_RtpClient_Data.reset();
	m_RtpClient_Control.reset();
	m_RtpClient_Timing.reset();

	CMyThread::Stop();

	if (m_decoder_info)
	{
		destroy_alac(m_decoder_info);
		m_decoder_info = NULL;
	}
	m_Queue.clear();
}

void CHairTunes::Flush()
{
	ULONG nFlushed	= 0;
	DWORD dwWait	= WAIT_TIMEOUT;

	ATLTRACE(L"-> CHairTunes::Flush\n");

	while(!m_Queue.empty())
	{
		nFlushed += (ULONG)m_Queue.size();

		m_bFlush = true;
		m_bPause = false;
		SetEvent(m_hEvent);
		
		if (IsStopping(5))
			break;
	}
	m_bFlush = false;

	ATLTRACE(L"<- CHairTunes::Flush %lu packets\n", nFlushed);
}

void CHairTunes::SetPause(bool bValue)
{
	if (bValue != m_bPause)
	{
		m_Queue.Lock();

		m_bPause = bValue;
		
		if (m_bPause)
			ResetEvent(m_hEvent);
		else
			SetEvent(m_hEvent);

		m_Queue.Unlock();
	}
}

void CHairTunes::OnStop()
{
    m_hPcmPipe.Close();
    m_AudioPlayer.Close();
	m_threadResend.Stop();
}

bool CHairTunes::OnStart() 
{
	if (!m_threadResend.Start())
		return false;

    CHandle hReadPipe;

    if (!::CreatePipe(&hReadPipe.m_h, &m_hPcmPipe.m_h, NULL, 0))
    {
        ATLASSERT(FALSE);
        std::string strErr = CW2A(ErrorToString().c_str());

        _LOG("failed to create Pipe: %s", strErr.c_str());
        return false;
    }
    if (!m_AudioPlayer.Init(hReadPipe, CAudioPlayer::GetWaveMapperID(GetSoundId()), m_nSamplingRate, NUM_CHANNELS, SAMPLE_SIZE))
    {
        std::string strErr = CW2A(ErrorToString().c_str());

        _LOG("failed to init Audio Player: %s", strErr.c_str());
        return false;
    }
	return true;
}

void CHairTunes::QueuePacket(std::shared_ptr<CRtpPacket>& p)
{
	if (!m_bFlush && p->getDataLen() >= 16)
	{
		USHORT nCurSeq = p->getSeqNo();

		m_Queue.Lock();

		if (m_Queue.count() > 0)
		{
			std::shared_ptr<CRtpPacket>& pBack = m_Queue.back();

			// check for lacking packets
			short nSeqDiff = nCurSeq - pBack->getSeqNo();

			switch(nSeqDiff)
			{
				case 0:
				{
					ATLTRACE("packet %lu already queued\n", (long)nCurSeq);
				}
				break;

				case 1:
				{
					// expected sequence
					m_Queue.push_back(p);
					p.reset();
				}
				break;

				default:
				{
					if (nSeqDiff > 1)
					{
						if (p->getPayloadType() != PAYLOAD_TYPE_RESEND_RESPONSE)
							m_threadResend.Push(pBack->getSeqNo()+1, nSeqDiff-1);
						m_Queue.push_back(p);
						p.reset();
					}
					else
					{
						ATLASSERT(nSeqDiff < 0);

						for (auto i = m_Queue.begin(); i != m_Queue.end(); ++i)
						{
							nSeqDiff = nCurSeq - i->get()->getSeqNo();

							if (nSeqDiff == 0)
							{
								break;
							}
							if (nSeqDiff < 0)
							{
								if (i != m_Queue.begin())
								{
									m_Queue.insert(i, p);
									p.reset();
#ifdef _DEBUG
									USHORT s3 = i->get()->getSeqNo();
									USHORT s2 = (--i)->get()->getSeqNo();
									USHORT s1 = i == m_Queue.begin() ? i->get()->getSeqNo()-1 : (--i)->get()->getSeqNo();
#endif
								}
								break;
							}
							ATLASSERT(nSeqDiff > 0);
						}
					}
				}
				break;
			}
		}
		else
		{
			// expected sequence (initial packet)
			m_Queue.push_back(p);
			p.reset();
		}
		if (!m_bPause && (LONG)m_Queue.count() >= c_nStartFill)
		{
			SetEvent(m_hEvent);
		}
		m_Queue.Unlock();
	}
}

void CHairTunes::OnRequest(CRtpEndpoint* pRtpEndpoint, std::shared_ptr<CRtpPacket>& packet)
{
	byte type = packet->getPayloadType();

	switch (type)
	{
		case PAYLOAD_TYPE_RESEND_RESPONSE:
		{
			// shift data 4 bytes left
			memmove(packet->m_Data, packet->m_Data + RTP_BASE_HEADER_SIZE, packet->m_nLen - RTP_BASE_HEADER_SIZE);
			packet->m_nLen -= RTP_BASE_HEADER_SIZE;

			if (packet->getSeqNo() == 0 && packet->getDataLen() < 16)
			{
				// resync?
				Flush();
			}
			else if (m_threadResend.Pull(packet->getSeqNo()))
				QueuePacket(packet);
		}
		break;

		case PAYLOAD_TYPE_STREAM_DATA:
		{
			QueuePacket(packet);
		}
		break;

		case PAYLOAD_TYPE_TIMING_RESPONSE:
		case PAYLOAD_TYPE_TIMING_REQUEST:
		{
			ATLASSERT(packet->m_nLen == 32);
		}
		break;

		case PAYLOAD_TYPE_STREAM_SYNC:
		{
		}
		break;

		default:
		{
		}
		break;
	}
	PutPacketToPool(packet);
}

void CHairTunes::OnEvent()
{
	std::shared_ptr<CRtpPacket>	packet;
	int						nBufFill = 0;

	m_Queue.Lock();

	nBufFill = (int)m_Queue.count();

    if (nBufFill == 0)
    {
        ResetEvent(m_hEvent);
    }
	else
	{
		packet = m_Queue.front();
		m_Queue.pop_front();

		// removing later packets from resend queue
		m_threadResend.Remove(packet->getSeqNo());
	}
	m_Queue.Unlock();

	if (nBufFill == 0)
		BF_EstReset();
	else
		BF_EstUpdate(nBufFill);

	if (packet)
	{
		if (!m_bFlush)
		{
			m_mtxOnPlayingCallback.Lock();

			if (m_funcOnPlayingCallback)
			{
				m_funcOnPlayingCallback();
			}
			m_mtxOnPlayingCallback.Unlock();

			packet->m_nLen = AlacDecode(packet->m_Data, packet->getData(), packet->getDataLen());

			if (packet->m_nLen >= 4 && packet->m_nLen <= FRAME_BYTES)
			{
				if (!g_bMute)
				{
					int nPlaySamples = StuffBuffer((const short*)(BYTE*)packet->m_Data, (short*)((BYTE*)m_OutBuf), packet->m_nLen >> NUM_CHANNELS, m_bf_playback_rate);
                    
                    m_AudioPlayer.Play();

                    DWORD dwOffset = 0;
                    DWORD dwToWrite = (DWORD)(nPlaySamples << NUM_CHANNELS);

                    while (dwToWrite && !IsStopping())
                    {
                        DWORD dwWritten = 0;

                        if (!::WriteFile(m_hPcmPipe, m_OutBuf + dwOffset, dwToWrite, &dwWritten, NULL))
                            break;
                        dwToWrite -= dwWritten;
                        dwOffset += dwWritten;
                    }
				}
			}
		}
		PutPacketToPool(packet);
	}
}

bool CHairTunes::_CResendThread::_RequestResend(USHORT nSeq, short nCount)
{
	// *not* a standard RTCP NACK
	unsigned char req[8] = { 0 };						

	// Apple 'resend'
	req[0]						= 0x80;
	req[1]						= PAYLOAD_TYPE_RESEND_REQUEST|0x80;	

	// our seqnum
	*(unsigned short *)(req+2)	= htons(1);		
	// missed seqnum
	*(unsigned short *)(req+4)	= htons(nSeq); 
	// count
	*(unsigned short *)(req+6)	= htons(nCount);  

	if (!m_RtpClient_Control->SendTo(req, sizeof(req), m_nControlport))
	{
		return false;
	}
	return true;
}

void CHairTunes::biquad_init(biquad_t* bq, double a[], double b[])
{
	bq->hist[0] = bq->hist[1] = 0.0l;

	memcpy(bq->a, a, 2 * sizeof(double));
	memcpy(bq->b, b, 3 * sizeof(double));
}

void CHairTunes::biquad_lpf(biquad_t* bq, double freq, double Q, double sampling_rate, double frame_size)
{
	double w0 = 2.0l * M_PI * freq * frame_size / sampling_rate;
	double alpha = sin(w0) / (2.0l * Q);
	double a_0 = 1.0l + alpha;

	double b[3];
	double a[2];

	b[0] = (1.0l - cos(w0)) / (2.0l * a_0);
	b[1] = (1.0l - cos(w0)) / a_0;
	b[2] = b[0];
	a[0] = -2.0l * cos(w0) / a_0;
	a[1] = (1 - alpha) / a_0;

	biquad_init(bq, a, b);
}

double CHairTunes::biquad_filt(biquad_t* bq, double in)
{
	double	w = in - bq->a[0] * bq->hist[0] - bq->a[1] * bq->hist[1];
	double	out = bq->b[1] * bq->hist[0] + bq->b[2] * bq->hist[1] + bq->b[0] * w;

	bq->hist[1] = bq->hist[0];
	bq->hist[0] = w;

	return out;
}

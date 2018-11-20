/*
 *
 *  Utils.h
 *
 */

#pragma once

#include "stdint_win.h"
#include "Networking.h"

#define RTP_BASE_HEADER_SIZE			0x04
#define RTP_DATA_HEADER_SIZE			0x08
#define RTP_DATA_OFFSET					0x0C // RTP_BASE_HEADER_SIZE + RTP_DATA_HEADER_SIZE

#define EXTENSION_MASK					0x10
#define MARKER_MASK						0x80
#define PAYLOAD_MASK					0x7F

#define PAYLOAD_TYPE_STREAM_DATA		0x60
#define PAYLOAD_TYPE_STREAM_SYNC		0x54
#define PAYLOAD_TYPE_TIMING_REQUEST		0x52
#define PAYLOAD_TYPE_TIMING_RESPONSE	0x53
#define PAYLOAD_TYPE_RESEND_REQUEST		0x55
#define PAYLOAD_TYPE_RESEND_RESPONSE	0x56

// maximum number of bytes in an RAOP audio data packet including headers
#define RAOP_PACKET_MAX_SIZE	2048


class CRtpPacket
{
public:
	static const char* GetPayloadName(byte nType)
	{
		switch(nType)
		{
			case PAYLOAD_TYPE_STREAM_DATA:
			{
				return "PAYLOAD_TYPE_STREAM_DATA";
			}
			case PAYLOAD_TYPE_STREAM_SYNC:
			{
				return "PAYLOAD_TYPE_STREAM_SYNC";
			}
			case PAYLOAD_TYPE_TIMING_REQUEST:
			{
				return "PAYLOAD_TYPE_TIMING_REQUEST";
			}
			case PAYLOAD_TYPE_TIMING_RESPONSE:
			{
				return "PAYLOAD_TYPE_TIMING_RESPONSE";
			}
			case PAYLOAD_TYPE_RESEND_REQUEST:
			{
				return "PAYLOAD_TYPE_RESEND_REQUEST";
			}
			case PAYLOAD_TYPE_RESEND_RESPONSE:
			{
				return "PAYLOAD_TYPE_RESEND_RESPONSE";
			}
		}
		static char dummy[16];
		sprintf_s(dummy, 16, "%lx", (ULONG)nType);
		return dummy;
	}

	CRtpPacket(const BYTE* p = NULL, int nLen = 0) : m_Data(RAOP_PACKET_MAX_SIZE)
	{
		m_nLen = nLen;

		if (m_nLen > 0 && p)
		{
			if (m_nLen > RAOP_PACKET_MAX_SIZE)
				m_Data.Reallocate(m_nLen);
			memcpy(m_Data, p, m_nLen);
		}
	}
	inline void Init()
	{
		m_Data[0]	= 0x80;
		m_Data[1]	= 0x00;
		m_Data[2]	= 0x00;
		m_Data[3]	= 0x00;
		m_Data[1]	= 0x00;
		m_Data[2]	= 0x00;
		m_Data[3]	= 0x00;
		m_Data[4]	= 0x00;
		m_Data[5]	= 0x00;
		m_Data[6]	= 0x00;
		m_Data[7]	= 0x00;
	}
	inline bool getExtension() const
	{
		return ((m_Data[0] & EXTENSION_MASK) != 0);
	}
	inline void setExtension(const bool value = true)
	{
		m_Data[0] = (value ? (m_Data[0] | EXTENSION_MASK) : (m_Data[0] & ~EXTENSION_MASK));
	}
	inline bool getMarker() const
	{
		return ((m_Data[1] & MARKER_MASK) != 0);
	}
	inline void setMarker(const bool value = true)
	{
		m_Data[1] = (value ? (m_Data[1] | MARKER_MASK) : (m_Data[1] & ~MARKER_MASK));
	}
	inline byte getPayloadType() const
	{
		return (m_Data[1] & PAYLOAD_MASK);
	}
	inline void setPayloadType(const byte value)
	{
		m_Data[1] = (m_Data[1] & ~PAYLOAD_MASK) | (value & PAYLOAD_MASK);
	}
	inline USHORT getSeqNo()
	{
		return ntohs(*(unsigned short *)(&m_Data[2]));
	}
	inline void setSeqNo(USHORT n)
	{
		*(unsigned short *)(&m_Data[2]) = htons(n);
	}
	inline ULONG getTimeStamp()
	{
		return ntohl(*(unsigned long *)(&m_Data[4]));
	}
	inline unsigned char* getData()
	{
		return m_Data + RTP_DATA_OFFSET;
	}
	inline int getDataLen() const
	{
		return m_nLen - RTP_DATA_OFFSET;
	}
	inline uint32_t getSSRC() const
	{
		return ntohl(*(uint32_t *)&m_Data[8]);
	}
	inline ULONGLONG getNTPTimeStamp(int nOffset) const
	{
		return MAKEULONGLONG(	MAKELONG(MAKEWORD(m_Data[nOffset+7], m_Data[nOffset+6]), MAKEWORD(m_Data[nOffset+5], m_Data[nOffset+4]))
							,	MAKELONG(MAKEWORD(m_Data[nOffset+3], m_Data[nOffset+2]), MAKEWORD(m_Data[nOffset+1], m_Data[nOffset]))
							);
	}
	inline ULONGLONG getReferenceTime() const
	{
		return getNTPTimeStamp(8);
	}
	inline ULONGLONG getReceivedTime() const
	{
		return getNTPTimeStamp(16);
	}
	inline ULONGLONG getSendTime() const
	{
		return getNTPTimeStamp(24);
	}
	inline void setNTPTimeStamp(int nOffset, ULONGLONG tVal) 
	{
		DWORD lDWord = LODWORD(tVal);
		DWORD hDWord = HIDWORD(tVal);

		WORD llWord = LOWORD(lDWord);
		WORD lhWord = HIWORD(lDWord);

		WORD hlWord = LOWORD(hDWord);
		WORD hhWord = HIWORD(hDWord);

		m_Data[nOffset]		= HIBYTE(hhWord);
		m_Data[nOffset+ 1]	= LOBYTE(hhWord);
		m_Data[nOffset+ 2]	= HIBYTE(hlWord);
		m_Data[nOffset+ 3]	= LOBYTE(hlWord);
		m_Data[nOffset+ 4]	= HIBYTE(lhWord);
		m_Data[nOffset+ 5]	= LOBYTE(lhWord);
		m_Data[nOffset+ 6]	= HIBYTE(llWord);
		m_Data[nOffset+ 7]	= LOBYTE(llWord);
	}
	inline void setReferenceTime(ULONGLONG tVal)
	{
		setNTPTimeStamp(8, tVal);
	}
	inline void setReceivedTime(ULONGLONG tVal)
	{
		setNTPTimeStamp(16, tVal);
	}
	inline void setSendTime(ULONGLONG tVal)
	{
		setNTPTimeStamp(24, tVal);
	}
	inline USHORT getMissedSeqNr() const
	{
		return ntohs(*(unsigned short*)(&m_Data[4]));
	}
	inline USHORT getMissedCount() const
	{
		return ntohs(*(unsigned short*)(&m_Data[6]));
	}
	inline void setTimeLessLatency(uint32_t nVal)
	{
		*(uint32_t *)(&m_Data[4]) = htonl(nVal);
	}
	inline void setRtpSync(uint32_t nVal)
	{
		*(uint32_t *)(&m_Data[16]) = htonl(nVal);
	}
	inline void setRtpData(uint32_t nVal)
	{
		*(uint32_t *)(&m_Data[4]) = htonl(nVal);
	}
	inline void setSSRC(uint32_t nVal)
	{
		*(uint32_t *)(&m_Data[8]) = htonl(nVal);
	}
public:
	CTempBuffer<BYTE, RAOP_PACKET_MAX_SIZE>	m_Data;
	int										m_nLen;
};

class CRtpEndpoint;

class IRtpRequestHandler
{
	friend class CRtpEndpoint;

protected:
	virtual void OnRequest(CRtpEndpoint* pClient, std::shared_ptr<CRtpPacket>& packet) = 0;

protected:
	inline void PutPacketToPool(std::shared_ptr<CRtpPacket>& p)
	{
		if (p)
		{
			m_mtxPacketPool.Lock(true);
			m_listPacketPool.push_back(p);
			m_mtxPacketPool.Unlock();

			p.reset();
		}
	}
	inline std::shared_ptr<CRtpPacket> GetNewPacketFromPool()
	{
		std::shared_ptr<CRtpPacket> pResult;

		m_mtxPacketPool.Lock(true);

		if (!m_listPacketPool.empty())
		{
			pResult = m_listPacketPool.front();
			m_listPacketPool.pop_front();
			m_mtxPacketPool.Unlock();
		}
		else
		{
			m_mtxPacketPool.Unlock();
			pResult = std::make_shared<CRtpPacket>();
		}
		if (pResult)
			pResult->Init();

		return pResult;
	}

protected:
	CMyMutex									m_mtxPacketPool;
	std::list< std::shared_ptr<CRtpPacket> >	m_listPacketPool;
};

class CRtpEndpoint : public CNetworkServer
{
public:
	CRtpEndpoint(const char* strName, int nType = SOCK_DGRAM)
	{
		m_pRequestHandler	= NULL;
		m_sa_len			= 0;
		m_nType				= nType;
		m_nSeqGen			= (USHORT)CreateRand(0, 0xffff);
		m_strReceiveFromName= std::string("RtpRequest.From.") + std::string(strName);
		m_strSendToName		= std::string("RtpSend.To.") + std::string(strName);

		memset(&m_saPeer, 0, sizeof(m_saPeer));
	}
	BOOL Run(IRtpRequestHandler* pRequestHandler, const PSOCKADDR_IN psaPeer, USHORT nPort_NetOrder)
	{
		ATLASSERT(pRequestHandler);
		ATLASSERT(psaPeer);
			
		m_pRequestHandler	= pRequestHandler;

		if (psaPeer->sin_family == AF_INET6)
			m_sa_len = sizeof(SOCKADDR_IN6);
		else
			m_sa_len = sizeof(SOCKADDR_IN);

		memcpy(&m_saPeer, psaPeer, m_sa_len);

		SOCKADDR_IN6 in;
		memset(&in, 0, sizeof(in));

		if (m_saPeer.sin6_family == AF_INET6)
		{
			in.sin6_family		= AF_INET6;
			in.sin6_port		= nPort_NetOrder;
			in.sin6_addr		= in6addr_any;
		}
		else
		{

			SOCKADDR_IN* pIN4 = (SOCKADDR_IN*)&in;

			pIN4->sin_family			= AF_INET;
			pIN4->sin_port				= nPort_NetOrder;
			pIN4->sin_addr.S_un.S_addr	= INADDR_ANY;
		}
		return CNetworkServer::Run((const struct sockaddr*)&in, m_sa_len, m_nType);
	}
	void OnRequest(SOCKET sd)
	{
		CSocketBase sock;

		sock.Attach(sd);

		std::shared_ptr<CRtpPacket> p = m_pRequestHandler->GetNewPacketFromPool();

		if (p)
		{
			if (m_nType == SOCK_DGRAM)
				p->m_nLen = sock.recvfrom(p->m_Data, RAOP_PACKET_MAX_SIZE);

			if (p->m_nLen > 0)
			{
				m_pRequestHandler->OnRequest(this, p);
			}
			else
				m_pRequestHandler->PutPacketToPool(p);
		}
		sock.Detach();
	}
	inline BOOL SendTo(const void* pBuf, int nLen, USHORT nPortHostOrder)
	{
		m_mtx_sa.Lock();
		m_saPeer.sin6_port = htons(nPortHostOrder);
		BOOL bResult = CNetworkServer::SendTo(pBuf, nLen, (const sockaddr*)&m_saPeer, m_sa_len);
		m_mtx_sa.Unlock();

		return bResult;
	}
	static bool CreateEndpoint(const char* strName, IRtpRequestHandler* pRequestHandler, const PSOCKADDR_IN psaPeer, std::shared_ptr<CRtpEndpoint>& udp_endpoint, std::shared_ptr<CRtpEndpoint>* tcp_endpoint = NULL)
	{
		if (udp_endpoint)
			udp_endpoint.reset();

		udp_endpoint = std::make_shared<CRtpEndpoint>(strName);

		int nTry = 12;

		do
		{
			USHORT nPort = GetUniquePortNumber();

			if (udp_endpoint->Run(pRequestHandler, psaPeer, htons(nPort)))
			{
				if (tcp_endpoint)
				{
					std::shared_ptr<CRtpEndpoint>& _tcp_endpoint = *tcp_endpoint;

					_tcp_endpoint = std::make_shared<CRtpEndpoint>(strName, SOCK_STREAM);

					if (_tcp_endpoint->Run(pRequestHandler, psaPeer, htons(nPort)))
						return true;
					udp_endpoint->Cancel();
					_tcp_endpoint.reset();
				}
				else
					return true;
			}
		}
		while(nTry--);

		return false;
	}
protected:
	IRtpRequestHandler*		m_pRequestHandler;
	SOCKADDR_IN6			m_saPeer;
	ULONG					m_sa_len;
	CMyMutex				m_mtx_sa;
	int						m_nType;
	std::string				m_strReceiveFromName;
	std::string				m_strSendToName;
public:
	USHORT					m_nSeqGen;
};
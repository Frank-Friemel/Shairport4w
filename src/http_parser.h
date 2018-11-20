/*
 *
 *  http_parser.h
 *
 */

#pragma once

class CHttp
{
public:
	CHttp()
	{
		m_nStatus = 0;
	}
	~CHttp()
	{
	}
	bool Parse(bool bRequest = true)
	{
		if (m_strBuf.length() == 0)
			return false;

		ULONG	nBuf	= (ULONG)(m_strBuf.length() + 1);
		char*	pBuf	= new char[nBuf];
		
		if (pBuf)
		{
			std::map<long, std::string> mapByLine;

			std::string::size_type nSep = m_strBuf.find("\r\n\r\n");

			if (nSep == std::string::npos)
			{
				strcpy_s(pBuf, nBuf, m_strBuf.c_str());
				m_strBody.clear();
			}
			else
			{
				strcpy_s(pBuf, nBuf, m_strBuf.substr(0, nSep).c_str());
				m_strBody = m_strBuf.substr(nSep+4);
			}

			char* ctx	= NULL;
			char* p		= strtok_s(pBuf, "\r\n", &ctx);

			long n = 0;

			while (p)
			{
				if (strlen(p) > 0)
					mapByLine[n++] = p;

				p = strtok_s(NULL, "\r\n", &ctx);
			}
			for (auto i = mapByLine.begin(); i != mapByLine.end(); ++i)
			{
				if (i == mapByLine.begin())
				{
					strcpy_s(pBuf, nBuf, i->second.c_str());

					ctx	= NULL;
					p	= strtok_s(pBuf, " \t", &ctx);

					if (p)
					{
						if (bRequest)
							m_strMethod = p;
						else
							m_strProtocol = p;
	
						p = strtok_s(NULL, " \t", &ctx);

						if (p)
						{
							if (bRequest)
								m_strUrl = p;
							else
								m_nStatus = atol(p);

							p = strtok_s(NULL, " \t", &ctx);

							if (p)
							{
								if (bRequest)
									m_strProtocol = p;
								else
									m_strStatus = p;
							}
						}
					}
					else
					{
						// improve parser!
						ATLASSERT(FALSE);
					}
				}
				else
				{
					std::string::size_type n = i->second.find(':');

					if (n != std::string::npos)
					{
						ic_string	strTok		= i->second.substr(0, n).c_str();
						std::string		strValue	= i->second.substr(n+1);

						Trim(strTok);
						Trim(strValue);

						m_mapHeader[strTok] = strValue;
					}
				}
			}
			delete pBuf;
			return true;
		}
		return false;
	}
	void Create(PCSTR strProtocol = "HTTP/1.1", long nStatus = 200, const char* strStatus = "OK")
	{
		m_nStatus		= nStatus;
		m_strProtocol	= strProtocol;
		m_strStatus		= strStatus ? strStatus : "OK";
	}
	void SetStatus(long nStatus, const char* strStatus)
	{
		m_nStatus		= nStatus;
		m_strStatus		= strStatus;
	}
	std::string GetAsString(bool bResponse = true)
	{
		std::string strResult;

		char buf[1024];

		if (bResponse)
		{
			sprintf_s(buf, 256, "%s %ld %s\r\n", m_strProtocol.c_str(), m_nStatus, m_strStatus.c_str());
			strResult += buf;
		}
		else
		{
			sprintf_s(buf, 256, "%s %s %s\r\n", m_strMethod.c_str(), m_strUrl.c_str(), m_strProtocol.c_str());
			strResult += buf;

			if (m_strBody.length() > 0)
			{
				if (_itoa_s((int)m_strBody.length(), buf, _countof(buf), 10) == ERROR_SUCCESS)
					m_mapHeader["content-length"] = buf;
			}
		}
		for (auto i = m_mapHeader.begin(); i != m_mapHeader.end(); ++i)
		{
			strResult += i->first.c_str();
			strResult += ": ";
			strResult += i->second;
			strResult += "\r\n";
		}
		if (!bResponse)
		{
			strResult += "\r\n";
			strResult += m_strBody;
		}
		return strResult;
	}
	void InitNew()
	{
		m_mapHeader.clear();
		m_strMethod.clear();
		m_strUrl.clear();
		m_strProtocol.clear();
		m_strBody.clear();
		m_nStatus= 0;
		m_strStatus.clear();
		m_strBuf.clear();
	}
public:
	std::map<ic_string, std::string>	m_mapHeader;
	std::string					        m_strMethod;
	ic_string				            m_strUrl;
	ic_string				            m_strProtocol;
	std::string					        m_strBody;
	long					            m_nStatus;
	std::string					        m_strStatus;
	std::string					        m_strBuf;
};

/*
 *
 *  Shairport4w.cpp
 *
 */

#include "stdafx.h"

#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "Bonjour/dns_sd.h"

#include "DmapParser.h"

typedef EXECUTION_STATE (WINAPI *_SetThreadExecutionState)(EXECUTION_STATE esFlags);
_SetThreadExecutionState	pSetThreadExecutionState = NULL;

static	std::map<SOCKET, std::shared_ptr<CRaopContext> >	c_mapConnection;
static 	CMyMutex								            c_mtxConnection;
static	HHOOK									            g_hDesktopHook		= NULL;

CAppModule		_Module;
CMainDlg		dlgMain;
BYTE			hw_addr[6];
CMyMutex		mtxRSA;

std::shared_ptr<my_crypt::Rsa> Rsa;

std::string			strPrivateKey   =   "UlNBMwAIAAADAAAAAAEAAIAAAACAAAAAAQAB59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U"
                                    "3GhC/j0Qg90u3sG/1CUtwC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ"
                                    "2YCCLlDRKSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/"
                                    "bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD"
                                    "6z5vYTcrtij2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEe/fg"
                                    "v1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFua39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFv"
                                    "lYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndMoPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDT"
                                    "CRiIPMQ++N2iLDaR72//+ZTx5WRBqgA1/RmgyNbwI3jHBYDZxIQgeR30B8WR+26/yjIsMIbdkB/S"
                                    "+uGuu2St9rt5/4BRvr0M2CCriYdABgGnsv6TkMrMmsq47Sv5HRhtj2lkPX7+D11W33V3otA16lQT"
                                    "/JjY8/kI2gWaN52kscw48V1WCoPMMXFTyEvQ66+8QCW6gYx1cCM0OE6PaW+ATXqg53ZOUHu309/v"
                                    "x9Z4xmgtP61xNEG+6uckoJ7Am9w7wHCckTPUiezipRrdBTEnSQ+ShtFzyKQFTcIKV1x+TAyYNPSh"
                                    "3odJF6PkAOr4hQYttct+NDaJ5xH3X+eD1+GRkv12nNVCvqS5AQfs0X9AGNx96iktpTBCOG8xBaB3"
                                    "itxvPeaQ2it0xQVZg+31dGYaL9e33oBTzMDiCPDIrGJvWX09mdLOUaN7Oa5Lfp7ywHXwvz2Dys0y"
                                    "2paRksKJkjWCXAfRzTJZoZBs3NSZy2E+IslMseqXGQZgnfGw9IsGPxc3IDQ2lJm1/flw70QNkE7p"
                                    "IPlE71qvfJQgoA9em0gILAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4I"
                                    "SgxB2bB3HcYzQMGsz1qJ2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjo"
                                    "D2QYjhBGuhvkWKbl8Axy9XfWBLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507Hv"
                                    "hF5N3drJoVU3O14nDY4TFQAaLlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2n"
                                    "IPscEsA5ltpxOgUGCY7b7ez5NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uF"
                                    "QEG4Z3BrWP7yoNuSK3dii2jmlpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5"
                                    "k0o9rKQLtvLzfAqdBxBurcizaaA/L0HIgAmOit1GJA2saMxTVPNh";

static bool digest_ok(std::shared_ptr<CRaopContext>& pRaopContext)
{
	if (!pRaopContext->m_strNonce.empty())
	{
		std::string& strAuth = pRaopContext->m_CurrentHttpRequest.m_mapHeader["Authorization"];

		if (strAuth.find("Digest ") == 0)
		{
            std::map<ic_string, std::string> mapAuth;

            ParseRegEx(strAuth.c_str() + 7, "[^\\s=]+[\\s]*=[\\s]*\"[^\"]+\""
                , [&mapAuth](std::string s) -> bool
            {
                std::string strLeft;
                std::string strRight;

                if (!ParseRegEx(s, "[^\\s\"=]+", [&strLeft](std::string t) -> bool
                {
                    strLeft = t;
                    return false;
                }))
                {
                    return false;
                }
                if (!ParseRegEx(s, "\"[^\"]+\"", [&strRight](std::string t) -> bool
                {
                    Trim(t, "\"");
                    strRight = t;
                    return false;
                }))
                {
                    return false;
                }
                mapAuth[strLeft.c_str()] = strRight;
                return true;
            });
            std::string str1 = mapAuth["username"] + std::string(":") + mapAuth["realm"] + std::string(":") + std::string(CT2CA(dlgMain.m_strPassword));
            std::string str2 = pRaopContext->m_CurrentHttpRequest.m_strMethod + std::string(":") + mapAuth["uri"];

            std::string strDigest1 = MD5_Hex((const BYTE*)str1.c_str(), (long)str1.length(), true);
            std::string strDigest2 = MD5_Hex((const BYTE*)str2.c_str(), (long)str2.length(), true);

            std::string strDigest = strDigest1 + std::string(":") + mapAuth["nonce"] + std::string(":") + strDigest2;

			strDigest = MD5_Hex((const BYTE*)strDigest.c_str(), (long)strDigest.length(), true);

			_LOG("Password Digest: \"%s\" == \"%s\"\n", strDigest.c_str(), mapAuth["response"].c_str());

			return strDigest == mapAuth["response"];
		}
	}
	return false;
}

class CRaopServer : public CNetworkServer, protected CDmapParser
{
protected:
	CRaopServer(const std::shared_ptr<CHairTunes::Volume>& volume)
		: m_Volume(volume)
	{
		ATLASSERT(m_Volume);
	}

	// Dmap Parser Callbacks
	virtual void on_string(void* ctx, const char *code, const char *name, const char *buf, size_t len)  
	{
		CRaopContext* pRaopContext = (CRaopContext*)ctx;
		ATLASSERT(pRaopContext);
		
		if (_stricmp(name, "daap.songalbum") == 0)
		{
			ATL::CString strValue;

			LPTSTR p = strValue.GetBuffer((int)(len*2));

			if (p)
			{
				int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, (int)len, p, (int)(len*2));
				strValue.ReleaseBuffer((n >= 0) ? n : -1);
			}
			pRaopContext->PutSongalbum(strValue);
		}
		else if (_stricmp(name, "daap.songartist") == 0)
		{
			ATL::CString strValue;

			LPTSTR p = strValue.GetBuffer((int)(len*2));

			if (p)
			{
				int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, (int)len, p, (int)(len*2));
				strValue.ReleaseBuffer((n >= 0) ? n : -1);
			}
			pRaopContext->PutSongartist(strValue);
		}
		else if (_stricmp(name, "dmap.itemname") == 0)
		{
			ATL::CString strValue;

			LPTSTR p = strValue.GetBuffer((int)(len*2));

			if (p)
			{
				int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, (int)len, p, (int)(len*2));
				strValue.ReleaseBuffer((n >= 0) ? n : -1);
			}
			pRaopContext->PutSongtrack(strValue);
		}
	}
	// Network Callbacks
	virtual bool OnAccept(SOCKET sd)
	{
		std::string strPeerIP;
		std::string strLocalIP;

		GetLocalIP(sd, m_bV4, strLocalIP);
		GetPeerIP(sd, m_bV4, strPeerIP);
		
		_LOG("Accepted new client %s\n", strPeerIP.c_str());

		std::shared_ptr<CRaopContext> pRaopContext = std::make_shared<CRaopContext>(m_bV4, strPeerIP.c_str(), strLocalIP.c_str());

		if (pRaopContext)
		{
			pRaopContext->m_pDecoder = std::make_shared<CHairTunes>(m_Volume);
			ATLASSERT(pRaopContext->m_pDecoder);			
			
			c_mtxConnection.Lock();

			c_mapConnection.erase(sd);

			c_mapConnection[sd] = pRaopContext;

			GetPeerIP(sd, m_bV4, (struct sockaddr_in*)&c_mapConnection[sd]->m_Peer);

			c_mtxConnection.Unlock();

			return true;
		}
		return false;
	}
	virtual void OnRequest(SOCKET sd)
	{
		CTempBuffer<char> buf(0x100000);

		memset(buf, 0, 0x100000);

		CSocketBase sock;

		sock.Attach(sd);

		int nReceived = sock.recv(buf, 0x100000);

		if (nReceived < 0)
		{
			sock.Detach();
			return;
		}
		ATLASSERT(nReceived > 0);

		c_mtxConnection.Lock();

		auto pRaopContext = c_mapConnection[sd];

		c_mtxConnection.Unlock();

		CHttp&	request	= pRaopContext->m_CurrentHttpRequest;

		request.m_strBuf += std::string(buf, nReceived);

		if (request.Parse())
		{
			if (request.m_mapHeader.find("content-length") != request.m_mapHeader.end())
			{
				if (atol(request.m_mapHeader["content-length"].c_str()) > (long)request.m_strBody.length())
				{
					// need more
					sock.Detach();
					return;
				}
			}
			//if (_stricmp(request.m_strMethod.c_str(), "OPTIONS") != 0)
			{
				_LOG("+++++++ Http Request +++++++\n%s------- Http Request -------\n", request.m_strBuf.c_str());
			}
			CHttp response;

			response.Create(request.m_strProtocol.c_str());

			response.m_mapHeader["CSeq"]				= request.m_mapHeader["CSeq"];
			response.m_mapHeader["Audio-Jack-Status"]	= "connected; type=analog";
			response.m_mapHeader["Server"]				= "AirTunes/105.1";

			if (request.m_mapHeader.find("Apple-Challenge") != request.m_mapHeader.end())
			{
				std::string strChallenge = request.m_mapHeader["Apple-Challenge"];
				
				CTempBuffer<BYTE>	pChallenge;
				ULONG				sizeChallenge = my_Base64::base64_decode(strChallenge, &pChallenge);

				if (sizeChallenge)
				{
					CTempBuffer<BYTE>	pIP;
					int					sizeIP = GetLocalIP(sd, m_bV4, pIP);

					BYTE  pAppleResponse[64];
					ULONG sizeAppleResponse = 0;

					memset(pAppleResponse, 0, sizeof(pAppleResponse));

					memcpy(pAppleResponse+sizeAppleResponse, pChallenge, sizeChallenge);
					sizeAppleResponse += sizeChallenge;
					ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));
					memcpy(pAppleResponse+sizeAppleResponse, pIP, sizeIP);
					sizeAppleResponse += sizeIP;
					ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));
					memcpy(pAppleResponse+sizeAppleResponse, hw_addr, sizeof(hw_addr));
					sizeAppleResponse += sizeof(hw_addr);
					ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));

					sizeAppleResponse = max(32, sizeAppleResponse);

                    CTempBuffer<BYTE> pSignature;
                    ULONG sizeSignature = 0;

					mtxRSA.Lock();

                    if (!Rsa->Sign(NULL, pAppleResponse, sizeAppleResponse, pSignature, sizeSignature))
                        sizeSignature = 0;

                    mtxRSA.Unlock();

					ATLASSERT(sizeSignature > 0);

					std::string strAppleResponse = my_Base64::base64_encode(pSignature, sizeSignature);

					TrimRight(strAppleResponse, "=\r\n");

					response.m_mapHeader["Apple-Response"] = strAppleResponse;
				}
			}
			if (dlgMain.m_strPassword.GetLength() > 0)
			{
				if (!digest_ok(pRaopContext))
				{
					BYTE nonce[20];

					for (long i = 0; i < 20; ++i)
					{
						nonce[i] = (256 * rand()) / RAND_MAX;
					}
					pRaopContext->m_strNonce = MD5_Hex(nonce, 20, true);

					response.m_mapHeader["WWW-Authenticate"] =		std::string("Digest realm=\"") 
																+	std::string(CT2CA(dlgMain.m_strApName))
																+	std::string("\", nonce=\"")
																+	pRaopContext->m_strNonce
																+	std::string("\"");
					response.SetStatus(401, "Unauthorized");
					request.m_strMethod = "DENIED";
				}
		    }
			if (_stricmp(request.m_strMethod.c_str(), "OPTIONS") == 0)
			{
				response.m_mapHeader["Public"] = "ANNOUNCE, SETUP, RECORD, PAUSE, FLUSH, TEARDOWN, OPTIONS, GET_PARAMETER, SET_PARAMETER, POST, GET";
			}
			else if (_stricmp(request.m_strMethod.c_str(), "ANNOUNCE") == 0)
			{
				std::map<ic_string, std::string> mapKeyValue;

                ParseRegEx(request.m_strBody, "a=[^\\s:]+[\\s]*:[^\r\n]+[\r\n]+"
                    , [&mapKeyValue](std::string s) -> bool
                {
                    std::string strLeft;
                    std::string strRight;

                    if (!ParseRegEx(s.c_str() + 2, "[^\\s:]+", [&strLeft](std::string t) -> bool
                    {
                        strLeft = t;
                        return false;
                    }))
                    {
                        return false;
                    }
                    if (!ParseRegEx(s.c_str() + 2, ":[^\r\n]+", [&strRight](std::string t) -> bool
                    {
                        Trim(t, " \t\r\n:");
                        strRight = t;
                        return false;
                    }))
                    {
                        return false;
                    }
                    mapKeyValue[strLeft.c_str()] = strRight;
                    return true;
                });
				pRaopContext->Lock();

				pRaopContext->Announce();

				pRaopContext->m_strFmtp			= mapKeyValue["fmtp"];
				pRaopContext->m_sizeAesiv		= my_Base64::base64_decode(mapKeyValue["aesiv"], &pRaopContext->m_Aesiv);
				pRaopContext->m_sizeRsaaeskey	= my_Base64::base64_decode(mapKeyValue["rsaaeskey"], &pRaopContext->m_Rsaaeskey);

				pRaopContext->m_strUserAgent	= CA2W(request.m_mapHeader["User-Agent"].c_str(), CP_UTF8);
				int nPar = pRaopContext->m_strUserAgent.Find(L'(');

				if (nPar > 0)
					pRaopContext->m_strUserAgent = pRaopContext->m_strUserAgent.Left(nPar);
				pRaopContext->m_strUserAgent.TrimRight(L"\r\n \t");

                ParseRegEx(pRaopContext->m_strFmtp, "[\\d]+", [&](std::string s)
                {
                    pRaopContext->m_listFmtp.push_back(s);
                    return true;
                });

				if (pRaopContext->m_listFmtp.size() >= 12)
				{
					auto iFmtp = pRaopContext->m_listFmtp.begin();
					advance(iFmtp, 11);

					pRaopContext->m_lfFreq = atof(iFmtp->c_str());
				}
				pRaopContext->Unlock();

				mtxRSA.Lock();

                CTempBuffer<BYTE> pInData;
                ATLVERIFY(pInData.Allocate(pRaopContext->m_sizeRsaaeskey));
                memcpy(pInData, pRaopContext->m_Rsaaeskey, pRaopContext->m_sizeRsaaeskey);

                if (!Rsa->Crypt(my_crypt::Rsa::decrypt_oaep, pInData, pRaopContext->m_sizeRsaaeskey, pRaopContext->m_Rsaaeskey, pRaopContext->m_sizeRsaaeskey, BCRYPT_SHA1_ALGORITHM))
                    pRaopContext->m_sizeRsaaeskey = 0;

                mtxRSA.Unlock();
			}
			else if (_stricmp(request.m_strMethod.c_str(), "SETUP") == 0)
			{
				std::string strTransport = request.m_mapHeader["transport"];

                ParseRegEx(strTransport, "control_port=[\\d]+", [&](std::string s) -> bool
                {
                    pRaopContext->m_strCport = s.c_str() + strlen("control_port=");
                    return false;
                });
                ParseRegEx(strTransport, "timing_port=[\\d]+", [&](std::string s) -> bool
                {
                    pRaopContext->m_strTport = s.c_str() + strlen("timing_port=");
                    return false;
                });

				response.m_mapHeader["Session"] = "DEADBEEF";

				// kick currently playing stream
				if (dlgMain.GetMMState() != CMainDlg::pause)
				{
					// give playing client a chance to disconnect itself
					if (dlgMain.OnPause())
					{
						long nTry = 12;

						do
						{ 
							Sleep(12);
						}
						while(dlgMain.GetMMState() != CMainDlg::pause && --nTry > 0);
					}
				}
				c_mtxConnection.Lock();

				for (auto i = c_mapConnection.begin(); i != c_mapConnection.end(); ++i)
				{
					if (sd != i->first && i->second->m_bIsStreaming)
					{
						i->second->m_pDecoder->Stop();

						if (pSetThreadExecutionState)
							pSetThreadExecutionState(ES_CONTINUOUS);
						i->second->m_bIsStreaming = false;
						break;
					}
				}
				pRaopContext->m_bIsStreaming = true;

				if (request.m_mapHeader.find("DACP-ID") != request.m_mapHeader.end())
				{
					dlgMain.SetDacpID(ic_string("iTunes_Ctrl_") + ic_string(request.m_mapHeader["DACP-ID"].c_str()), request.m_mapHeader["Active-Remote"].c_str());
				}
				dlgMain.SendMessage(WM_RAOP_CTX, 0, (LPARAM)&pRaopContext);

				if (pRaopContext->m_pDecoder->Start(pRaopContext))
				{
					if (pSetThreadExecutionState)
						pSetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

					char strTransport[256];

					long nServerPort	= pRaopContext->m_pDecoder->GetServerPort();
					long nControlPort	= pRaopContext->m_pDecoder->GetControlPort();
					long nTimingPort	= pRaopContext->m_pDecoder->GetTimingPort();

					sprintf_s(strTransport, 256, "RTP/AVP/UDP;unicast;mode=record;server_port=%ld;control_port=%ld;timing_port=%ld", nServerPort, nControlPort, nTimingPort);

					response.m_mapHeader["Transport"] = strTransport;
				}
				else
				{
					pRaopContext->m_bIsStreaming = false;
					response.SetStatus(503, "Service Unavailable");
				}
				c_mtxConnection.Unlock();
			}
			else if (_stricmp(request.m_strMethod.c_str(), "FLUSH") == 0)
			{
				response.m_mapHeader["RTP-Info"] = "rtptime=0";

				if (pRaopContext->m_bIsStreaming)
					pRaopContext->m_pDecoder->Flush();

				if (dlgMain.m_hWnd && dlgMain.IsCurrentContext(pRaopContext))
					dlgMain.PostMessage(WM_PAUSE, 1);				
			}
			else if (_stricmp(request.m_strMethod.c_str(), "TEARDOWN") == 0)
			{
				if (dlgMain.m_hWnd && dlgMain.IsCurrentContext(pRaopContext))
					dlgMain.PostMessage(WM_STOP);

				if (pRaopContext->m_bIsStreaming)
				{
					pRaopContext->m_pDecoder->Stop();
					pRaopContext->m_bIsStreaming = false;

					if (pSetThreadExecutionState)
						pSetThreadExecutionState(ES_CONTINUOUS);
				}
				response.m_mapHeader["Connection"] = "close";
			}
			else if (_stricmp(request.m_strMethod.c_str(), "RECORD") == 0)
			{
				dlgMain.PostMessage(WM_RESUME);
				response.m_mapHeader["Audio-Latency"] = "6174";
			}
			else if (_stricmp(request.m_strMethod.c_str(), "SET_PARAMETER") == 0)
			{
				auto ct = request.m_mapHeader.find("content-type");

				if (ct != request.m_mapHeader.end())
				{
					if (_stricmp(ct->second.c_str(), "application/x-dmap-tagged") == 0)
					{
						if (!request.m_strBody.empty())
						{
							pRaopContext->ResetSongInfos();

							dmap_parse(pRaopContext.get(), request.m_strBody.c_str(), request.m_strBody.length());

							if (dlgMain.m_hWnd && dlgMain.IsCurrentContext(pRaopContext))
								dlgMain.PostMessage(WM_SONG_INFO);
						}
					}
					else if (ct->second.length() > 6 && _strnicmp(ct->second.c_str(), "image/", 6) == 0)
					{
						pRaopContext->Lock();

						pRaopContext->DeleteImage();

						if (_stricmp(ct->second.c_str()+6, "none") == 0)
						{
							// do nothing
						}
						else if (_stricmp(ct->second.c_str()+6, "jpeg") == 0 || _stricmp(ct->second.c_str()+6, "png") == 0)
						{
							pRaopContext->m_nBitmapBytes = 0;

							long nBodyLength = (long) request.m_strBody.length();

							if (nBodyLength)
							{
								if (pRaopContext->m_binBitmap.Reallocate(nBodyLength))
								{
									memcpy(pRaopContext->m_binBitmap, request.m_strBody.c_str(), nBodyLength);
									pRaopContext->m_nBitmapBytes = nBodyLength;

									pRaopContext->m_bmInfo = BitmapFromBlob(pRaopContext->m_binBitmap, pRaopContext->m_nBitmapBytes);
								}
							}
						}
						else
						{
							_LOG("Unknown Imagetype: %s\n", ct->second.c_str());
						}
						pRaopContext->Unlock();

						if (dlgMain.m_hWnd && dlgMain.IsCurrentContext(pRaopContext))
						{
							dlgMain.PostMessage(WM_INFO_BMP);
							dlgMain.PostMessage(WM_SONG_INFO);
						}
					}
					else if (_stricmp(ct->second.c_str(), "text/parameters") == 0)
					{
						if (!request.m_strBody.empty())
						{
							std::map<ic_string, std::string>	mapKeyValue;

							ParseRegEx(request.m_strBody, "[^\\s:]+[\\s]*:[\\s]*[^\\s]+[\r\n]*"
                                , [&mapKeyValue](std::string s) -> bool
                            {
                                std::string strLeft;
                                std::string strRight;

                                if (!ParseRegEx(s, "[^\\s:]+", [&strLeft](std::string t) -> bool
                                {
                                    strLeft = t;
                                    return false;
                                }))
                                {
                                    return false;
                                }
                                if (!ParseRegEx(s, ":[\\s]*[^\\s]+", [&strRight](std::string t) -> bool
                                {
                                    Trim(t, " \t\r\n:");
                                    strRight = t;
                                    return false;
                                }))
                                {
                                    return false;
                                }
                                mapKeyValue[strLeft.c_str()] = strRight;
                                return true;
                            });

							if (!mapKeyValue.empty())
							{
								auto i = mapKeyValue.find("volume");

								if (i != mapKeyValue.end())
								{
									m_Volume->SetVolume(atof(i->second.c_str()));
								}
								else
								{
									i = mapKeyValue.find("progress");

									if (i != mapKeyValue.end())
									{
										std::list<std::string> listProgressValues;

                                        ParseRegEx(i->second, "[\\d]+", [&listProgressValues](std::string s) -> bool
                                        {
                                            listProgressValues.push_back(s);
                                            return true;
                                        });

										if (listProgressValues.size() >= 3)
										{
											auto tsi = listProgressValues.begin();

											double start	= atof(tsi->c_str());
											double curr		= atof((++tsi)->c_str());
											double end		= atof((++tsi)->c_str());

											if (end >= start)
											{
												// obviously set wrongly by some Apps (e.g. Soundcloud) 
												if (start > curr)
												{
													double _curr = curr;
													curr	= start;
													start	= _curr;
												}
												time_t nTotalSecs = (time_t)((end-start) / pRaopContext->m_lfFreq);

												pRaopContext->Lock();

												pRaopContext->m_rtpStart		= MAKEULONGLONG((ULONG)start, 0);
												pRaopContext->m_rtpCur			= MAKEULONGLONG((ULONG)curr, 0);
												pRaopContext->m_rtpEnd			= MAKEULONGLONG((ULONG)end, 0);
												pRaopContext->m_nTimeStamp		= ::time(NULL);
												pRaopContext->m_nTimeTotal		= nTotalSecs;

												if (curr > start)
													pRaopContext->m_nTimeCurrentPos	= (time_t)((curr-start) / pRaopContext->m_lfFreq); 
												else
													pRaopContext->m_nTimeCurrentPos	= 0;

												pRaopContext->m_nDurHours = (long)(nTotalSecs / 3600);
												nTotalSecs -= ((time_t)pRaopContext->m_nDurHours*3600);

												pRaopContext->m_nDurMins = (long)(nTotalSecs / 60);
												pRaopContext->m_nDurSecs = (long)(nTotalSecs % 60);

												pRaopContext->Unlock();

												if (dlgMain.m_hWnd && dlgMain.IsCurrentContext(pRaopContext))
													dlgMain.PostMessage(WM_RESUME, 1);
											}
										}
										else
										{
											_LOG("Unexpected Value of Progress Parameter: %s\n", i->second.c_str());
										}
									}
									else
									{
										_LOG("Unknown textual parameter: %s\n", request.m_strBody.c_str());
									}
								}
							}
							else
							{
								_LOG("No Key Values in textual parameter: %s\n", request.m_strBody.c_str());
							}
						}
					}
					else
					{
						_LOG("Unknown Parameter Content Type: %s\n", ct->second.c_str());
					}
				}
			}
			std::string strResponse = response.GetAsString();

			strResponse += "\r\n";

			// _LOG("++++++++ Me +++++++++\n%s-------- Me ---------\n", strResponse.c_str());

			if (!sock.Send(strResponse.c_str(), (int)strResponse.length()))
			{
				_LOG("Communication failed\n");
			}
			request.InitNew();
		}
		sock.Detach();
	}
	virtual void OnDisconnect(SOCKET sd)
	{
		std::string strIP;

		GetPeerIP(sd, m_bV4, strIP);

		_LOG("Client %s disconnected\n", strIP.c_str());
		c_mtxConnection.Lock();

		auto i = c_mapConnection.find(sd);

		if (i != c_mapConnection.end())
		{
			auto pRaopContext = i->second;

			c_mapConnection.erase(i);
			c_mtxConnection.Unlock();

			if (dlgMain.m_hWnd && dlgMain.IsCurrentContext(pRaopContext))
			{
				dlgMain.PostMessage(WM_RAOP_CTX, 0, NULL);
			}
			if (pRaopContext->m_bIsStreaming)
			{
				pRaopContext->m_pDecoder->Stop();

				if (pSetThreadExecutionState)
					pSetThreadExecutionState(ES_CONTINUOUS);
			}
		}
		else
			c_mtxConnection.Unlock();
	}
protected:
	const std::shared_ptr<CHairTunes::Volume>	m_Volume;
	bool										m_bV4;
};

class CRaopServerV6 : public CRaopServer
{
public:
	CRaopServerV6(const std::shared_ptr<CHairTunes::Volume>& volume)
		: CRaopServer(volume)
	{
	}

	BOOL Run(int nPort)
	{
		m_bV4 = false;

		SOCKADDR_IN6 in;

		memset(&in, 0, sizeof(in));

		in.sin6_family		= AF_INET6;
		in.sin6_port		= nPort;
		in.sin6_addr		= in6addr_any;

		return CNetworkServer::Run(in);
	}
};

class CRaopServerV4 : public CRaopServer
{
public:
	CRaopServerV4(const std::shared_ptr<CHairTunes::Volume>& volume)
		: CRaopServer(volume)
	{
	}

	BOOL Run()
	{
		m_bV4 = true;

		int		nPort	= 5000;
		BOOL	bResult = FALSE;
		int		nTry	= 10;

		do 
		{
			bResult	= CNetworkServer::Run(htons(nPort++));
		}
		while(!bResult && nTry--);

		return bResult;
	}
};

const std::shared_ptr<CHairTunes::Volume> volume{ std::make_shared<CHairTunes::Volume>() };
static CRaopServerV6			raop_v6_server(volume);
static CRaopServerV4			raop_v4_server(volume);
static CDnsSD_Register			dns_sd_register_server;
static CDnsSD_BrowseForService	dns_sd_browse_dacp;

bool is_streaming()
{
	bool bResult = false;

	c_mtxConnection.Lock();

	for (auto i = c_mapConnection.begin(); i != c_mapConnection.end(); ++i)
	{
		if (i->second->m_bIsStreaming)
		{
			bResult = true;
			break;
		}
	}
	c_mtxConnection.Unlock();

	return bResult;
}

bool start_serving()
{
	// try to get hw_addr from registry
	std::string				strHwAddr;
	ATL::CTempBuffer<BYTE>	HwAddr;

	if (	GetValueFromRegistry(HKEY_CURRENT_USER, "HwAddr", strHwAddr)
		&&	my_Base64::base64_decode(strHwAddr, &HwAddr) == 6
		)
	{
		memcpy(hw_addr, HwAddr, 6);
	}
	else
	{
		for (long i = 1; i < 6; ++i)
		{
			hw_addr[i] = (256 * rand()) / RAND_MAX;
		}
	}
	hw_addr[0] = 0;

	if (!raop_v4_server.Run())
	{
		_LOG("can't listen on IPv4\n");
		::MessageBoxA(NULL, "Can't start network service on IPv4", strConfigName.c_str(), MB_ICONERROR|MB_OK);
		return false;
	}
	if (bLogToConsole || bLogToFile)
	{
		if (!Log("listening at least on IPv4...port %lu\n", (ULONG)ntohs(raop_v4_server.m_nPort)))
			::MessageBoxA(NULL, "Can't log to file. Please run elevated", strConfigName.c_str(), MB_ICONERROR|MB_OK);
	}
	
	if (!InitBonjour())
	{
		raop_v4_server.Cancel();
		raop_v6_server.Cancel();

		ATL::CString strInstBonj;

		ATLVERIFY(strInstBonj.LoadString(IDS_INSTALL_BONJOUR));

		if (IDOK == ::MessageBox(NULL, strInstBonj, CA2CT(strConfigName.c_str()), MB_ICONINFORMATION|MB_OKCANCEL))
		{
			ShellExecute(NULL, L"open", L"http://support.apple.com/kb/DL999", NULL, NULL, SW_SHOWNORMAL);
		}
		return false;
	}

	bool bResult = dns_sd_register_server.Start(hw_addr, CW2A(dlgMain.m_strApName, CP_UTF8), dlgMain.m_bNoMetaInfo, dlgMain.m_strPassword.IsEmpty(), raop_v4_server.m_nPort);

	if (!bResult)
	{
		long nTry			= 10;
		PCSTR strApName	= CW2A(dlgMain.m_strApName, CP_UTF8);

		do
		{
			_LOG("Could not register Raop.Tcp Service on Port %lu with dnssd.dll\n", (ULONG)ntohs(raop_v4_server.m_nPort));

			Sleep(1000);
			bResult = dns_sd_register_server.Start(hw_addr, strApName, dlgMain.m_bNoMetaInfo, dlgMain.m_strPassword.IsEmpty(), raop_v4_server.m_nPort);
			nTry--;
		}
		while(!bResult && nTry > 0);

		if (!bResult)
			::MessageBoxA(NULL, "Could not register Raop.Tcp Service with dnssd.dll", strConfigName.c_str(), MB_ICONERROR);
	}
	if (bResult)
	{
		_LOG("Registered Raop.Tcp Service ok\n");

		if (!dlgMain.m_bNoMediaControl)
		{
			if (!dns_sd_browse_dacp.Start("_dacp._tcp", dlgMain.m_hWnd, WM_DACP_REG))
			{
				_LOG("Failed to start DACP browser!\n");
				ATLASSERT(FALSE);
			}
			else
			{
				_LOG("Succeeded to start DACP browser!\n");
			}
		}
		else
		{
			_LOG("Omitted to start DACP browser - no Media Controls desired\n");
		}
	}
	else
	{
		_LOG("Raop.Tcp Service failed to register - Dead End!\n");

		raop_v4_server.Cancel();
		raop_v6_server.Cancel();
	}
	return bResult;
}

void stop_serving()
{
	_LOG("Stopping Services .... ");

	raop_v6_server.Cancel();
	raop_v4_server.Cancel();

	dns_sd_browse_dacp.Stop();
	dns_sd_register_server.Stop();

	_LOG("stopped ok!\n");
}


/////////////////////////////////////////////////////////////////////
// OnDTKeyboardEvent - global Windows Hook for Multimedia Keys

LRESULT CALLBACK OnDTKeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
	// Attention:
	// The hook procedure should process a message in less time than the data entry specified in the LowLevelHooksTimeout value in the following registry key: 
	// HKEY_CURRENT_USER\Control Panel\Desktop
	// The value is in milliseconds. If the hook procedure times out, the system passes the message to the next hook.
	// However, on Windows 7 and later, the hook is *silently removed* without being called. There is no way for the application to know whether the hook is removed.
	//

	if (nCode == HC_ACTION)
	{
		if (wParam == WM_KEYDOWN)
		{
			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;

			if (p)
			{
				switch(p->vkCode)
				{
					case VK_MEDIA_PLAY_PAUSE:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_MEDIA_PLAY_PAUSE)); 
					}
					break;

					case VK_VOLUME_MUTE:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_VOLUME_MUTE)); 
					}
					break;

					case VK_VOLUME_DOWN:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_VOLUME_DOWN)); 
					}
					break;

					case VK_VOLUME_UP:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_VOLUME_UP)); 
					}
					break;

					case VK_MEDIA_NEXT_TRACK:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_MEDIA_NEXTTRACK)); 
					}
					break;

					case VK_MEDIA_PREV_TRACK:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_MEDIA_PREVIOUSTRACK)); 
					}
					break;

					case VK_MEDIA_STOP:
					{
						dlgMain.PostMessage(WM_APPCOMMAND, (WPARAM)dlgMain.m_hWnd, MAKELONG(0, APPCOMMAND_MEDIA_STOP)); 
					}
					break;
				}
			}
		}
	}
	return CallNextHookEx(g_hDesktopHook, nCode, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////
// Run

int Run(LPTSTR lpstrCmdLine = NULL)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	dlgMain.ReadConfig();

	if (dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	if (start_serving())
    {
		dlgMain.ShowWindow(dlgMain.m_bStartMinimized ? dlgMain.m_bTray ? SW_HIDE : SW_SHOWMINIMIZED : SW_SHOWDEFAULT);

		if (dlgMain.m_bGlobalMMHook)
		{
			if ((g_hDesktopHook = SetWindowsHookEx(WH_KEYBOARD_LL, OnDTKeyboardEvent, NULL, 0)) == NULL)
			{
				ATLTRACE(L"Shairport4w: Sorry, no windows hook for us...\n");
			}
		}
		int nRet = theLoop.Run();

		if (g_hDesktopHook)
		{
			UnhookWindowsHookEx(g_hDesktopHook);
			g_hDesktopHook = NULL;
		}
		_Module.RemoveMessageLoop();

		stop_serving();

		return nRet;
	}
    else
    {
        dlgMain.DestroyWindow();
    }
	return 0;
}

/////////////////////////////////////////////////////////////////////
// _tWinMain

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);	// add flags to support other controls

	setlocale(LC_ALL, "");

    if (mtxAppSessionInstance.Create(NULL, FALSE, ATL::CString(_T("__")) + ATL::CString(CA2CT(strConfigName.c_str())) + ATL::CString(_T("SessionInstanceIsRunning"))))
    {
        while (WaitForSingleObject(mtxAppSessionInstance, 0) != WAIT_OBJECT_0)
        {
            HANDLE hShowEvent = ::OpenEvent(EVENT_MODIFY_STATE, FALSE, EVENT_NAME_SHOW_WINDOW);

            if (hShowEvent)
            {
                SetEvent(hShowEvent);
                CloseHandle(hShowEvent);
                hShowEvent = NULL;
            }
            else
            {
                ATL::CString strMsg;
                ATLVERIFY(strMsg.LoadString(IDS_ERR_MULTIPLE_INSTANCES));
                ::MessageBoxW(NULL, strMsg, CA2W(strConfigName.c_str()), MB_ICONWARNING | MB_OK);
            }
            ::CoUninitialize();
            return 0;
        }
    }
    else
    {
        ATLASSERT(FALSE);
    }
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR			gdiplusToken				= 0;

	gdiplusStartupInput.GdiplusVersion				= 1;
	gdiplusStartupInput.DebugEventCallback			= NULL;
	gdiplusStartupInput.SuppressBackgroundThread	= FALSE;
	gdiplusStartupInput.SuppressExternalCodecs		= FALSE; 
		
	ATLVERIFY(GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) == Gdiplus::Ok);

	std::string _strLogToFile;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "LogToFile", _strLogToFile))
		bLogToFile = _strLogToFile != "no" ? TRUE : FALSE;

    if (!Rsa)
    {
        CTempBuffer<BYTE> pKey;

        // decode to key blob
        ULONG sizeKey = my_Base64::base64_decode(strPrivateKey, &pKey);

        // set key blob to rsa
        ATLVERIFY(Rsa = std::make_shared<my_crypt::Rsa>());
        ATLVERIFY(Rsa->Open(pKey, sizeKey));
    }
	srand(GetTickCount());

    HMODULE hKernel32 = ::LoadLibraryA("Kernel32.dll");

	if (hKernel32)
	{
		pSetThreadExecutionState = (_SetThreadExecutionState) GetProcAddress(hKernel32, "SetThreadExecutionState");
	}
    int nRet = Run(lpstrCmdLine);

	c_mtxConnection.Lock();
	c_mapConnection.clear();
	c_mtxConnection.Unlock();

	GdiplusShutdown(gdiplusToken);

	if (hKernel32)
	{
		FreeLibrary(hKernel32);
	}
    Rsa.reset();

	_Module.Term();
	DeInitBonjour();

	::CoUninitialize();

	return nRet;
}

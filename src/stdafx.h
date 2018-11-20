/*
 *
 *  stdafx.h
 *
 */

#pragma once

// Change these values to use different versions
#define WINVER			_WIN32_WINNT_VISTA
#define _WIN32_WINNT	_WIN32_WINNT_VISTA
#define _WIN32_IE		0x0600
#define _RICHEDIT_VER	0x0300

#include <sdkddkver.h>

#include <atlbase.h>
#include <atlapp.h>

extern	CAppModule				_Module;
extern	volatile LONG			g_bMute;

#define	MUTE_FROM_TOGGLE_BUTTON		0x00000001
#define	MUTE_FROM_EXTERN			0x00000002

#include <atlwin.h>

#include <atlstr.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include <atlCtrlx.h>
#include <atldlgs.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atlgdi.h>

#include <atlsync.h>

#include <ws2tcpip.h>

#include <comdef.h>

#include <list>
#include <queue>
#include <deque>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <functional>
#include <memory>

#include <GdiPlus.h>

using namespace Gdiplus;

#include <Wincodec.h>

#include "myCrypt.h"
#include "base64.h"
#include "Networking.h"
#include "myMutex.h"
#include "utils.h"
#include "sp_bonjour.h"
#include "http_parser.h"
#include "HairTunes.h"
#include "RaopContext.h"
#include "RaopContextImpl.h"
#include "Config.h"

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

bool start_serving();
void stop_serving();
bool is_streaming();

#include "resource.h"

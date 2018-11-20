/*
 *
 *  Config.h
 *
 */

#pragma once

#include <time.h>

#define NUM_CHANNELS	2
#define	SAMPLE_SIZE		16
#define	SAMPLE_FACTOR	(NUM_CHANNELS * (SAMPLE_SIZE >> 3))
#define	SAMPLE_FREQ		(44100.0l)
#define BYTES_PER_SEC	(SAMPLE_FREQ * SAMPLE_FACTOR)

#define APP_NAME_A "Shairport4w"
#define APP_NAME_W L"Shairport4w"

#define EVENT_NAME_EXIT_PROCESS _T("Shairport4w_ExitProcess")
#define EVENT_NAME_SHOW_WINDOW	_T("Shairport4w_ShowMain")

extern	CMutex		mtxAppSessionInstance;
extern	CMutex		mtxAppGlobalInstance;
extern	std::string		strConfigName;

class IRaopContext;



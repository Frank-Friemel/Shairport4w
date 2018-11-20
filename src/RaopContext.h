/*
 *
 *  RaopContext.h
 *
 */

#pragma once


class IRaopContext
{
public:
	virtual void	Lock(bool bExclusive)								= 0;
	virtual void	Lock()												= 0;
	virtual void	Unlock()											= 0;
	virtual bool	TryLock()											= 0;
	virtual void	ResetSongInfos()									= 0;
	virtual time_t	GetTimeStamp()										= 0;
	virtual void	PutTimeStamp(time_t tValue)							= 0;
	virtual time_t	GetTimeTotal()										= 0;
	virtual void	PutTimeTotal(time_t tValue)							= 0;
	virtual time_t	GetTimeCurrentPos()									= 0;
	virtual void	PutTimeCurrentPos(time_t tValue)					= 0;
	virtual long	GetDurHours()										= 0;
	virtual void	PutDurHours(long nValue)							= 0;
	virtual long	GetDurMins()										= 0;
	virtual void	PutDurMins(long nValue)								= 0;
	virtual long	GetDurSecs()										= 0;
	virtual void	PutDurSecs(long nValue)								= 0;
	virtual void	GetSongalbum(PWSTR pBuf, ULONG nMax)				= 0;
	virtual long	GetSongalbumCount()									= 0;
	virtual void	PutSongalbum(PCWSTR strValue)						= 0;
	virtual void	GetSongartist(PWSTR pBuf, ULONG nMax)				= 0;
	virtual long	GetSongartistCount()								= 0;
	virtual void	PutSongartist(PCWSTR strValue)						= 0;
	virtual void	GetSongtrack(PWSTR pBuf, ULONG nMax)				= 0;
	virtual long	GetSongtrackCount()									= 0;
	virtual void	PutSongtrack(PCWSTR strValue)						= 0;
	virtual Bitmap* GetBitmap()											= 0;
	virtual void	PutBitmap(Bitmap* bmValue)							= 0;
	virtual BYTE*	GetBitmapBytes()									= 0;
	virtual long	GetBitmapByteCount()								= 0;
	virtual bool	GetHICON(HICON* phIcon)								= 0;
	virtual void	DeleteImage()										= 0;
	virtual unsigned long long&		GetRtpStart()						= 0;
	virtual unsigned long long&		GetRtpCur()							= 0;
	virtual unsigned long long&		GetRtpEnd()							= 0;
	virtual double&					GetFreq()							= 0;
};
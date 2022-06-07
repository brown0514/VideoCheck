//////////////////////////////////////////////////////////////////////////
//
// SG Video Validator
//
//////////////////////////////////////////////////////////////////////////

#pragma once
struct FormatInfo
{
	UINT32          imageWidthPels;
	UINT32          imageHeightPels;
	BOOL            bTopDown;
	RECT            rcPicture;    // Corrected for pixel aspect ratio

	FormatInfo() : imageWidthPels(0), imageHeightPels(0), bTopDown(FALSE)
	{
		SetRectEmpty(&rcPicture);
	}
};


class SG_VideoValidator
{
private:

    IMFSourceReader *m_pReader;
    FormatInfo      m_format;

public:

    SG_VideoValidator();
    ~SG_VideoValidator();


    int			OpenFile(const WCHAR* wszFileName);
    HRESULT     GetDuration(LONGLONG *phnsDuration);

private:
	HRESULT		SetPosition(LONGLONG hnsPosition);
	HRESULT     SelectVideoStream();
	HRESULT		SelectAudioStream();
    HRESULT     GetVideoFormat(FormatInfo *pFormat);
};

#define VIDEOVALIDATOR_VIDEO_OK				0
#define VIDEOVALIDATOR_ERROR_OPENING		1
#define VIDEOVALIDATOR_CORRUPTED			2
#define VIDEOVALIDATOR_NO_AUDIO				3
#define VIDEOVALIDATOR_CORRUPTED_NO_AUDIO	4

#define MAX_AUDIO_SAMPLE_READ_BYTES		(1 << 20)
#define MIN_AUDIO_SAMPLE_READ_BYTES		(1 << 15)


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}





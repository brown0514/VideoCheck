//////////////////////////////////////////////////////////////////////////
//
// Validates a video
// 
//////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "winmain.h"
#include "videovalidator.h"
#include "SG_VideoValidator.h"
#include <list>

#pragma warning(disable:4127)  // Disable warning C4127: conditional expression is constant


//-------------------------------------------------------------------
// SG_VideoValidator constructor
//-------------------------------------------------------------------

SG_VideoValidator::SG_VideoValidator()
	: m_pReader(NULL)
{
	ZeroMemory(&m_format, sizeof(m_format));
}



//-------------------------------------------------------------------
// SG_VideoValidator destructor
//-------------------------------------------------------------------

SG_VideoValidator::~SG_VideoValidator()
{
	//    SafeRelease(&m_pReader);
}




//-------------------------------------------------------------------
// OpenFile: Opens a video file.
// Return value
// 0 : It's OK
// 1 : Failed to open file.
// 2 : Video is corrupted.
// 3 : Video has no audio.
//-------------------------------------------------------------------

int SG_VideoValidator::OpenFile(const WCHAR* wszFileName)
{
	HRESULT hr = S_OK;
	//ShowStatus(L"Opening file %s", wszFileName);
	IMFAttributes* pAttributes = NULL;

	SafeRelease(&m_pReader);

	// Configure the source reader to perform video processing.
	//
	// This includes:
	//   - YUV to RGB-32
	//   - Software deinterlace

	hr = MFCreateAttributes(&pAttributes, 1);

	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
	}

	// Create the source reader from the URL.

	if (SUCCEEDED(hr))
	{
		hr = MFCreateSourceReaderFromURL(wszFileName, pAttributes, &m_pReader);
	}

	if (FAILED(hr))
	{
		ShowStatus(L"=> Error opening file");
		return VIDEOVALIDATOR_ERROR_OPENING;
	}

	// Attempt to find a video stream.
	hr = SelectVideoStream();
	if (FAILED(hr))
	{
		ShowStatus(L"=> Video file corrupted");
		hr = SelectAudioStream();
		if (FAILED(hr)) 
		{
			ShowStatus(L"=> Video file has no audio");
			return VIDEOVALIDATOR_CORRUPTED_NO_AUDIO;
		}
		return VIDEOVALIDATOR_CORRUPTED;
	}
	hr = SelectAudioStream();
	if (FAILED(hr))
	{
		ShowStatus(L"=> Video file has no audio");
		return VIDEOVALIDATOR_NO_AUDIO;
	}
//	ShowStatus(L"=> Video file OK");
	return VIDEOVALIDATOR_VIDEO_OK;
}


//-------------------------------------------------------------------
// GetDuration: Finds the duration of the current video file.
//-------------------------------------------------------------------

HRESULT SG_VideoValidator::GetDuration(LONGLONG *phnsDuration)
{
	PROPVARIANT var;
	PropVariantInit(&var);

	HRESULT hr = S_OK;

	if (m_pReader == NULL)
	{
		return MF_E_NOT_INITIALIZED;
	}

	hr = m_pReader->GetPresentationAttribute(
		(DWORD)MF_SOURCE_READER_MEDIASOURCE,
		MF_PD_DURATION,
		&var
	);

	if (SUCCEEDED(hr))
	{
		assert(var.vt == VT_UI8);
		*phnsDuration = var.hVal.QuadPart;
	}

	PropVariantClear(&var);

	return hr;
}




//-------------------------------------------------------------------
// GetVideoFormat
// 
// Gets format information for the video stream.
//
// iStream: Stream index.
// pFormat: Receives the format information.
//-------------------------------------------------------------------

HRESULT SG_VideoValidator::GetVideoFormat(FormatInfo *pFormat)
{
	HRESULT hr = S_OK;
	UINT32  width = 0, height = 0;
	LONG lStride = 0;
	MFRatio par = { 0 , 0 };

	FormatInfo& format = *pFormat;

	GUID subtype = { 0 };

	IMFMediaType *pType = NULL;

	// Get the media type from the stream.
	hr = m_pReader->GetCurrentMediaType(
		(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
		&pType
	);

	if (FAILED(hr))
	{
		goto done;
	}

	// Make sure it is a video format.
	hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
	if (subtype != MFVideoFormat_RGB32)
	{
		hr = E_UNEXPECTED;
		goto done;
	}

	// Get the width and height
	hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);

	if (FAILED(hr))
	{
		goto done;
	}


done:
	SafeRelease(&pType);

	return hr;
}

//-------------------------------------------------------------------
// SelectVideoStream
//
// Finds the first video stream and sets the format to RGB32.
//-------------------------------------------------------------------

HRESULT SG_VideoValidator::SelectVideoStream()
{
	HRESULT hr = S_OK;

	IMFMediaType *pType = NULL;

	// Configure the source reader to give us progressive RGB32 frames.
	// The source reader will load the decoder if needed.

	hr = MFCreateMediaType(&pType);

	if (SUCCEEDED(hr))
	{
		hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	}

	GUID subtype = { 0 };
	if (SUCCEEDED(hr))
	{
		// Get the media type from the stream.
		hr = m_pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType);

		if (SUCCEEDED(hr))
		{
			hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = pType->SetGUID(MF_MT_SUBTYPE, subtype);//MFVideoFormat_RGB32
		if (SUCCEEDED(hr))
		{
			hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
		}
	}

	// Ensure the stream is selected.
	if (SUCCEEDED(hr))
	{
		hr = m_pReader->SetStreamSelection(
			(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			TRUE
		);
	}

	// 	if (SUCCEEDED(hr))
	// 	{
	// 		hr = GetVideoFormat(&m_format);
	// 	}

	SafeRelease(&pType);
	return hr;
}

HRESULT SG_VideoValidator::SetPosition(LONGLONG hnsPosition)
{
	PROPVARIANT var;
	HRESULT hr = InitPropVariantFromInt64(hnsPosition, &var);
	if (SUCCEEDED(hr))
	{
		hr = m_pReader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);
	}
	return hr;
}

//-------------------------------------------------------------------
// SelectAudioStream
//-------------------------------------------------------------------

HRESULT SG_VideoValidator::SelectAudioStream()
{
	HRESULT hr = S_OK;

	IMFMediaType* pType = NULL;
	IMFMediaType* outputMediaType = NULL;

	hr = MFCreateMediaType(&pType);

	if (SUCCEEDED(hr))
	{
		hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	}

	if (SUCCEEDED(hr))
	{
		hr = pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	}


	if (SUCCEEDED(hr))
	{
		hr = m_pReader->SetCurrentMediaType(
			(DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			NULL,
			pType
		);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputMediaType);
	}

	UINT32 formatSize = 0;
	WAVEFORMATEX* waveFormat;
	if (SUCCEEDED(hr))
	{
		MFCreateWaveFormatExFromMFMediaType(outputMediaType, &waveFormat, &formatSize);
		CoTaskMemFree(waveFormat);

		IMFSample* audioSample = NULL;

		int nCnt = 0, nBytes = 0, nTotalBytes = 0;
		int nZeroCnt = 0, nOneCnt = 0, nTwoCnt = 0, nFFCnt = 0;
		LONGLONG lDuration;
		GetDuration(&lDuration);
		SetPosition(lDuration >> 1);
		BOOL isFirst = TRUE;
		for (;;)
		{
			DWORD streamIndex, flags;
			LONGLONG llAudioTimeStamp;
			hr = m_pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llAudioTimeStamp, &audioSample);

			if (!SUCCEEDED(hr)) 
			{
				break;
			}

			if (flags & MF_SOURCE_READERF_ENDOFSTREAM) 
			{
				if (isFirst) 
				{
					SetPosition(0);
					isFirst = FALSE;
					continue;
				}
				break;
			}

			IMFMediaBuffer* buffer;
			BYTE* data;
			DWORD sampleBufferLength;
			audioSample->ConvertToContiguousBuffer(&buffer);

			buffer->Lock(&data, NULL, &sampleBufferLength);
			for (int i = 0; i < sampleBufferLength; i++)
			{
				if (data[i] == 0) nZeroCnt++;
				else if (data[i] == 1) nOneCnt++;
				else if (data[i] == 2) nTwoCnt++;
				else if (data[i] == 0xFF) nFFCnt++;
			}
			nTotalBytes += sampleBufferLength;
			nBytes += sampleBufferLength;
			
			buffer->Unlock();
			SafeRelease(&buffer);
			SafeRelease(&audioSample);
			if (nBytes >= MIN_AUDIO_SAMPLE_READ_BYTES)
			{
				if ((float)(nZeroCnt + nOneCnt + nTwoCnt + nFFCnt) / (float)nTotalBytes <= 0.97f)
					break;
				if (nTotalBytes >= MAX_AUDIO_SAMPLE_READ_BYTES)
					break;
				nBytes = 0;
			}
		}

		float fPercent = 1.0f;
		if (nTotalBytes)
			fPercent = (float)(nZeroCnt + nOneCnt + nTwoCnt + nFFCnt) / (float)nTotalBytes;
		if (fPercent > 0.97f) return -1;
	}

	SafeRelease(&pType);
	return hr;
}


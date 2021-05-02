/**
*	@file	GdiplusUtil.cpp
*	@brief	Gdi+を使うのを便利にする
*/

#include "stdafx.h"
#include "GdiplusUtil.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;


namespace {

class CGdlplusUtil
{
public:
	CGdlplusUtil()
		: m_token(0)
		, m_nEncoders(0)
		, m_pImageCodecInfo(NULL)
	{	}

	void	Init()
	{
		GdiplusStartupInput	gdiplusStartupInput;
		GdiplusStartup(&m_token, &gdiplusStartupInput, NULL);

		UINT	size;
		GetImageEncodersSize(&m_nEncoders, &size);
		m_pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
		GetImageEncoders(m_nEncoders, size, m_pImageCodecInfo);
	}
	void	Term()
	{
		if (m_pImageCodecInfo) {
			free(m_pImageCodecInfo);
			m_pImageCodecInfo = nullptr;
			Gdiplus::GdiplusShutdown(m_token);
			m_token = 0;
		}
	}

	ImageCodecInfo*	GetEncoderByExtension(LPCWSTR extension)
	{
		for (UINT i = 0; i < m_nEncoders; ++i) {
			if (PathMatchSpecW(extension, m_pImageCodecInfo[i].FilenameExtension))
				return &m_pImageCodecInfo[i];
		}
		return nullptr;
	}

	ImageCodecInfo*	GetEncoderByMimeType(LPCWSTR mimetype)
	{
		for (UINT i = 0; i < m_nEncoders; ++i) {
			if (wcscmp(m_pImageCodecInfo[i].MimeType, mimetype) == 0)
				return &m_pImageCodecInfo[i];
		}
		return nullptr;
	}

private:
	ULONG_PTR	m_token;
	UINT		m_nEncoders;
	ImageCodecInfo* m_pImageCodecInfo;

	
};

CGdlplusUtil	GdiplusUtil;

};	// namespace


void	GdiplusInit()
{
	GdiplusUtil.Init();
}

void	GdiplusTerm()
{
	GdiplusUtil.Term();
}




//---------------------------------------
/// 拡張子を指定してエンコーダーを取得する
Gdiplus::ImageCodecInfo*	GetEncoderByExtension(LPCWSTR extension)
{
	return GdiplusUtil.GetEncoderByExtension(extension);
}


//--------------------------------------
/// MIMEタイプを指定してエンコーダを取得する
Gdiplus::ImageCodecInfo*	GetEncoderByMimeType(LPCWSTR mimetype)
{
	return GdiplusUtil.GetEncoderByMimeType(mimetype);
}













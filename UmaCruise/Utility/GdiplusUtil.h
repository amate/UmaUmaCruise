/**
*	@file	GdiplusUtil.h
*	@brief	Gdi+を使うのを便利にする
*/

#pragma once

#include <GdiPlus.h>

// 初期化/後始末
void	GdiplusInit();
void	GdiplusTerm();

//---------------------------------------
/// 拡張子を指定してエンコーダーを取得する
Gdiplus::ImageCodecInfo*	GetEncoderByExtension(LPCWSTR extension);

//--------------------------------------
/// MIMEタイプを指定してエンコーダを取得する
Gdiplus::ImageCodecInfo*	GetEncoderByMimeType(LPCWSTR mimetype);


















#pragma once
#include <Windows.h>
#include <string>
#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace ::std;
using namespace ::Gdiplus;

#include "Util.h"

class Screenshot
{
public:
	Screenshot();
	~Screenshot();

	// 指定されたウインドウのビットマップ情報を取得して返却します
	static HBITMAP Screenshot::GetBitmapFromWindow(HWND window, BITMAPINFO *pbmi, void **pbits, RECT *rect);

	// MIME-TYPEをもとにEncoderを取得します
	static BOOL Screenshot::GetClsidEncoderFromMimeType(LPCTSTR format, LPCLSID lpClsid);

	// ファイル名をもとに、Enocderを取得します
	static BOOL Screenshot::GetClsidEncoderFromFileName(LPCTSTR fileName, LPCLSID lpClsid);

	// 指定されたファイル名で、hBitmapを保存します
	static BOOL Screenshot::SaveToFileAutoDetectFormat(HBITMAP hBitmap, LPCTSTR fileName);

	// 指定のウインドウの、指定の範囲をスクリーンショットします
	static BOOL Screenshot::ScreenshotWindow(LPCTSTR fileName, HWND window, RECT *rect);

	// ファイルに保存せず、HBITMAP形式で返却します
	static HBITMAP Screenshot::ScreenshotInMemory(HWND window, RECT *rect);

	// Desktopのスクリーンショットを撮影してファイルに保存します
	static BOOL Screenshot::ScreenshotDesktop(LPCTSTR fileName, RECT *rect);
};



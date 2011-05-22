#include "Screenshot.h"

Screenshot::Screenshot()
{
}

Screenshot::~Screenshot()
{
}

// 指定されたウインドウのビットマップ情報を取得して返却します
HBITMAP Screenshot::GetBitmapFromWindow(HWND window, BITMAPINFO *pbmi, void **pbits, RECT *rect)
{
	// デバイスコンテキスト取得
	HDC hdc = ::GetWindowDC(window);
	if(hdc == NULL){
		::ReleaseDC(window, hdc);
		::ShowLastError();
		return NULL;
	}

	// デバイスコンテキスト複製
	HDC myhdc = ::CreateCompatibleDC(hdc);
	if(myhdc == NULL){
		::DeleteDC(myhdc);
		::ShowLastError();
		return NULL;
	}
	
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = rect->right - rect->left;
	pbmi->bmiHeader.biHeight = rect->bottom - rect->top;
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biBitCount = ::GetDeviceCaps(hdc, BITSPIXEL); // デスクトップのカラーbit数取得
	pbmi->bmiHeader.biSizeImage = pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biHeight * (pbmi->bmiHeader.biBitCount / 8); // カラーbit数に合わせて倍数が変わる

	HBITMAP hBitmap = ::CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, pbits, NULL, 0);
	if(hBitmap == NULL){
		::ReleaseDC(window, hdc);
		::DeleteDC(myhdc);
		::ShowLastError();
		return NULL;
	}
	
	HBITMAP oldBitmap = (HBITMAP)::SelectObject(myhdc, hBitmap);
	if( ::BitBlt(myhdc, 0, 0, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight, hdc, rect->left, rect->top, SRCCOPY) == 0 ){
		::SelectObject(myhdc, oldBitmap);
		::ReleaseDC(window, hdc);
		::DeleteDC(myhdc);
		::ShowLastError();
		return NULL;
	}

	::SelectObject(myhdc, oldBitmap);
	::ReleaseDC(window, hdc);
	::DeleteDC(myhdc);
	return hBitmap;
}

// MIME-TYPEをもとにEncoderを取得します
BOOL Screenshot::GetClsidEncoderFromMimeType(LPCTSTR format, LPCLSID lpClsid)
{
	UINT num, size;
	if( ::GetImageEncodersSize(&num, &size) != Ok ){
		::ShowLastError();
		return FALSE;
	}

	// バッファを確保
	ImageCodecInfo *info = (ImageCodecInfo *)::GlobalAlloc(GMEM_FIXED, size);

	// エンコーダーの情報を転送
	if( ::GetImageEncoders(num, size, info) != Ok ){
		::GlobalFree(info);
		return FALSE;
	}

	for(UINT i=0; i<num; i++){
		if( wcscmp(info[i].MimeType, format) == 0 ){
			*lpClsid = info[i].Clsid;
			::GlobalFree(info); // found
			return TRUE;
		}
	}

	::GlobalFree(info); // not found
	return FALSE;
}

// ファイル名をもとに、Enocderを取得します
BOOL Screenshot::GetClsidEncoderFromFileName(LPCTSTR fileName, LPCLSID lpClsid)
{
	UINT num, size;
	if( ::GetImageEncodersSize(&num, &size) != Ok ){
		::ShowLastError();
		return FALSE;
	}

	// バッファを確保
	ImageCodecInfo *info = (ImageCodecInfo *)::GlobalAlloc(GMEM_FIXED, size);

	// エンコーダーの情報を転送
	if( ::GetImageEncoders(num, size, info) != Ok ){
		::GlobalFree(info);
		return FALSE;
	}

	for(UINT i=0; i<num; i++){
		if( PathMatchSpecW(fileName, info[i].FilenameExtension)){
			*lpClsid = info[i].Clsid;
			::GlobalFree(info); // found
			return TRUE;
		}
	}

	::GlobalFree(info); // not found
	return FALSE;
}

// 指定されたファイル名で、hBitmapを保存します
BOOL Screenshot::SaveToFileAutoDetectFormat(HBITMAP hBitmap, LPCTSTR fileName)
{
	ULONG_PTR token;
	::GdiplusStartupInput input;
	::GdiplusStartup(&token, &input, NULL);

	CLSID clsid;
	if( !GetClsidEncoderFromFileName(fileName, &clsid) ){
		::ShowLastError();
		::GdiplusShutdown(token);
		return FALSE;
	}

	Bitmap *b = new Bitmap(hBitmap, NULL);
	if( 0 != b->Save(fileName, &clsid) ){
		::ShowLastError();
		delete b;		
		::GdiplusShutdown(token);
		return FALSE;
	}
	
	delete b;
	::GdiplusShutdown(token);
	return TRUE;
}

// 指定のウインドウの、指定の範囲をスクリーンショットします
BOOL Screenshot::ScreenshotWindow(LPCTSTR fileName, HWND window, RECT *rect)
{
	HBITMAP hBitmap = ScreenshotInMemory(window, rect);
	BOOL bRet = SaveToFileAutoDetectFormat(hBitmap, fileName);
	::DeleteObject(hBitmap);
	return bRet;
}

// ファイルに保存せず、HBITMAP形式で返却します
HBITMAP Screenshot::ScreenshotInMemory(HWND window, RECT *rect)
{
	BITMAPINFO bmi = {0};
	void *pbits = NULL;
	return GetBitmapFromWindow(window, &bmi, &pbits, rect);
}

// Desktopのスクリーンショットを撮影してファイルに保存します
BOOL Screenshot::ScreenshotDesktop(LPCTSTR fileName, RECT *rect)
{
	return ScreenshotWindow(fileName, GetDesktopWindow(), rect);
}

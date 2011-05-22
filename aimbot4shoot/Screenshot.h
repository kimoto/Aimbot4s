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

	static BOOL ScreenshotDesktop(LPCTSTR fileName);
	static BOOL ScreenshotDesktop(LPCTSTR fileName, RECT *rect);
	static BOOL ScreenshotWindow(LPCTSTR fileName, HWND window, RECT *rect);
	static BOOL SaveToFileAutoDetectFormat(HBITMAP hBitmap, LPCTSTR fileName);
	static BOOL GetClsidEncoderFromFileName(LPCTSTR fileName, LPCLSID lpClsid);
	static BOOL GetClsidEncoderFromMimeType(LPCTSTR format, LPCLSID lpClsid);
	static BOOL SaveBitmapToFile(HBITMAP hBitmap, BITMAPINFO *pbmi, void *pbits, LPCTSTR fileName);
	static HBITMAP GetBitmapFromWindow(HWND window, BITMAPINFO *pbmi, void **pbits);
	static HBITMAP GetBitmapFromWindow(HWND window, BITMAPINFO *pbmi, void **pbits, RECT *rect);
	static HBITMAP ScreenshotMemoryWindow(HWND window, BITMAPINFO *pbmi, void **pbits, RECT *rect);

};

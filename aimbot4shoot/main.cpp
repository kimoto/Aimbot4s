#include <Windows.h>
#include <tchar.h>
#include "Util.h"
#include "Screenshot.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#pragma comment(lib, "opencv_core220.lib")
#pragma comment(lib, "opencv_highgui220.lib")
#pragma comment(lib, "opencv_legacy220.lib")
#pragma comment(lib, "opencv_ml220.lib")
#pragma comment(lib, "opencv_objdetect220.lib")
#pragma comment(lib, "opencv_features2d220.lib")
#pragma comment(lib, "opencv_flann220.lib")
#pragma comment(lib, "opencv_imgproc220.lib")

HINSTANCE g_hInstance;
HWND g_hDlg;

BOOL bitmap_trancecode(HBITMAP hBitmap , IplImage *ipl_image) {
	BITMAP bmp;
	int x, y;
	RGBQUAD *lpbits;

	GetObject( hBitmap, sizeof(BITMAP) ,&bmp );
	lpbits = (RGBQUAD*)bmp.bmBits;

	//IplImageから画像をコピー
	//IplImageの色数が24bitではない場合や、
	//横ピクセルが4の倍数じゃない場合などは修正が必要なハズ
	for( y=0 ; y<bmp.bmHeight ; y++ ){       
		for( x=0 ; x<bmp.bmWidth ; x++ ){
			lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbRed = ipl_image->imageData[(y*bmp.bmWidth+x)*3];
			lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbGreen = ipl_image->imageData[(y*bmp.bmWidth+x)*3+1];
			lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbBlue = ipl_image->imageData[(y*bmp.bmWidth+x)*3+2];
		}
	}
	return TRUE;
}

bool recognizeCircleFromMemory(HBITMAP hBitmap, BITMAPINFO *pbmi, void **pbits,
	RECT *windowRect, POINT *point)
{
	int width = windowRect->right - windowRect->left;
	int height = windowRect->bottom - windowRect->top;
	IplImage *img = ::cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	IplImage *img_gray = ::cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	trace(L"#########: %d,%d\n", width, height);

	RGBQUAD *lpbits; BITMAP bmp;
	GetObject( hBitmap, sizeof(BITMAP) ,&bmp );
	lpbits = (RGBQUAD*)bmp.bmBits;
	for(int x=0; x<bmp.bmWidth; x++){
		for(int y=0; y<bmp.bmHeight; y++){
			int k = (y * img->widthStep) + (x * 3);
			int j = (bmp.bmHeight-y-1) * bmp.bmWidth + x;

			// 特定の赤色以外はすべて黒色にする
			/*
			img->imageData[k+0] = lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbBlue;
			img->imageData[k+1] = lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbGreen;
			img->imageData[k+2] = lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbRed;
			*/

			if(!(lpbits[j].rgbRed == 255 || lpbits[j].rgbRed == 142)){
				img->imageData[k+0] = (BYTE)255;
				img->imageData[k+1] = (BYTE)255;
				img->imageData[k+2] = (BYTE)255;
			}
			continue;

			if( (lpbits[j].rgbRed == 205 && lpbits[j].rgbGreen == 205 && lpbits[j].rgbBlue == 255) ||
				(lpbits[j].rgbRed == 164 && lpbits[j].rgbGreen == 164 && lpbits[j].rgbBlue == 205) ||
				(lpbits[j].rgbRed == 0 && lpbits[j].rgbGreen == 0 && lpbits[j].rgbBlue == 0) ||
				(lpbits[j].rgbRed == 123 && lpbits[j].rgbGreen == 123 && lpbits[j].rgbBlue == 154) ){
				img->imageData[k+0] = (BYTE)255;
				img->imageData[k+1] = (BYTE)255;
				img->imageData[k+2] = (BYTE)255;
			}else{
				if((lpbits[j].rgbRed == 142) ||
					(lpbits[j].rgbRed == 147 || lpbits[j].rgbGreen == 76 || lpbits[j].rgbBlue == 76 )){
					img->imageData[k+0] = (BYTE)0;
					img->imageData[k+1] = (BYTE)0;
					img->imageData[k+2] = (BYTE)255;
				}else{
					img->imageData[k+0] = (BYTE)lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbBlue;
					img->imageData[k+1] = (BYTE)lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbGreen;
					img->imageData[k+2] = (BYTE)lpbits[(bmp.bmHeight-y-1)*bmp.bmWidth+x].rgbRed;
				}
			}
		}
	}
	trace(L"converted bitmaps\n");
	
	// グレースケール化
	::cvCvtColor(img, img_gray, CV_BGR2GRAY);
	cvThreshold(img_gray, img_gray, 128, 255, CV_THRESH_BINARY);
	//cvThreshold(img_gray, img_gray, 100, 255, CV_THRESH_BINARY);
	
	// (2)ハフ変換のための前処理（画像の平滑化を行なわないと誤検出が発生しやすい）
	cvSmooth(img_gray, img_gray, CV_GAUSSIAN, 11, 11, 0, 0);

	/*
	::cvNamedWindow("w");
	::cvShowImage("w", img_gray);
	::cvWaitKey(0);
	*/

	CvMemStorage *storage = cvCreateMemStorage (0);

	// (3)ハフ変換による円の検出と検出した円の描画
	CvSeq *circles = cvHoughCircles (img_gray, storage, CV_HOUGH_GRADIENT,
		1, // dp
		150, // min_dist
		30, 50, // param1-2
		10, 80); // 検出すべき半径(min - max)
	
	
	// 円が検出出来なかったとき
	if(circles->total == 0){
		cvReleaseImage(&img);
		cvReleaseImage(&img_gray);
		cvReleaseMemStorage(&storage);
		return false;
	}

	if(circles->total >= 2){
		cvReleaseImage(&img);
		cvReleaseImage(&img_gray);
		cvReleaseMemStorage(&storage);
		return false;
	}

	for (int i=0; i<circles->total; i++) {
		float *p = (float *)cvGetSeqElem (circles, i);
		CvPoint pt = cvPoint(cvRound(p[0]), cvRound(p[1]));
		// 最初に検出された円をクリック対象とする
		point->x = pt.x;
		point->y = pt.y;
		break;
	}

	cvReleaseImage(&img);
	cvReleaseImage(&img_gray);
	cvReleaseMemStorage(&storage);
	return true;
}

bool recognizeCircleFromFile(const char *file, const char *dumpfile, POINT *point, bool debug=false)
{
	IplImage *src_img_gray	= cvLoadImage(file, CV_LOAD_IMAGE_GRAYSCALE);
	IplImage *src_img		= cvLoadImage(file, CV_LOAD_IMAGE_COLOR);

	// グレースケール化
	cvThreshold(src_img_gray, src_img_gray, 100, 255, CV_THRESH_BINARY);
	
	// (2)ハフ変換のための前処理（画像の平滑化を行なわないと誤検出が発生しやすい）
	cvSmooth (src_img_gray, src_img_gray, CV_GAUSSIAN, 11, 11, 0, 0);
	CvMemStorage *storage = cvCreateMemStorage (0);

	// (3)ハフ変換による円の検出と検出した円の描画
	CvSeq *circles = cvHoughCircles (src_img_gray, storage, CV_HOUGH_GRADIENT,
		1, // dp
		300, // min_dist
		30, 50, // param1-2
		10, 80); // 検出すべき半径(min - max)

	// 円が検出出来なかったとき
	if(circles->total != 1){
		return false;
	}

	for (int i=0; i<circles->total; i++) {
		float *p = (float *)cvGetSeqElem (circles, i);
		CvPoint pt = cvPoint(cvRound(p[0]), cvRound(p[1]));
		// 最初に検出された円をクリック対象とする
		point->x = pt.x;
		point->y = pt.y;
		break;
	}

	cvReleaseImage(&src_img);
	cvReleaseImage(&src_img_gray);
	cvReleaseMemStorage(&storage);
	return true;
}

BOOL HighlightWindow(HWND hWnd)
{
	HDC hdc = ::GetWindowDC(hWnd);
	if(hdc == NULL){
		return FALSE;
	}

	HPEN hPen = CreatePen(PS_SOLID, 5, RGB(0, 0, 0));
	HBRUSH hBrush = (HBRUSH)::GetStockObject(HOLLOW_BRUSH);

	HGDIOBJ hPrevPen = ::SelectObject(hdc, hPen);
	HGDIOBJ hPrevBrush = ::SelectObject(hdc, hBrush);
	
	RECT rect;
	::GetWindowRect(hWnd, &rect);
	::Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

	::SelectObject(hdc, hPrevPen);
	::SelectObject(hdc, hPrevBrush);

	::DeleteObject(hPen);
	::DeleteObject(hBrush);

	::ReleaseDC(hWnd, hdc);
	return TRUE;
}

bool SetCursor2Circle(HWND h)
{
	// Shootのウインドウ座標を取得します
	RECT windowRect;
	::GetWindowRect(h, &windowRect);
	//::Screenshot::ScreenshotDesktop(L"capture.png", &windowRect);

	// メモリ上にスクリーンショットを撮影します
	void *pbits = NULL;
	BITMAPINFO bmi = {0};
	HBITMAP hBitmap = ::Screenshot::ScreenshotMemoryWindow(::GetDesktopWindow(), &bmi, &pbits, &windowRect);
	POINT point = {0};
	
	if( ::recognizeCircleFromMemory(hBitmap, &bmi, &pbits, &windowRect, &point) ){
		// 成功したときだけ移動
		point.x += windowRect.left;
		point.y += windowRect.top;
		::SetCursorPos(point.x, point.y);
		::mouse_event(MOUSEEVENTF_LEFTDOWN, point.x, point.y, 0, 0);

		::DeleteObject(hBitmap);
		return true;
	}

	::DeleteObject(hBitmap);
	return false;
}

HHOOK g_mouseHook = NULL;
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wp, LPARAM lp)
{
	if( nCode < 0 ) //nCodeが負、HC_NOREMOVEの時は何もしない
		return CallNextHookEx(g_mouseHook, nCode, wp, lp );

	if( nCode == HC_ACTION){
		MSLLHOOKSTRUCT *msg = (MSLLHOOKSTRUCT *)lp;
		if( wp == WM_MOUSEMOVE ){
			::PostMessage(g_hDlg, wp, 0, MAKELPARAM(msg->pt.x, msg->pt.y));
			return CallNextHookEx(::g_mouseHook, nCode, 0, lp);
		}

		if( wp == WM_LBUTTONDOWN || wp == WM_LBUTTONUP ||
			wp == WM_RBUTTONDOWN || wp == WM_RBUTTONUP ){
			::PostMessage(g_hDlg, wp, 0, MAKELPARAM(msg->pt.x, msg->pt.y));
			return TRUE;
		}
	}
	return CallNextHookEx(::g_mouseHook, nCode, 0, lp);
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	static UINT_PTR timer = NULL;
	static HWND prevHighlightHwnd = NULL;
	static BOOL bWindowSelect = FALSE;
	static HWND targetWindow = NULL;
	static int counter = 0;

	switch(msg){
	case WM_INITDIALOG:
		::SetWindowHandle(hDlg);

		KEYINFO keyInfo;
		::QuickSetKeyInfo(&keyInfo, KEY_NOT_SET, VK_ESCAPE); // ESCAPEキーで強制終了
		::RegistKey(keyInfo, WM_USER_MESSAGE);
		if( !::StartHook() ){
			::ShowLastError();
			::PostQuitMessage(0);
		}
		::SetWindowTopMost(hDlg);
		return TRUE;
	case WM_CLOSE:
		::StopHook();
		return TRUE;
	case WM_USER_MESSAGE:
		::StopHook();
		::PostQuitMessage(0);
		return TRUE;
	case WM_TIMER:
		trace(L"WM_TIMER\n");
		if(counter++ > 5000){
			::PostQuitMessage(0);
		}else{
			SetCursor2Circle(targetWindow);
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wp)){
		case IDOK:
			bWindowSelect = TRUE;
			g_mouseHook = ::SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, g_hInstance, 0);
			if(!g_mouseHook){
				::ShowLastError();
				::DestroyWindow(hDlg);
			}
			return TRUE;
		}
		return TRUE;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if(timer != NULL)
			::KillTimer(hDlg, timer);
		if(::g_mouseHook){
			if(!::UnhookWindowsHookEx(::g_mouseHook))
				::ShowLastError();
		}
		bWindowSelect = FALSE;
		//EndDialog(hDlg, LOWORD(wp));
		::PostQuitMessage(0);
		return TRUE;
	case WM_LBUTTONUP:
		if(bWindowSelect){
			if(::g_mouseHook){
				if(!::UnhookWindowsHookEx(::g_mouseHook))
					::ShowLastError();
			}
			::MessageBeep(MB_ICONASTERISK);

			::ReleaseCapture();
			::NoticeRedraw(hDlg);
			::NoticeRedraw(prevHighlightHwnd);
			prevHighlightHwnd = NULL;
			bWindowSelect = FALSE;

			// メイン処理
			targetWindow = ::WindowFromCursorPos();
			::SetCursor2Circle(targetWindow);
			timer = ::SetTimer(hDlg, 0, 180, NULL);
		}
		break;
	case WM_MOUSEMOVE:
		if(bWindowSelect){
			HWND h = ::WindowFromCursorPos();
			::HighlightWindow(h);
			if(h != prevHighlightHwnd)
				::NoticeRedraw(prevHighlightHwnd);
			prevHighlightHwnd = h;
		}
		break;
	}
	return FALSE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine,
	int nCmdShow)
{
	g_hInstance = hInstance;
	g_hDlg = CreateDialog(hInstance, L"IDD_MAIN_DIALOG", NULL, DlgProc);

	MSG msg;
	while(::GetMessage(&msg, g_hDlg, 0, 0)){
		if(msg.message == WM_CLOSE || msg.message == WM_QUIT){
			::PostQuitMessage(0);
			return 0;
		}
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	return 0;
}

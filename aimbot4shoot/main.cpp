#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
//#include <strsafe.h>
// disable warning
#pragma warning(disable : 4996) // ignore "recommend *_s functions warning"
#pragma warning(disable : 4819) // ignore "character code problem warning"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "Util.h"
#include "Screenshot.h"
#pragma comment(lib, "opencv_core220.lib")
#pragma comment(lib, "opencv_highgui220.lib")
#pragma comment(lib, "opencv_legacy220.lib")
#pragma comment(lib, "opencv_ml220.lib")
#pragma comment(lib, "opencv_objdetect220.lib")
#pragma comment(lib, "opencv_features2d220.lib")
#pragma comment(lib, "opencv_flann220.lib")
#pragma comment(lib, "opencv_imgproc220.lib")
#pragma warning(default : 4996)
#pragma warning(default : 4819)

#define TICKTACK 180		// 200msec interval (WM_TIMER)
#define SAFETY_VALVE 5000	// safety valve for this program
#define WM_EMERGENCY_STOP (WM_USER_MESSAGE + 1)
#define CV_DEBUG_WINDOW(IMGOBJ) {::cvNamedWindow("w"); ::cvShowImage("w", IMGOBJ); ::cvWaitKey(0);}

HINSTANCE g_hInstance;
HHOOK g_mouseHook = NULL;
HWND g_hDlg;
UINT_PTR g_timer = NULL;
BOOL bWindowSelect = FALSE;
HWND g_prevHighlightHwnd = NULL;

void ExitDialog(HWND hWnd); // よく使うので宣言

// 指定された画像から"円"を認識し、その中心点の座標(x,y)を返却します
bool recognizeCircleFromMemory(HBITMAP hBitmap, int width, int height, POINT *point)
{
	IplImage *img		= ::cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3); // rgb color array
	IplImage *img_gray	= ::cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1); // gray scale

	RGBQUAD *lpbits; BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP) ,&bmp);
	lpbits = (RGBQUAD*)bmp.bmBits;

	for(int x=0; x<bmp.bmWidth; x++){
		for(int y=0; y<bmp.bmHeight; y++){
			int k = (y * img->widthStep) + (x * 3);
			int j = (bmp.bmHeight-y-1) * bmp.bmWidth + x;

			if(!(lpbits[j].rgbRed == 255 || lpbits[j].rgbRed == 142)){
				img->imageData[k+0] = (BYTE)255;
				img->imageData[k+1] = (BYTE)255;
				img->imageData[k+2] = (BYTE)255;
			}
		}
	}

	// グレースケール化
	::cvCvtColor(img, img_gray, CV_BGR2GRAY);

	// 白と黒の二値化
	::cvThreshold(img_gray, img_gray, 128, 255, CV_THRESH_BINARY);

	// 画像の平滑化(円として認識しやすくするために)
	cvSmooth(img_gray, img_gray, CV_GAUSSIAN, 11, 11, 0, 0);

	// デバッグ表示
	//CV_DEBUG_WINDOW(img_gray);
	
	// Houghアルゴリズムによる円検出
	CvMemStorage *storage = cvCreateMemStorage (0);
	CvSeq *circles = cvHoughCircles (img_gray, storage, CV_HOUGH_GRADIENT,
		1,			// dp
		150,		// min_dist
		30, 50,		// param1-2
		10, 80);	// 検出すべき半径の大きさ(min, max)

	// 円が検出出来なかったとき
	if(circles->total != 1){
		::DeleteObject(hBitmap);
		cvReleaseMemStorage(&storage);
		cvReleaseImage(&img);
		cvReleaseImage(&img_gray);
		return false;
	}

	// 最初に発見された円要素の座標を取得、返却
	for (int i=0; i<circles->total; i++) {
		float *p = (float *)cvGetSeqElem (circles, i);
		CvPoint pt = cvPoint(cvRound(p[0]), cvRound(p[1]));

		// 最初に検出された円をクリック対象とする
		// (先ほどのif条件で実質一つの要素しかここに到達するときにはない)
		point->x = pt.x;
		point->y = pt.y;
		break;
	}

	::DeleteObject(hBitmap);
	cvReleaseMemStorage(&storage);
	cvReleaseImage(&img);
	cvReleaseImage(&img_gray);
	return true;
}

void SetCursorPosAndClick(POINT *point)
{
	::SetCursorPos(point->x, point->y);
	::mouse_event(MOUSEEVENTF_LEFTDOWN, point->x, point->y, 0, 0);
}

// 指定したウインドウ内にある「円っぽいものにマウスカーソルを移動してクリックします」
BOOL SetCursor2CircleAndClick(HWND h)
{
	// Shootのウインドウ座標を取得します
	RECT windowRect;
	::GetWindowRect(h, &windowRect);

	// メモリ上に画像としてキャプチャします
	HBITMAP hBitmap = ::Screenshot::ScreenshotInMemory(::GetDesktopWindow(), &windowRect);
	if(hBitmap == NULL){
		::ShowLastError();
		::ExitDialog(h);
		return FALSE;
	}

	int width	= windowRect.right - windowRect.left;
	int height	= windowRect.bottom - windowRect.top;

	// キャプチャした画像から、円を検出してその場所にマウスを移動して
	// クリックイベントを送出します
	POINT point = {0};
	if( ::recognizeCircleFromMemory(hBitmap, width, height, &point) ){
		::ClientToScreen(h, &point);
		::SetCursorPosAndClick(&point);
		::DeleteObject(hBitmap);
		return TRUE;
	}

	::DeleteObject(hBitmap);
	return FALSE;
}

// 強制終了用のキーフック仕込む関数
void StartEmergencyExitKeyHook(HWND hWnd)
{
	::SetWindowHandle(hWnd);

	KEYINFO keyInfo;
	::QuickSetKeyInfo(&keyInfo, KEY_NOT_SET, VK_ESCAPE);	// ESCAPEキーで強制終了
	::RegistKey(keyInfo, WM_EMERGENCY_STOP);					// 入力を検知したときのメッセージ

	if( !::StartHook() ){
		::ShowLastError();
		::PostQuitMessage(0);
	}
}

// 強制終了用のキーフックを解放する関数
void StopEmergencyExitKeyHook(HWND hWnd)
{
	if( !::StopHook() ){
		::ShowLastError();
		::PostQuitMessage(0);
	}
}

BOOL IsWindowInspectMode()
{
	return ::bWindowSelect;
}

void StartAutoAimTimer(HWND hWnd)
{
	if(g_timer == NULL){
		g_timer = ::SetTimer(hWnd, 0, TICKTACK, NULL);
	}else{
		::ErrorMessageBox(L"すでに実行中です");
	}
}

void StopAutoAimTimer(HWND hWnd)
{
	if(g_timer){
		::KillTimer(hWnd, g_timer);
	}
}

void InitDialog(HWND hWnd)
{
	::SetWindowTopMost(hWnd);
	::StartEmergencyExitKeyHook(hWnd);
}

void ExitDialog(HWND hWnd)
{
	::StopAutoAimTimer(hWnd);
	::StopEmergencyExitKeyHook(hWnd);
	::EndDialog(hWnd, (INT_PTR)TRUE);
	::PostQuitMessage(0);
}

void StartWindowInspect(HWND hWnd)
{
	::bWindowSelect = TRUE;
	if( !::StartMouseEventProxy(hWnd, ::g_hInstance) )
		::ShowLastError();
}

void EndWindowInspect(HWND hWnd)
{
	if(!::StopMouseEventProxy())
		::ShowLastError();

	::NoticeRedraw(hWnd);
	::NoticeRedraw(g_prevHighlightHwnd);
	g_prevHighlightHwnd = NULL;

	bWindowSelect = FALSE;
}

void HighlightCursorPosWindow(HWND hWnd)
{
	if( HWND h = ::WindowFromCursorPos() ){
		::HighlightWindow(h);

		if(h != g_prevHighlightHwnd){
			::NoticeRedraw(g_prevHighlightHwnd);
		}

		g_prevHighlightHwnd = h;
	}else{
		::ShowLastError();
		::ExitDialog(hWnd);
	}
}

void AimBOTInCursorPosWindow()
{
	::SetCursor2CircleAndClick(::WindowFromCursorPos());
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	static int safetyValveCounter = 0;

	switch(msg){
	case WM_INITDIALOG:
		::InitDialog(hDlg);
		return TRUE;

	case WM_CLOSE:
	case WM_EMERGENCY_STOP:
		::ExitDialog(hDlg);
		return TRUE;

	case WM_TIMER: // each TICKTACK(default: each 200ms)
		if(safetyValveCounter++ > SAFETY_VALVE){
			::ExitDialog(hDlg);
		}else{
			::AimBOTInCursorPosWindow();
		}
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wp)){
		case IDOK:
			if( !::IsWindowInspectMode() ) {
				::StartWindowInspect(hDlg);
			}
			return TRUE;
		case IDCANCEL: // if click close button
			::ExitDialog(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_LBUTTONUP:
		if( ::IsWindowInspectMode() ){
			::EndWindowInspect(hDlg);

			::MessageBeep(MB_ICONASTERISK);
			::StartAutoAimTimer(hDlg);
		}
		return TRUE;

	case WM_MOUSEMOVE:
		if( ::IsWindowInspectMode() ){
			::HighlightCursorPosWindow(hDlg);
		}
		return TRUE;
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
	return 0; // not reach
}

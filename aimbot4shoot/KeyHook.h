#pragma once
#include <Windows.h>
#include <tchar.h>
#include <SDKDDKVer.h>

// マクロ定義
#define WM_USER_MESSAGE (WM_USER+0x1000)
#define KEY_NOT_SET -1
#define REGIST_KEY_MAX 256
#define unless(statement) if( !(statement) )

#ifndef DLLIMPORT
	#define DLLIMPORT extern "C" __declspec(dllimport)
#endif

#ifndef DLLEXPORT
	#define DLLEXPORT extern "C" __declspec(dllexport)
#endif

// 構造体定義
typedef struct{
	int ctrlKey;	// CTRL
	int shiftKey;	// SHIFT
	int altKey;		// ALT
	int key;		// a-zなど
	int message;	// message
}KEYINFO;

// 関数定義
DLLEXPORT BOOL StartHook(void);
DLLEXPORT BOOL SetWindowHandle(HWND hWnd);
DLLEXPORT BOOL StopHook(void);
DLLEXPORT BOOL StartKeyHook(int prevKey, int nextKey, int resetKey, int optKey);
DLLEXPORT BOOL RestartHook(void);
DLLEXPORT BOOL isHook(void);
DLLEXPORT int getHookKeys();
DLLEXPORT BOOL RegistKeyCombination(int ctrlKey, int altKey, int shiftKey, int key, int message);
DLLEXPORT BOOL RegistKey(KEYINFO keyInfo, int message);
DLLEXPORT void ClearKeyInfo(KEYINFO *keyInfo);

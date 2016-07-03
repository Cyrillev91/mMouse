// mMouse v0.2c - fix middle mouse button for ASUS laptop that runs windows 10
// ceezblog.info - 29/2/2016
//
// different approach with back / next function, just kill TAB of combo ALT-TAB-LEFT/RIGHT
// Fix disable "Backward / Forward" and "Middle mouse fix"
// Open File Explorer With 3 fingers up gesture


#include <windows.h>
#include <tchar.h>
#include "resource.h"

//Macro for String
#define copyString(a,b)	swprintf(a,L"%s",b)

#define TRAY_ICON_ID			5001
#define SWM_TRAYMSG				WM_APP		//	the message ID sent to our window
#define SM_THREEMOUSE_SWIPE_UP	WM_APP + 6	//	3 fingers swipe up
#define SM_DESTROY				WM_APP + 5	//	hide the window
#define SM_THREEMOUSE_TAP		WM_APP + 4	//	3 fingers tap
#define SM_THREEMOUSE_SWIPE		WM_APP + 3	//	3 fingers swipe - unused
#define SM_ABOUTAPP				WM_APP + 2
#define SM_SEPARATOR			WM_APP + 1	//	SEPARATOR
#define VK_S					83

#define SM_CLOSE				5002
#define DELAY_TIMER_ID			2100
#define DELAY_TIMER_VALUE		30	// wait 30ms for detect software keypress or human keypress.

//Structure used by WH_KEYBOARD_LL
typedef struct KBHOOKSTRUCT {
	DWORD   dwKeyCode;  //they usually use vkCode, for virtual key code
	DWORD   dwScanCode;
	DWORD   dwFlags;
	DWORD   dwTime;
	ULONG_PTR dwExtraInfo;
} KBHOOKSTRUCT;

enum cKeyEvent { KEY_UP, KEY_DOWN };

// Global variables
static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("About mMouse v0.2c");
static TCHAR szTip[] = _T("mMouse - Middle Mouse for windows 10.\r\n\r\n This program provides Middle Mouse function\r\n which ASUS Smart Gesture fails.");

HINSTANCE hInst;
NOTIFYICONDATA	niData;	// Storing notify icon data
HHOOK kbhHook;
HWND hHiddenDialog;

static BOOL ThreeFingerTap = TRUE;
static BOOL ThreeFingerSwipe = TRUE;
static BOOL ThreeFingerSwipeUp = TRUE;
static BOOL SwipeReady = FALSE;
static BOOL SwipeLock = FALSE;

static BOOL LWinDown = FALSE;
static BOOL kill_LWin = FALSE;
static BOOL LAltDown = FALSE;
static BOOL kill_LAlt = FALSE;
static BOOL kill_Tab = FALSE;
static BOOL kill_Leftkey = FALSE;
static BOOL kill_RightKey = FALSE;
static BOOL Kill_SKey = FALSE;
static BOOL passNextKey = FALSE;

static BOOL timerOn = FALSE;
static INT	keyCounter = 0;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
__declspec(dllexport) LRESULT CALLBACK KBHookProc (int, WPARAM, LPARAM);


//////////// FUNCTIONS //////////////

// Pop up menu
void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
		InsertMenu(hMenu, -1, MF_BYPOSITION , SM_ABOUTAPP, L"About mMouse 0.2d...");
		InsertMenu(hMenu, -1, MF_SEPARATOR, WM_APP+3, NULL);
		InsertMenu(hMenu, -1, (ThreeFingerTap)?MF_CHECKED:MF_UNCHECKED , SM_THREEMOUSE_TAP, L"Middle mouse fix");
		InsertMenu(hMenu, -1, (ThreeFingerSwipe)?MF_CHECKED:MF_UNCHECKED , SM_THREEMOUSE_SWIPE, L"Backward / Forward");
		InsertMenu(hMenu, -1, (ThreeFingerSwipeUp) ? MF_CHECKED : MF_UNCHECKED, SM_THREEMOUSE_SWIPE_UP, L"Open File Explorer (3 fingers swipe up)");

		InsertMenu(hMenu, -1, MF_SEPARATOR, SM_SEPARATOR, NULL);
		InsertMenu(hMenu, -1, MF_BYPOSITION, SM_DESTROY, L"Quit");

		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}

void SetTimeOut()
{
	SetTimer(hHiddenDialog, DELAY_TIMER_ID, DELAY_TIMER_VALUE, NULL);
	timerOn = TRUE;
}

void SetTimeOut(int milisec)
{
	SetTimer(hHiddenDialog, DELAY_TIMER_ID, milisec, NULL);
	timerOn = TRUE;
}

void StopTimeOut()
{
	KillTimer(hHiddenDialog, DELAY_TIMER_ID);
	timerOn = FALSE;
}

void sendKey(DWORD vkKey, cKeyEvent keyevent = KEY_DOWN)
{	
	DWORD _keyevent;
	if (keyevent == KEY_UP) _keyevent = KEYEVENTF_KEYUP; 
	else _keyevent = KEYEVENTF_EXTENDEDKEY; 

	passNextKey = TRUE;
	keybd_event(vkKey, MapVirtualKey(vkKey, 0), _keyevent,0 );

	//wait until the LMENU is process by the hook or wait 1s until break out while
	int k=11;
	while (passNextKey || k>1) {Sleep(3); k--;} 

}


// Start point of the program
// Start keyboard hook, init config dialog and notification icon
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex)) return 1;

	hInst = hInstance; // Store instance handle in our global variable
	
	// a dummy hidden dialog to receive the message
	hHiddenDialog = CreateWindow(szWindowClass,	szTitle, WS_TILED|WS_CAPTION|WS_THICKFRAME| WS_MINIMIZEBOX , CW_USEDEFAULT, CW_USEDEFAULT,390, 310, NULL,NULL,hInstance,NULL);
	
	if (!hHiddenDialog)	return 1;

	// add button OK
	HMENU OKButtonID = reinterpret_cast<HMENU>(static_cast<DWORD_PTR>(SM_CLOSE));
	HWND hButton = CreateWindowExW(0, L"Button", L"OK", WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON, 140, 228, 100, 25, hHiddenDialog, OKButtonID, hInst, nullptr);
	SetWindowLongPtr(hButton, GWLP_ID, static_cast<LONG_PTR>(static_cast<DWORD_PTR>(SM_CLOSE)));
		
	// setup the icon
	ZeroMemory(&niData,sizeof(NOTIFYICONDATA));
	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = TRAY_ICON_ID;
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	// load the icon
	niData.hIcon = (HICON)LoadImage(hInst,MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	niData.hWnd = hHiddenDialog;
	niData.uCallbackMessage = SWM_TRAYMSG;

	// tooltip message
	copyString(niData.szTip, szTip);
	Shell_NotifyIcon(NIM_ADD,&niData);

	// free icon handle
	if (niData.hIcon && DestroyIcon(niData.hIcon)) niData.hIcon = NULL;
	
	// setup keyboard hook
	kbhHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) KBHookProc, hInst, NULL);  

	// Reposition the window
	int ScreenX=0;
	int ScreenY=0;
	int WinX=0;
	int WinY=0;
	RECT wnd;
	GetWindowRect(hHiddenDialog, &wnd);
	WinX = wnd.right - wnd.left;
	WinY = wnd.bottom - wnd.top;
	ScreenX = GetSystemMetrics(SM_CXSCREEN);
	ScreenY = GetSystemMetrics(SM_CYSCREEN);
	ScreenX = (ScreenX / 2) - ((int)WinX / 2);
	ScreenY = (ScreenY / 2) - ((int)WinY / 2);
	SetWindowPos(hHiddenDialog,HWND_TOP, ScreenX, ScreenY, (int)WinX,(int)WinY,NULL);
		
	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam; // program end here
}

// Timeout of press and hold ALT and also WIN
void timerTick()
{
	StopTimeOut();
	if (LAltDown)
	{
		kill_Tab = FALSE;
		//kill_LAlt = FALSE; // just kill TAB so Alt - LEFT = back
		//sendKey(VK_LMENU);
		return;
	}
	
	if (LWinDown)
	{
		kill_LWin = FALSE;
		sendKey(VK_LWIN);
		return;
	}
}

// Handle message for the hidden dialog
// Show text on main dialog
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR text1[] = _T("mMouse v0.2d");
	TCHAR text2[] = _T("Touchpad fix for windows 10 Asus laptop, which");
	TCHAR text3[] = _T("ASUS not-so-Smart gesture epically fails for advanced");
	TCHAR text4[] =	_T("user like yourself.");
	TCHAR text5[] = _T("How does it work?");
	TCHAR text6[] = _T("This app will intercept messages that generated by");
	TCHAR text7[] = _T("ASUS not-so-Smart gesture and then send the Middle");
	TCHAR text8[] = _T("mouse button or Backward / Forward button instead");
	TCHAR text0[] = _T("ceezblog.info - 2016");

	TCHAR texta[] = _T("ASUS not-so-Smart gesture");
	TCHAR textb[] = _T("not-so");

	switch (message)
	{
	case WM_KEYDOWN: // same effect as OK = default button
		if (wParam == VK_RETURN || wParam == VK_ESCAPE)
			ShowWindow(hHiddenDialog, SW_HIDE);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		HICON hIcon;

		hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
		DrawIcon(hdc, 10, 5, hIcon);

		SetTextColor(hdc, RGB(30,160,10)); //green-ish
		TextOut(hdc, 50, 15, text1, _tcslen(text1));
		SetTextColor(hdc, RGB(10,20,130)); //blue-ish
		TextOut(hdc, 10, 110, text5, _tcslen(text5));
		SetTextColor(hdc, RGB(10,10,10)); //black-ish
		TextOut(hdc, 10, 45, text2, _tcslen(text2));
		TextOut(hdc, 10, 65, text3, _tcslen(text3));
		TextOut(hdc, 10, 85, text4, _tcslen(text4));
		TextOut(hdc, 10, 130, text6, _tcslen(text6));
		TextOut(hdc, 10, 150, text7, _tcslen(text7));
		TextOut(hdc, 10, 170, text8, _tcslen(text8));
		TextOut(hdc, 220, 195, text0, _tcslen(text0));		
		SetTextColor(hdc, RGB(180,80,80)); //red-ish
		TextOut(hdc, 10, 65, texta, _tcslen(texta));
		TextOut(hdc, 10, 150, texta, _tcslen(texta));
		SetTextColor(hdc, RGB(140,140,140)); //gray-ish
		TextOut(hdc, 50, 65, textb, _tcslen(textb));
		TextOut(hdc, 50, 150, textb, _tcslen(textb));

		EndPaint(hWnd, &ps);
		break;

	case SWM_TRAYMSG:
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hHiddenDialog, SW_SHOW);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
			break;
		}
		break;
	
	case WM_CLOSE:
		ShowWindow(hHiddenDialog, SW_HIDE);
		break;

	case WM_DESTROY:
		UnhookWindowsHookEx(kbhHook);
		PostQuitMessage(0);
		break;
		
	case WM_TIMER:
		timerTick();
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case SM_DESTROY:
			niData.uFlags = 0;
			Shell_NotifyIcon(NIM_DELETE,&niData);
			PostQuitMessage(0);
			break;
		case SM_THREEMOUSE_TAP:
			ThreeFingerTap = !ThreeFingerTap;
			break;
		case SM_THREEMOUSE_SWIPE:
			ThreeFingerSwipe = !ThreeFingerSwipe;
			break;
		case SM_THREEMOUSE_SWIPE_UP:
			ThreeFingerSwipeUp = !ThreeFingerSwipeUp;
			break;
		case SM_ABOUTAPP:
			ShowWindow(hHiddenDialog, SW_SHOW);
			break;
		case SM_CLOSE:
			ShowWindow(hHiddenDialog, SW_HIDE);
			break;
		}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

//Backward button
void sendMBack()
{
	mouse_event(MOUSEEVENTF_XDOWN,0,0,XBUTTON1,NULL);
	mouse_event(MOUSEEVENTF_XUP,0,0,XBUTTON1,NULL);
}

//Forward button
void sendMNext()
{
	mouse_event(MOUSEEVENTF_XDOWN,0,0,XBUTTON2,NULL);
	mouse_event(MOUSEEVENTF_XUP,0,0,XBUTTON2,NULL);
}

//Middle mouse
void sendMMiddle()
{
	mouse_event(MOUSEEVENTF_MIDDLEDOWN,0,0,NULL,NULL);
	mouse_event(MOUSEEVENTF_MIDDLEUP,0,0,NULL,NULL);
}

// Hook process of Keyboard hook
// Process cases of key-press to determine when to send mouse button 3,4,5
// Everything goes here
__declspec(dllexport) LRESULT CALLBACK KBHookProc (int nCode, WPARAM wParam, LPARAM lParam)
{
	KBHOOKSTRUCT kbh = *(KBHOOKSTRUCT*) lParam;

	if (nCode != HC_ACTION)
		return CallNextHookEx(kbhHook, nCode, wParam, (LPARAM)(&kbh));

	if (passNextKey)
	{
		passNextKey = FALSE;
		return CallNextHookEx(kbhHook, nCode, wParam, (LPARAM)(&kbh)); 
	}
	
	if (!ThreeFingerTap && !ThreeFingerSwipe && !ThreeFingerSwipeUp)
		return CallNextHookEx(kbhHook, nCode, wParam, (LPARAM)(&kbh));

	//if (killNextKey)
	//{
	//	killNextKey = FALSE;
	//	return 1;
	//}

	//////////////////////////
	// KEYDOWN / SYSKEYDOWN event
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
	{
		switch (kbh.dwKeyCode)
		{
		// Three finger Swipe
		case VK_LMENU:
			LAltDown = TRUE;
			if (LWinDown) break;
			if (!ThreeFingerSwipe) break;
			
			kill_Tab=TRUE;
			//kill_LAlt = TRUE;
			if (timerOn) StopTimeOut();
			SetTimeOut();
			//return 1; // since use Alt-Left instead of mouse 4 button
			break;

		case VK_TAB:
				if (!ThreeFingerSwipe && !ThreeFingerSwipeUp && !ThreeFingerTap) break;
			// if alt is not press in time or ALT is not held down
			if (LWinDown) //three finger swipe up gesture
			{
				if (ThreeFingerSwipeUp)
				{
					// Open Explorer (LWIN + E)
					sendKey(VK_LWIN);
					sendKey('E');
					sendKey('E', KEY_UP);
					sendKey(VK_LWIN, KEY_UP);
					if (timerOn) { StopTimeOut(); Sleep(10); }
					return 1;
				}
				else
				{
					if (timerOn) { StopTimeOut(); timerTick(); Sleep(10); }
				}
				break;
			}
			if (!LAltDown) break;
			if (!timerOn) break;

			// timer is on, and alt is held down			
			StopTimeOut();
			SwipeReady = TRUE;
			SwipeLock = TRUE; //we got a lock on this
			kill_Tab = TRUE;
			return 1;
			break;

		case VK_LEFT:
			if (LWinDown)
			{
				if (timerOn) {StopTimeOut(); timerTick();Sleep(10);}
				break;
			}
			if (SwipeLock)
			{
				kill_Leftkey = TRUE;
				if (SwipeReady) 
				{
					sendKey(VK_LEFT);
					sendKey(VK_LEFT,KEY_UP);
					SwipeReady = FALSE;
				}
				return 1;
			}
			break;

		case VK_RIGHT:
			if (LWinDown)
			{
				if (timerOn) {StopTimeOut(); timerTick();Sleep(10);}
				break;
			}
			if (SwipeLock)
			{
				kill_RightKey = TRUE;
				if (SwipeReady) 
				{
					sendKey(VK_RIGHT);
					sendKey(VK_RIGHT,KEY_UP);
					SwipeReady = FALSE;
				}
				return 1;
			}
			break;

		// Three finger Tap
		case VK_LWIN:
			if (LAltDown) break;
			if (!ThreeFingerTap && !ThreeFingerSwipeUp) break;

			LWinDown = TRUE;
			if (ThreeFingerTap) { Kill_SKey = FALSE; }
			kill_LWin = TRUE;
			//keyCounter = 1;
			if (timerOn) StopTimeOut();
			SetTimeOut();
			return 1; // kill the key
			break;

		case VK_S:
			if (timerOn && ThreeFingerTap)
			{
				StopTimeOut(); // stop LWIN being fired on timer
				Kill_SKey = TRUE; // we have a match 's'
				sendMMiddle();
				return 1; //kill the key
			}
			else if (kill_LWin)
			{
				// LWIN + S with TreeFigerTap desactivate
				// send LWIN Key
				sendKey(VK_LWIN);
				sendKey('S');
				sendKey('S', KEY_UP);
				sendKey(VK_LWIN, KEY_UP);
				return -1;
			}
			break;
		
		default: //if other key is pressed, just pass it
			if (LWinDown && timerOn) {StopTimeOut(); timerTick();Sleep(10);}
				break; 
		
		}

		// if other keys, just do nothing at all
	}

	//////////////////////////
	// KEYUP / SYSKEYUP event
	if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
	{
		switch (kbh.dwKeyCode)
		{
		// Three finger Swipe
		case VK_LMENU:
			LAltDown = FALSE;
			SwipeLock = FALSE;
			SwipeReady = FALSE;
			if (!ThreeFingerSwipe) break;
			
			if (timerOn)
			{
				StopTimeOut();
				kill_Tab = FALSE;
			}
			break;
			
		case VK_TAB:
			if (kill_Tab) {kill_Tab=FALSE; return 1;}
			break;

		case VK_LEFT:
			if (kill_Leftkey) {kill_Leftkey=FALSE; return 1;}
			break;

		case VK_RIGHT:
			if (kill_RightKey) {kill_RightKey=FALSE; return 1;}
			break;

		// Three finger tap
		case VK_LWIN:
			LWinDown = FALSE;
			if (kill_LWin) {kill_LWin=FALSE; return 1;}
			break;

		case VK_S:
			if (Kill_SKey) {Kill_SKey=FALSE; return 1;}
			break;
		}		
	}

	return CallNextHookEx(kbhHook, nCode, wParam, (LPARAM)(&kbh));
}
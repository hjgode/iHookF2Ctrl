// iHookF2Ctrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "iHookF2Ctrl.h"
#include "hooks.h"

TCHAR szAppName[] = L"iHooF2Ctrl_v0.0.1";

// Global functions: The original Open Source
BOOL g_HookDeactivate();
BOOL g_HookActivate(HINSTANCE hInstance);
HINSTANCE  g_hHookApiDLL    = NULL;         // Handle to loaded library (system DLL where the API is located)

//
void ShowIcon(HWND hWnd, HINSTANCE hInst);
void RemoveIcon(HWND hWnd);

#define MAX_BUFSIZE 256
#define KEYEVENTF_KEYDOWN 0x0000
NOTIFYICONDATA nid;

//global to hold keycodes and commands assigned
typedef struct {
    byte keyFCode;
	//DWORD keyFScanCode; //always 0x0014
    byte keyForCtrl;
	byte scanCode;
} hookmap;

bool bForwardKey=false;
bool bDoCheckIntermec=TRUE;

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
static hookmap kMap[]={ 
	{VK_F5, 'G', 0x34},
	{VK_F6, 'N', 0x31},
	{VK_F7, 'S', 0x1B},
	{VK_F8, 'D', 0x23},
	{VK_F9, 'K', 0x34},
	{VK_F10, 'L', 0x4B},
	{VK_F11, 'B', 0x32},
	{VK_F12, 'C', 0x21},
};
int lastKey=8;


#define MAX_LOADSTRING 100
// Global Variables:
HINSTANCE			g_hInst;			// current instance
HWND				g_hWndCommandBar;	// command bar handle

// Forward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


#pragma data_seg(".HOOKDATA")                                   //  Shared data (memory) among all instances.
    HHOOK g_hInstalledLLKBDhook = NULL;                     // Handle to low-level keyboard hook
    //HWND hWnd = NULL;                                         // If in a DLL, handle to app window receiving WM_USER+n message
#pragma data_seg()

#pragma comment(linker, "/SECTION:.HOOKDATA,RWS")       //linker directive

// The command below tells the OS that this EXE has an export function so we can use the global hook without a DLL
__declspec(dllexport) LRESULT CALLBACK g_LLKeyboardHookCallback(
   int nCode,      // The hook code
   WPARAM wParam,  // The window message (WM_KEYUP, WM_KEYDOWN, etc.)
   LPARAM lParam   // A pointer to a struct with information about the pressed key
) 
{
    /*  typedef struct {
        DWORD vkCode;
        DWORD scanCode;
        DWORD flags;
        DWORD time;
        ULONG_PTR dwExtraInfo;
    } KBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;*/
    
    // Get out of hooks ASAP; no modal dialogs or CPU-intensive processes!
    // UI code really should be elsewhere, but this is just a test/prototype app
    // In my limited testing, HC_ACTION is the only value nCode is ever set to in CE
    static int iActOn = HC_ACTION;
    int i;
    bool processed_key=false;
    if (nCode == iActOn) 
    { 
        PKBDLLHOOKSTRUCT pkbhData = (PKBDLLHOOKSTRUCT)lParam;
        if ( (wParam == WM_KEYUP) && (processed_key==false) )
        {
//            Add2Log(L"# hook got 0x%02x (%i). Looking for match...\r\n", pkbhData->vkCode, pkbhData->vkCode);
			DEBUGMSG(1, (L"# hook got 0x%02x (%i). Looking for match...\r\n", pkbhData->vkCode));
            BOOL bMatchFound=FALSE;
            for (i=0; i<=lastKey; i++) 
            {
                if (pkbhData->vkCode == kMap[i].keyFCode)
                {
                    bMatchFound=TRUE;
                    DEBUGMSG(1, (L"# hook Catched key 0x%0x, send keys '0x%0x'\n", kMap[i].keyFCode, kMap[i].keyForCtrl));
//                    Add2Log(L"# hook Matched key 0x%0x, send keys '%s'\n", kMap[i].keyCode, kMap[i].keyTXT);
					// send Ctrl plus kMap[i]
					keybd_event(VK_LCONTROL, 0x0014, 0, 0);
					keybd_event(kMap[i].keyForCtrl, kMap[i].scanCode, 0, 0);
					keybd_event(kMap[i].keyForCtrl, kMap[i].scanCode, 0 | KEYEVENTF_KEYUP, 0);
					keybd_event(VK_LCONTROL, 0x0014, 0 | KEYEVENTF_KEYUP, 0);
					processed_key=true;
                    DEBUGMSG(1,(L"# hook processed_key is TRUE\r\n"));
					break;
                }
            }
            if(!bMatchFound)
                DEBUGMSG(1,(L"# hook No match found\r\n"));
        }
        else if(wParam == WM_KEYDOWN){
            DEBUGMSG(1,(L"# hook got keydown: %i (0x%x). processed_key is '%i'\r\n", pkbhData->vkCode, pkbhData->vkCode, processed_key));
        }
    }
    //shall we forward processed keys?
    if (processed_key)
    {
        if (bForwardKey){
            DEBUGMSG(1,(L"# hook bForwardKey is TRUE. Resetting processed_key\r\n"));
            processed_key=false; //reset flag
            DEBUGMSG(1,(L"# hook CallNextHookEx() with processed_key=false\r\n"));
            return CallNextHookEx(g_hInstalledLLKBDhook, nCode, wParam, lParam);
        }
        else{
            DEBUGMSG(1,(L"# hook bForwardKey is FALSE. Returning...\r\n"));
            return true;
        }
    }
    else{
        DEBUGMSG(1,(L"# hook CallNextHookEx()\r\n"));
        return CallNextHookEx(g_hInstalledLLKBDhook, nCode, wParam, lParam);
    }
}

BOOL g_HookActivate(HINSTANCE hInstance)
{
    // We manually load these standard Win32 API calls (Microsoft says "unsupported in CE")
    SetWindowsHookEx        = NULL;
    CallNextHookEx          = NULL;
    UnhookWindowsHookEx = NULL;

    // Load the core library. If it's not found, you've got CErious issues :-O
    //TRACE(_T("LoadLibrary(coredll.dll)..."));
    g_hHookApiDLL = LoadLibrary(_T("coredll.dll"));
    if(g_hHookApiDLL == NULL) {
        DEBUGMSG(1,(L"g_HookActivate: LoadLibrary FAILED...\r\n"));
        return false;
    }
    else {
        // Load the SetWindowsHookEx API call (wide-char)
        //TRACE(_T("OK\nGetProcAddress(SetWindowsHookExW)..."));
        SetWindowsHookEx = (_SetWindowsHookExW)GetProcAddress(g_hHookApiDLL, _T("SetWindowsHookExW"));
        if(SetWindowsHookEx == NULL) {
            DEBUGMSG(1,(L"g_HookActivate: GetProcAddress(SetWindowsHookEx) FAILED...\r\n"));
            return false;
        }
        else
        {
            DEBUGMSG(1,(L"g_HookActivate: GetProcAddress(SetWindowsHookEx) OK...\r\n"));
            // Load the hook.  Save the handle to the hook for later destruction.
            //TRACE(_T("OK\nCalling SetWindowsHookEx..."));
            g_hInstalledLLKBDhook = SetWindowsHookEx(WH_KEYBOARD_LL, g_LLKeyboardHookCallback, hInstance, 0);
            if(g_hInstalledLLKBDhook == NULL) {
                DEBUGMSG(1,(L"g_HookActivate: SetWindowsHookEx FAILED...\r\n"));
                return false;
            }
            else
                DEBUGMSG(1,(L"g_HookActivate: SetWindowsHookEx OK...\r\n"));

        }

        // Get pointer to CallNextHookEx()
        //TRACE(_T("OK\nGetProcAddress(CallNextHookEx)..."));
        CallNextHookEx = (_CallNextHookEx)GetProcAddress(g_hHookApiDLL, _T("CallNextHookEx"));
        if(CallNextHookEx == NULL) {
            DEBUGMSG(1,(L"g_HookActivate: GetProcAddress(CallNextHookEx) FAILED...\r\n"));
            return false;
        }
        else
            DEBUGMSG(1,(L"g_HookActivate: GetProcAddress(CallNextHookEx) OK...\r\n"));

        // Get pointer to UnhookWindowsHookEx()
        //TRACE(_T("OK\nGetProcAddress(UnhookWindowsHookEx)..."));
        UnhookWindowsHookEx = (_UnhookWindowsHookEx)GetProcAddress(g_hHookApiDLL, _T("UnhookWindowsHookEx"));
        if(UnhookWindowsHookEx == NULL) {
            DEBUGMSG(1,(L"g_HookActivate: GetProcAddress(UnhookWindowsHookEx) FAILED...\r\n"));
            return false;
        }
        else
            DEBUGMSG(1,(L"g_HookActivate: GetProcAddress(UnhookWindowsHookEx) OK...\r\n"));
    }
    //TRACE(_T("OK\nEverything loaded OK\n"));
    return true;
}


BOOL g_HookDeactivate()
{
    DEBUGMSG(1,(L"g_HookDeactivate()...\r\n"));
    //TRACE(_T("Uninstalling hook..."));
    if(g_hInstalledLLKBDhook != NULL)
    {
        DEBUGMSG(1,(L"\tUnhookWindowsHookEx...\r\n"));
        UnhookWindowsHookEx(g_hInstalledLLKBDhook);     // Note: May not unload immediately because other apps may have me loaded
        g_hInstalledLLKBDhook = NULL;
    }
    else
        DEBUGMSG(1,(L"\tg_hInstalledLLKBDhook is NULL\r\n"));

    DEBUGMSG(1,(L"\tUnloading coredll.dll...\r\n"));
    if(g_hHookApiDLL != NULL)
    {
        FreeLibrary(g_hHookApiDLL);
        g_hHookApiDLL = NULL;
    }
    //TRACE(_T("OK\nEverything unloaded OK\n"));
    DEBUGMSG(1,(L"\tEverything unloaded OK\r\n"));
    return true;
}


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IHOOKF2CTRL));

	g_hInst=hInstance;

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IHOOKF2CTRL));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable


    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_IHOOKF2CTRL, szWindowClass, MAX_LOADSTRING);


    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

    hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    //Notification icon
    DEBUGMSG(1,(L"Adding notification icon\r\n"));
    HICON hIcon;
    hIcon=(HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IHOOK_STARTED), IMAGE_ICON, 16,16,0);
    nid.cbSize = sizeof (NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE;
    // NIF_TIP not supported    
    nid.uCallbackMessage = MYMSG_TASKBARNOTIFY;
    nid.hIcon = hIcon;
    nid.szTip[0] = '\0';
    BOOL res = Shell_NotifyIcon (NIM_ADD, &nid);
    if(!res){
        DEBUGMSG(1 ,(L"Could not add taskbar icon. LastError=%i\r\n", GetLastError() ));
    }else
        DEBUGMSG(1,(L"Taskbar icon added.\r\n"));

//    ShowWindow(hWnd, nCmdShow);
    ShowWindow   (hWnd , SW_HIDE); // nCmdShow) ;  
    UpdateWindow(hWnd);

    if (g_hWndCommandBar)
    {
        CommandBar_Show(g_hWndCommandBar, TRUE);
    }

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

	
    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
            g_hWndCommandBar = CommandBar_Create(g_hInst, hWnd, 1);
            CommandBar_InsertMenubar(g_hWndCommandBar, g_hInst, IDR_MENU, 0);
            CommandBar_AddAdornments(g_hWndCommandBar, 0, 0);

			if (g_HookActivate(g_hInst))
			{
				DEBUGMSG(1,(L"g_HookActivate loaded OK\r\n"));
				MessageBeep(MB_OK);
				//system bar icon
				//ShowIcon(hwnd, g_hInstance);
				DEBUGMSG(1, (L"Hook loaded OK"));
			}
			else
			{
				MessageBeep(MB_ICONEXCLAMATION);
				DEBUGMSG(1,(L"g_HookActivate FAILED. EXIT!\r\n"));
				MessageBox(hWnd, L"Could not hook. Already running a copy of iHookMultikey? Will exit now.", L"iHookMultikey", MB_OK | MB_ICONEXCLAMATION);
				PostQuitMessage(-1);
			}
            break;
		case MYMSG_TASKBARNOTIFY:
				switch (lParam) {
					case WM_LBUTTONUP:
						//ShowWindow(hwnd, SW_SHOWNORMAL);
						SetWindowPos(hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE | SWP_NOREPOSITION | SWP_SHOWWINDOW);
						if (MessageBox(hWnd, L"Hook is loaded. End hooking?", szAppName, 
							MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST)==IDYES)
						{
							g_HookDeactivate();
							Shell_NotifyIcon(NIM_DELETE, &nid);
							PostQuitMessage (0) ; 
						}
						ShowWindow(hWnd, SW_HIDE);
					}
			return 0;
			break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: Add any drawing code here...
            
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            CommandBar_Destroy(g_hWndCommandBar);
			//taskbar system icon
			RemoveIcon(hWnd);
			MessageBeep(MB_OK);
			g_HookDeactivate();

			PostQuitMessage(0);
            break;


        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            RECT rectChild, rectParent;
            int DlgWidth, DlgHeight;	// dialog width and height in pixel units
            int NewPosX, NewPosY;

            // trying to center the About dialog
            if (GetWindowRect(hDlg, &rectChild)) 
            {
                GetClientRect(GetParent(hDlg), &rectParent);
                DlgWidth	= rectChild.right - rectChild.left;
                DlgHeight	= rectChild.bottom - rectChild.top ;
                NewPosX		= (rectParent.right - rectParent.left - DlgWidth) / 2;
                NewPosY		= (rectParent.bottom - rectParent.top - DlgHeight) / 2;
				
                // if the About box is larger than the physical screen 
                if (NewPosX < 0) NewPosX = 0;
                if (NewPosY < 0) NewPosY = 0;
                SetWindowPos(hDlg, 0, NewPosX, NewPosY,
                    0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}

void ShowIcon(HWND hWnd, HINSTANCE hInst)
{
    NOTIFYICONDATA nid;

    int nIconID=1;
    nid.cbSize = sizeof (NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = nIconID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE;   // NIF_TIP not supported
    nid.uCallbackMessage = MYMSG_TASKBARNOTIFY;
    nid.hIcon = (HICON)LoadImage (g_hInst, MAKEINTRESOURCE (ID_ICON1), IMAGE_ICON, 16,16,0);
    nid.szTip[0] = '\0';

    BOOL r = Shell_NotifyIcon (NIM_ADD, &nid);
    if(!r){
        DEBUGMSG(1 ,(L"Could not add taskbar icon. LastError=%i\r\n", GetLastError() ));
    }

}

void RemoveIcon(HWND hWnd)
{
    NOTIFYICONDATA nid;

    memset (&nid, 0, sizeof nid);
    nid.cbSize = sizeof (NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;

    Shell_NotifyIcon (NIM_DELETE, &nid);
    DEBUGMSG(1,(L"Shell_NotifyIcon(NIM_DELETE) done.\r\n"));

}
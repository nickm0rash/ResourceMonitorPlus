// ResourceMonitorPlus.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ResourceMonitorPlus.h"
#include "SystemInfo.h"
#include <string>
#include <sstream>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


SystemInfo si;

std::wstring ConvertToWideString(const std::string& input) {
    std::wstring wideString;
    wideString.reserve(input.length());
    for (char c : input) {
        wideString.push_back(static_cast<wchar_t>(c));
    }
    return wideString;
}


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    //si.PrintRunningProcesses();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RESOURCEMONITORPLUS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RESOURCEMONITORPLUS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
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
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RESOURCEMONITORPLUS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RESOURCEMONITORPLUS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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
   hInst = hInstance; // Store instance handle in our global variable
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   SetTimer(hWnd, 1, 1000, NULL); // 1 second timer

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
		InvalidateRect(hWnd, NULL, TRUE);
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_PRINT_RUNNING_PROCESSES:
				//DEBUGGING FUNCTION
				si.PrintRunningProcesses();
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            si.UpdateMetricsAsync();
            std::wstring ss = 
                L"CPU Usage: " + std::to_wstring(si.GetCpuUsage()) + L"%\n" +
                L"Memory Usage: " + std::to_wstring(si.GetMemoryUsage()) + L"%\n" +
                L"Disk Read: " + std::to_wstring(si.GetDiskReadUsage()) + L" bytes/sec\n" +
                L"Disk Write: " + std::to_wstring(si.GetDiskWriteUsage()) + L" bytes/sec\n" +
                //TODO: Display IP info here.
                L"Network Sent: " + std::to_wstring(si.GetNetworkSentUsage()) + L" bytes/sec\n" +
                L"Network Received: " + std::to_wstring(si.GetNetworkReceivedUsage()) + L" bytes/sec\n" +
                L"Running Processes: ";
            std::vector<std::string> processes = si.GetRunningProcesses();
            std::wstring ipInfo =
                L"Local IPv4: " + ConvertToWideString(si.GetLocalIPv4Address()) + L"\n" +
                L"Local IPv6: " + ConvertToWideString(si.GetLocalIPv6Address()) + L"\n";
            ss += ipInfo;

            for (auto& process : processes) {
				ss += std::wstring(process.begin(), process.end()) + L", ";
			}
            DrawText(
                hdc, 
                ss.c_str(), 
                ss.length(), 
                &ps.rcPaint, 
                DT_LEFT | 
                DT_TOP | 
                DT_WORDBREAK | 
                DT_NOCLIP | 
                DT_EXPANDTABS | 
                DT_EXTERNALLEADING | 
                DT_CALCRECT | 
                DT_NOPREFIX | 
                DT_EDITCONTROL | 
                DT_END_ELLIPSIS | 
                DT_PATH_ELLIPSIS | 
                DT_MODIFYSTRING | 
                DT_RTLREADING | 
                DT_WORD_ELLIPSIS | 
                DT_NOFULLWIDTHCHARBREAK | 
                DT_HIDEPREFIX | 
                DT_PREFIXONLY | 
                DT_TABSTOP
            );
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
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
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#define CLASSNAME L"sjis2hxs"

WNDPROC oldWndProc;
HANDLE hHeap;

void convertAndCopy(HWND wnd)
{
    WCHAR *txtIn;
    WCHAR *txtOut;
    CHAR  *txtMb;
    HGLOBAL hGlob;
    int inLen;
    int mbLen;

    inLen = Edit_GetTextLength(wnd) + 1;
    if(inLen == 1)
        return;

    txtIn = (WCHAR*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, inLen * sizeof(WCHAR));
    Edit_GetText(wnd, txtIn, inLen);
    Edit_SetText(wnd, NULL);

    mbLen = WideCharToMultiByte(
        932,
        0,
        txtIn,
        inLen,
        NULL,
        0,
        NULL,
        NULL);

    if(!mbLen) {
        MessageBox(NULL, TEXT("Couldn't get the mbstring length"), TEXT("FATAL"), MB_ICONSTOP);
        goto out_free_i_str;
    }

    mbLen += 1;
    txtMb = (CHAR*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, mbLen);

    if(!WideCharToMultiByte(
        932,
        0,
        txtIn,
        -1,
        txtMb,
        mbLen,
        NULL,
        NULL)) {
            MessageBox(NULL, TEXT("failed"), TEXT("FATAL"), MB_ICONSTOP);
            goto out_free_mb_str;
    }

    hGlob = GlobalAlloc(GMEM_MOVEABLE, (mbLen * 4 + 2) * sizeof(WCHAR));
    if(hGlob == NULL)
        goto out_free_mb_str;

    txtOut = (WCHAR*)GlobalLock(hGlob);

    {
        int i;
        int n = wsprintfW(txtOut, L"\"");
        WCHAR *tmp = txtOut + n;

        for(i = 0; i < mbLen; ++i) {
            if(!txtMb[i]) break;
            n = wsprintfW(tmp, L"\\x%x", txtMb[i] & 0xFF);
            tmp += n;
        }
        wsprintfW(tmp, L"\"");
    }

    GlobalUnlock(hGlob);
    txtOut[mbLen * 4 + 2] = 0;

    if(!OpenClipboard(NULL))
        goto out_free_glob;

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hGlob);
    CloseClipboard();
    goto out_free_mb_str;

out_free_glob:
    GlobalFree(hGlob);
out_free_mb_str:
    HeapFree(hHeap, 0, txtMb);
out_free_i_str:
    HeapFree(hHeap, 0, txtIn);
}


LRESULT CALLBACK newWndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
    case WM_KEYDOWN:
        switch(wp) {
        case VK_RETURN:
            convertAndCopy(wnd);
            return 0;
        }
    }
    return CallWindowProc(oldWndProc, wnd, msg, wp, lp);
}

LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
    case WM_CREATE:
        {
            HWND hTxt = CreateWindowW(
                L"EDIT",
                NULL,
                ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
                10,
                10,
                270,
                25,
                wnd,
                NULL,
                NULL,
                NULL);

            oldWndProc = (WNDPROC)SetWindowLongPtrW(hTxt, GWLP_WNDPROC, (LONG_PTR)newWndProc);
            return 0;
        }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 1;
    }
    return DefWindowProcW(wnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE pi, LPWSTR lpCmdLn, int nCmdShow)
{



    HWND wnd;
    MSG msg;

    hHeap = GetProcessHeap();

    {
        WNDCLASSEXW wc;
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW ;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hi;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = CLASSNAME;
        wc.hIconSm = NULL;

        RegisterClassExW(&wc);
    }

    wnd = CreateWindowW(
        CLASSNAME,
        CLASSNAME,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        300,
        80,
        NULL,
        NULL,
        hi,
        NULL);

    if(wnd == NULL)
        ExitProcess(-1);

    while(GetMessageW(&msg, wnd, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyWindow(wnd);
    UnregisterClassW(CLASSNAME, hi);

    return msg.wParam;
}
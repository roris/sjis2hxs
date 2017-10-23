/** 
 * Copyright © 2017 roris
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#include <string.h>

#define CLASSNAME L"sjis2hxs"

WNDPROC oldWndProc;
HANDLE hHeap;
HWND hStat;

void convertAndCopy(HWND wnd)
{
    int len, size;
    WCHAR   *inStr;
    WCHAR   *outStr;
    HGLOBAL clipStr;
    CHAR    *mbStr;

    // get the string from the text box
    len = GetWindowTextLengthW(wnd);
    if(!len) return;

    inStr = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (len + 1) * sizeof(WCHAR));
    // if(!inStr) return; // Just crash :)
    GetWindowTextW(wnd, inStr, len + 1);

    size = WideCharToMultiByte(
        932,
        0,
        inStr,
        -1,
        NULL,
        0,
        NULL,
        NULL);

    if(!size) goto free_instr;
    mbStr = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size);
    len = size - 1;

    if(!WideCharToMultiByte(
        932,
        0,
        inStr,
        -1,
        mbStr,
        size,
        NULL,
        NULL)) {
            goto free_mbstr;
    }

    // convert it back into unicode
    {
        size = MultiByteToWideChar(
            932,
            MB_ERR_INVALID_CHARS,
            mbStr,
            -1,
            NULL,
            0);
        if(!size) goto free_mbstr;

        outStr = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size * sizeof(WCHAR));

        if(!MultiByteToWideChar(
            932,
            MB_ERR_INVALID_CHARS,
            mbStr,
            -1,
            outStr,
            size))
            goto free_mbstr;

        SetWindowTextW(hStat, outStr);
        HeapFree(hHeap, 0, outStr);
    }

    clipStr = GlobalAlloc(GMEM_MOVEABLE, (len * 4 + 3) * sizeof(WCHAR));


    {
        WCHAR *tmp;
        int i, j;

        tmp = GlobalLock(clipStr);
        j = wsprintfW(tmp, L"\"");
        tmp += j;

        for(i = 0; i < len; ++i) {
            j = wsprintfW(tmp, L"\\x%x", 0xFF & mbStr[i]);
            tmp += j;
        }

        wsprintfW(tmp, L"\"");
        GlobalUnlock(clipStr);
    }

    if(!OpenClipboard(NULL))
        goto free_global;

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, clipStr);
    CloseClipboard();
    goto free_mbstr;

free_global:
    GlobalFree(clipStr);
free_mbstr:
    HeapFree(hHeap, 0, mbStr);
free_instr:
    HeapFree(hHeap, 0, inStr);
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
    return CallWindowProcW(oldWndProc, wnd, msg, wp, lp);
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
                470,
                20,
                wnd,
                NULL,
                NULL,
                NULL);

            SendMessageW(hTxt, EM_LIMITTEXT, 80, 0);
            oldWndProc = (WNDPROC)SetWindowLongPtrW(hTxt, GWLP_WNDPROC, (LONG_PTR)newWndProc);

            hStat = CreateWindowW(
                L"STATIC",
                NULL,
                WS_CHILD | WS_VISIBLE,
                10,
                40,
                470,
                50,
                wnd,
                NULL,
                NULL,
                NULL);

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

    UNREFERENCED_PARAMETER(pi);
    UNREFERENCED_PARAMETER(lpCmdLn);
    UNREFERENCED_PARAMETER(nCmdShow);

    hHeap = GetProcessHeap();

    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(WNDCLASSEXW));
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW ;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hi;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        wc.lpszClassName = CLASSNAME;

        RegisterClassExW(&wc);
    }

    wnd = CreateWindowW(
        CLASSNAME,
        CLASSNAME,
        (WS_VISIBLE | WS_SYSMENU),
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        120,
        NULL,
        NULL,
        hi,
        NULL);

    if(wnd == NULL)
        ExitProcess(0xFFFFFFFF);

    while(GetMessageW(&msg, wnd, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyWindow(wnd);
    UnregisterClassW(CLASSNAME, hi);

    return msg.wParam;
}

#pragma once

#ifdef	ComCtl6
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif //ComCtl6

#define HANDLE_DLG_MSG(hwnd, message, fn) \
   case (message): \
   return (SetDlgMsgResult(hwnd, \
							message, \
							HANDLE_##message((hwnd), (wParam), (lParam), (fn))))

#define HANDLE_DLG_MSG_RET(hwnd, message, fn, ret) \
	case (message): \
		HANDLE_##message((hwnd), (wParam), (lParam), (fn)); \
		return (SetDlgMsgResult(hwnd, message, ret));

#define HANDLE_DLG_MSG_DERIVED(hwnd, message, fn, base) \
	case (message): { \
		INT_PTR ret = HANDLE_##message((hwnd), (wParam), (lParam), (fn)); \
		(base); \
		return (SetDlgMsgResult(hwnd, message, ret)); }

//	Uncommented on 2.6.18 for ClipboardToWord CustomDialog.cpp
/* LRESULT Cls_OnNotify(HWND hwnd, int id, NMHDR * pNMHDR) */
#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(wParam), (NMHDR *)(lParam)), 0L)

/* LRESULT Cls_OnClipboardUpdate(HWND hwnd) */
#define HANDLE_WM_CLIPBOARDUPDATE(hwnd, wParam, lParam, fn) \
	((fn)(hwnd), 0L)
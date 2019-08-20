#pragma once

#include <Windows.h>

#include <vector>
#include <algorithm>

#define WM_USER_NEWCHILDDIALOG		(WM_USER + 10)
#define WM_USER_REMOVECHILDDIALOG	(WM_USER + 11)

template <typename T>
class BasicWindow {
	
public:

	WNDCLASS wc{ 0 };
	WCHAR * szWindowName = nullptr;

    HWND	m_window = nullptr;

    static T * GetThisFromHandle(HWND const window) {
        return reinterpret_cast<T *>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) {
        
		if (WM_NCCREATE == message) {

            CREATESTRUCT * cs = reinterpret_cast<CREATESTRUCT *>(lparam);
            T * that = static_cast<T *>(cs->lpCreateParams);

            that->m_window = window;

            SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));

			return that->MessageHandler(message, wparam, lparam);

        } else if (T * that = GetThisFromHandle(window)) {

            return that->MessageHandler(message, wparam, lparam);

        }

		return DefWindowProc(window, message, wparam, lparam);
	}

	virtual LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) = 0;
};

template <typename T>
class ApplicationWindow : public BasicWindow<T> {

	std::vector<HWND> m_childdialogwindows;

	BOOL HandleDialogMessage(MSG * pmsg) {

		BOOL ret = FALSE;
		
		if( m_childdialogwindows.size() == 0)
			return ret;


		for( HWND hwndChild : m_childdialogwindows ) {
			ret = IsDialogMessage(hwndChild, pmsg);
			if( ret ) break;
		}

		return ret;

	}

protected:

	HACCEL	m_accel = nullptr;

public:

	using BasicWindow<T>::m_window;

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override {

		if( WM_DESTROY == message ) {

			PostQuitMessage(0);

		} else if( WM_USER_NEWCHILDDIALOG == message ) {

			HWND childWindow = (HWND) lparam;
			m_childdialogwindows.push_back(childWindow);

		} else if( WM_USER_REMOVECHILDDIALOG == message) {

			HWND childWindow = (HWND) lparam;
			m_childdialogwindows.erase(
				std::remove_if(std::begin(m_childdialogwindows), std::end(m_childdialogwindows), 
					[childWindow](HWND hwnd) -> bool {
						return ( hwnd == childWindow );
					}),
				std::end(m_childdialogwindows)
			);

		} else {
			return DefWindowProc(m_window, message, wparam, lparam);

		}

		return 0;
	}

	void run() {
		MSG msg;
		while( GetMessage(&msg, nullptr, 0, 0) ) {
			if( !TranslateAccelerator(m_window, m_accel, &msg) && !HandleDialogMessage(&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

	}
};

template <typename T>
struct ChildWindow : public BasicWindow<T> {

	using BasicWindow<T>::wc;
	using BasicWindow<T>::m_window;
	using BasicWindow<T>::szWindowName;

	virtual ~ChildWindow() { DestroyWindow(m_window); }

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override {

		return DefWindowProc(m_window, message, wparam, lparam);
	}

	HWND CreateChildWindow(HWND hwndParent, bool visible = false) {

		auto hwnd = CreateWindowEx(0, wc.lpszClassName, szWindowName,
			WS_CHILD, 0, 0, 0, 0, hwndParent, nullptr, wc.hInstance, this);

		if( visible )
			ShowWindow(hwnd, SW_SHOW);

		UpdateWindow(hwnd);

		return hwnd;

	}
};

template <typename T>
struct BasicDialog {

	HWND	m_window = nullptr;

	HINSTANCE		m_hInstance{ NULL };
	const WCHAR	*	m_szTemplate{ NULL };

	HWND			m_hwndParent = NULL;

	static T * GetThisFromHandle(HWND const window) {
		return reinterpret_cast<T *>( GetWindowLongPtr(window, GWLP_USERDATA) );
	}	

	virtual INT_PTR ShowDialogBox(HWND hwndParent) = 0;

	static INT_PTR __stdcall DlgProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) {

		if( WM_INITDIALOG == message ) {

			T * that = reinterpret_cast<T *>( lparam );

			that->m_window = window;

			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( that ));

			return that->MessageHandler(message, wparam, lparam);

		} else if( T * that = GetThisFromHandle(window) ) {

			return that->MessageHandler(message, wparam, lparam);

		}

		return FALSE;
	}

	virtual INT_PTR MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) = 0;

};

template <typename T>
struct ModalDialog : public BasicDialog<T> {

	using BasicDialog<T>::m_hInstance;
	using BasicDialog<T>::m_szTemplate;
	using BasicDialog<T>::m_hwndParent;
	using BasicDialog<T>::m_window;

	INT_PTR ShowDialogBox(HWND hwndParent) override {

		m_hwndParent = hwndParent;

		return DialogBoxParam(m_hInstance, m_szTemplate,
			m_hwndParent, BasicDialog<T>::DlgProc, ( LPARAM ) this);

	}

	INT_PTR MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) override {

		if( message == WM_COMMAND ) {

			if( LOWORD(wParam) == IDCANCEL ) {
				EndDialog(m_window, 0);
			}

			return TRUE;

		} else if( message == WM_INITDIALOG ) {

			return TRUE;
		}

		return FALSE;
	}

};

template <typename T>
struct ModelessDialog : public BasicDialog<T> {

	using BasicDialog<T>::m_hInstance;
	using BasicDialog<T>::m_window;
	using BasicDialog<T>::m_szTemplate;
	using BasicDialog<T>::m_hwndParent;

	INT_PTR ShowDialogBox(HWND hwndParent) override {

		m_hwndParent = hwndParent;

		auto hwnd = CreateDialogParam(m_hInstance, m_szTemplate,
			NULL, BasicDialog<T>::DlgProc, ( LPARAM ) this);

		UpdateWindow(m_window);
		ShowWindow(m_window, SW_SHOW);

		return (INT_PTR) hwnd;
	}
};

template <typename T>
struct ChildModelessDialog : public ModelessDialog<T> {

	using BasicDialog<T>::m_hInstance;
	using BasicDialog<T>::m_window;
	using BasicDialog<T>::m_szTemplate;
	using BasicDialog<T>::m_hwndParent;

	INT_PTR MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) override {

		if( message == WM_CLOSE ) {

			SendMessage(m_hwndParent, WM_USER_REMOVECHILDDIALOG, 0, (LPARAM) m_window);
			DestroyWindow(m_window);

			return TRUE;

		} else if( message == WM_COMMAND ) {

			if( LOWORD(wParam) == IDCANCEL ) {
				SendMessage(m_window, WM_CLOSE, 0, 0);
				return TRUE;

			}

			return FALSE;

		} else if( message == WM_INITDIALOG ) {
			
			SendMessage(m_hwndParent, WM_USER_NEWCHILDDIALOG, 0, (LPARAM) m_window);

			return TRUE;
		}

		return FALSE;
	}

};

template <typename T>
struct ApplicationModelessDialog : public ModelessDialog<T> {

	using BasicDialog<T>::m_hInstance;
	using BasicDialog<T>::m_window;
	using BasicDialog<T>::m_szTemplate;
	using BasicDialog<T>::m_hwndParent;

	HACCEL	m_accel = nullptr;

	INT_PTR MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) override {

		if( message == WM_COMMAND ) {

			if( LOWORD(wParam) == IDCANCEL ) {
				DestroyWindow(m_window);
			}

			return TRUE;

		} else if( message == WM_DESTROY ) {

			PostQuitMessage(0);

			return TRUE;

		} else if( message == WM_INITDIALOG ) {

			return TRUE;
		}

		return FALSE;
	}

	void run() {

		MSG msg;

		while( GetMessage(&msg, nullptr, 0, 0) ) {

			if( !TranslateAccelerator(m_window, m_accel, &msg) && !IsDialogMessage(m_window, &msg) ) {

				TranslateMessage(&msg);
				DispatchMessage(&msg);

			}
		}
	}

};
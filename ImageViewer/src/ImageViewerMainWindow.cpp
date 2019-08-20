#include <Windows.h>
#include <windowsx.h>

#include <Ole2.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlalloc.h>

#include <string>
using std::wstring;
#include <cctype>

#include <string.h>

#define ComCtl6
#include <CommonHeaders.h>

#include "ImageViewerMainWindow.h"
#include "Utility.h"

#include "resource.h"

ImageViewerMainWindow::ImageViewerMainWindow(const WCHAR * szCmdLine) {

	if( wcslen(szCmdLine) < 1 ) {
		PostQuitMessage(0);
		return;
	}

	this->m_window;

	auto hr = OleInitialize(NULL);

	hr = m_spWindows.CoCreateInstance(CLSID_ShellWindows);

	m_pApplicationSettings = std::make_unique<ApplicationSettings>();

	wc.hbrBackground = GetStockBrush(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = (HINSTANCE) GetModuleHandle(nullptr);
	wc.lpfnWndProc = BasicWindow::WndProc;
	wc.lpszClassName = L"ImageViewerMainWindow";
	wc.style = CS_VREDRAW | CS_HREDRAW;

	auto ret = RegisterClass(&wc);

	m_accel = LoadAccelerators(wc.hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	auto hwnd = CreateWindow(wc.lpszClassName, L"Image Viewer", WS_OVERLAPPEDWINDOW,
		m_pApplicationSettings->x, m_pApplicationSettings->y,
		m_pApplicationSettings->width, m_pApplicationSettings->height,
		NULL, NULL, wc.hInstance, this);

	wstring cmdline = SanitizeCommandLine(szCmdLine);
	DisplayImage(cmdline.c_str());
}

LRESULT ImageViewerMainWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	switch( message ) {

		HANDLE_MSG(m_window, WM_CREATE, Cls_OnCreate);
		HANDLE_MSG(m_window, WM_SIZE, Cls_OnSize);
		HANDLE_MSG(m_window, WM_RBUTTONDOWN, Cls_OnRButtonDown);
		HANDLE_MSG(m_window, WM_LBUTTONDOWN, Cls_OnLButtonDown);
		HANDLE_MSG(m_window, WM_TIMER, Cls_OnTimer);
		HANDLE_MSG(m_window, WM_MOUSEMOVE, Cls_OnMouseMove);
		HANDLE_MSG(m_window, WM_MOUSEWHEEL, Cls_OnMouseWheel);
		HANDLE_MSG(m_window, WM_COMMAND, Cls_OnCommand);

	case WM_DESTROY:	// Let parent class also handle WM_DESTROY.
		
		RECT rect;
		GetWindowRect(m_window, &rect);
		
		m_pApplicationSettings->x = rect.left;
		m_pApplicationSettings->y = rect.top;

		m_pApplicationSettings->width = rect.right - rect.left;
		m_pApplicationSettings->height = rect.bottom - rect.top;

		break;

	case WM_CONTEXTACTION: {
		auto szAction = (const WCHAR *) lParam;

		if( wcscmp(szAction, L"Next") == 0 ) {
			NextImage();
		} else if( wcscmp(szAction, L"Previous") == 0 ) {
			PreviousImage();
		} else if( wcscmp(szAction, L"Open Folder") == 0 ) {
			OpenExplorer();
		} else if( wcscmp(szAction, L"Rotate") == 0 ) {
			Rotate();
		} else if( wcscmp(szAction, L"Slideshow") == 0 ) {
			ToggleSlideshow();
		} else if( wcscmp(szAction, L"Actual Size") == 0 ) {
			ToggleScale();
		}

		break; }

	case WM_USER:
		
		m_index = (UINT) wParam;

		DisplayCurrentIndex();

		break;

	case IVMW_RESIZEPREVIEW: {

		int cx = LOWORD(wParam);
		int cy = HIWORD(wParam);

		MoveWindow(m_pPreviewWindow->m_window, 0, cy - PreviewWindow::HEIGHT,
			cx, PreviewWindow::HEIGHT, TRUE);

		m_pPreviewWindow->HidePreviewWindow();

		break; }
	}

	auto ret = __super::MessageHandler(message, wParam, lParam);
	return ret;
}

BOOL ImageViewerMainWindow::Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {

	m_pImageWindow = std::make_unique<ImageWindow>();

	m_pImageWindow->InitWindow(0, 0, nullptr, hwnd);
	ShowWindow(m_pImageWindow->m_window, SW_SHOW);
	
	const WCHAR * buttons[] = { L"Next", L"Previous", L"Open Folder", L"Actual Size", L"Rotate", L"Slideshow", };
	m_pContextWindow =
		std::make_unique<ContextWindow>(101, buttons);
	m_pContextWindow->InitWindow(150, 150, nullptr, hwnd);

	m_pPreviewWindow = std::make_unique<PreviewWindow>(L"ImageViewerPreviewWindowClass", 102);
	m_pPreviewWindow->InitWindow(lpCreateStruct->cx, m_pPreviewWindow->SIDE + 2 * m_pPreviewWindow->PAD, nullptr, hwnd);

	return TRUE;
}

void ImageViewerMainWindow::Cls_OnSize(HWND hwnd, UINT state, int cx, int cy) {

	m_pContextWindow->HideContextWindow();

	MoveWindow(m_pImageWindow->m_window, 5, 5, cx - 10, cy - 10, TRUE);

	m_timerPreviewResize = SetTimer(m_window, previewResizeId, 800, nullptr);
}

void ImageViewerMainWindow::Cls_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) {

	m_pContextWindow->ShowContextWindow(x, y);
}

void ImageViewerMainWindow::Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) {

	m_pContextWindow->HideContextWindow();
}

void ImageViewerMainWindow::Cls_OnTimer(HWND hwnd, UINT id) {

	switch( id ) {
	
	case ImageViewerMainWindow::slideshowId:

		if( !NextImage() ) {
			m_index = 0;
			DisplayCurrentIndex();
		}

		return;

	case ImageViewerMainWindow::previewResizeId:

		KillTimer(m_window, m_timerPreviewResize);
		RECT rect;
		GetClientRect(m_window, &rect);
		int cx = rect.right;
		int cy = rect.bottom;
		PostMessage(m_window, IVMW_RESIZEPREVIEW, MAKEWPARAM(cx, cy), 0);

		return;

	}
}

void ImageViewerMainWindow::Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags) {

#ifdef _DEBUG
	static wstring out;
	
	out = L"x: ";
	out += std::to_wstring(x);
	out += L", y:";
	out += std::to_wstring(y);

	OutputDebugString(out.c_str());
	OutputDebugString(L"\n");
#endif //_DEBUG

	RECT rect;
	GetClientRect(m_window, &rect);

	if( y >= rect.bottom - PreviewWindow::HEIGHT )
		m_pPreviewWindow->ShowPreviewWindow();
	else
		m_pPreviewWindow->HidePreviewWindow();
}

void ImageViewerMainWindow::Cls_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys) {

	if( zDelta > 0 )
		PreviousImage();
	else
		NextImage();
}

void ImageViewerMainWindow::Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {

	switch( id ) {

	case ID_ACC_ROTATE:
		Rotate();
		break;

	case ID_ACC_ZOOMIN:
		ChangeZoom(1);
		break;

	case ID_ACC_ZOOMOUT:
		ChangeZoom(-1);
		break;

	case ID_ACC_SIZE:
		ToggleScale();
		break;

	case ID_ACC_COPY:
		CopyImageToClipboard();
		break;

	case ID_ACC_RESET:
		m_pImageWindow->Reset();
		break;

	case ID_ACC_NEXT:
		NextImage();
		break;

	case ID_ACC_PREVIOUS:
		PreviousImage();
		break;

	case ID_ACC_SCROLLUP:
	case ID_ACC_SCROLLDOWN:
	case ID_ACC_SCROLLLEFT:
	case ID_ACC_SCROLLRIGHT: {

		int dx = 0;
		int dy = 0;

		if( id == ID_ACC_SCROLLUP )		dy = -10;
		if( id == ID_ACC_SCROLLDOWN )	dy = 10;

		if( id == ID_ACC_SCROLLLEFT )	dx = -10;
		if( id == ID_ACC_SCROLLRIGHT )	dx = 10;

		m_pImageWindow->Scroll(dx, dy);

		break; }
	}
}

void ImageViewerMainWindow::DisplayImage(const WCHAR * szImagePath) {

	if( !szImagePath )	return;

	if( !m_pImageWindow->DisplayImage(szImagePath) )	return;

	wstring caption{ L"Image Viewer: " };
	caption += szImagePath;
	SetWindowText(m_window, caption.c_str());

	m_current_image_name = GetFilenameFromPath(szImagePath);
	auto dir = GetDirFromPath(szImagePath);
	if( dir != m_current_dir ) {
		m_current_dir = dir;
		CacheImagePaths();
	}
}

template <typename character>
bool char_compare_ignore_case(character c1, character c2) {

	return std::tolower(c1) == std::tolower(c2);
}

template <typename strClass>
bool compareStringsIgnoreCase(const strClass& str1, const strClass& str2) {

	if( str1.size() != str2.size() )	return false;

	auto str1_end = str1.cend();
	auto str1_iter = str1.cbegin();
	auto str2_iter = str2.cbegin();
	for( ; str1_iter != str1_end; 
		++str1_iter, ++str2_iter ) {

		bool result = char_compare_ignore_case(*str1_iter, *str2_iter);
		if( result == false )	return false;
	}

	return true;
}

void ImageViewerMainWindow::CacheImagePaths() {	

	CComPtr<IFolderView2> pfv;
	auto res = GetFolderViewFromPath(m_current_dir.c_str(), &pfv);
	if( res && pfv ) {

		int count, index;
		auto hr = pfv->ItemCount(SVGIO_ALLVIEW, &count);

		for( index = 0; index < count; ++index ) {

			CComPtr<IShellItem> p_item;
			hr = pfv->GetItem(index, IID_PPV_ARGS(&p_item));
			if( S_OK != hr ) continue;

			CComHeapPtr<WCHAR> pszPath;
			hr = p_item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszPath);
			if( S_OK == hr ) {
				auto filename = GetFilenameFromPath(pszPath);
				auto extension = GetExtensionFromFilename(filename.c_str());
				if( compareStringsIgnoreCase(extension, std::wstring{ L"jpg" }) ) {
					m_image_filename_cache.push_back(filename);
					if( filename == m_current_image_name ) {
						m_index = m_image_filename_cache.size() - 1;
						m_pPreviewWindow->SetDisplay(m_index);
					}
				}

				
			}
		}

		//	Alert PreviewWindow
		PostMessage(m_pPreviewWindow->m_window, PV_SETPREVIEWCACHE, (WPARAM) &m_current_dir, (LPARAM) &m_image_filename_cache);
	}
}

bool ImageViewerMainWindow::NextImage() {
		
	auto size = m_image_filename_cache.size();
	if( size == 0 ) return false;

	if( m_index < size - 1 ) {
		++m_index;
		DisplayCurrentIndex();
		m_pPreviewWindow->Increment();
		return true;
	}

	return false;
}

bool ImageViewerMainWindow::PreviousImage() {

	auto size = m_image_filename_cache.size();
	if( size == 0 ) return false;

	if( m_index > 0 ) {
		--m_index;
		DisplayCurrentIndex();
		m_pPreviewWindow->Decrement();
		return true;
	}

	return false;
}

void ImageViewerMainWindow::DisplayCurrentIndex() {

	if( m_image_filename_cache.size() < 1 )
		return;

	m_angle = 0.0f;

	wstring img_path = m_current_dir + L"\\";
	img_path += m_image_filename_cache.at(m_index);

	DisplayImage(img_path.c_str());

	m_pPreviewWindow->SetDisplay(m_index);
}

void ImageViewerMainWindow::OpenExplorer() {

	ShellExecute(NULL, L"open", m_current_dir.c_str(), nullptr, nullptr, SW_SHOW);

	Sleep(1000);

	auto size = m_image_filename_cache.size();
	if( size == 0 )
		CacheImagePaths();
}

void ImageViewerMainWindow::Rotate() {

	m_angle += 90.0f;
	if( m_angle == 360.0f) m_angle = 0.0f;
	m_pImageWindow->Rotate(m_angle);

}

bool ImageViewerMainWindow::GetFolderViewFromPath(const WCHAR * szPath, IFolderView2 ** ppfv) {

	if( !m_spWindows )	return false;
	if( !szPath )		return false;
	if( !ppfv )			return false;

	*ppfv = nullptr;

	CComPtr<IUnknown> spunkEnum;
	HRESULT hr = m_spWindows->_NewEnum(&spunkEnum);
	if( S_OK != hr )	return false;

	CComQIPtr<IEnumVARIANT> spev(spunkEnum);
	for( CComVariant svar; spev->Next(1, &svar, nullptr) == S_OK; svar.Clear() ) {

		if( svar.vt != VT_DISPATCH ) continue;
		CComPtr<IShellBrowser> spsb;
		hr = IUnknown_QueryService(svar.pdispVal, SID_STopLevelBrowser, IID_PPV_ARGS(&spsb));
		if( S_OK != hr )	continue;

		CComPtr<IShellView> spsv;
		hr = spsb->QueryActiveShellView(&spsv);
		if( S_OK != hr )	continue;

		CComQIPtr<IPersistIDList> sppidl(spsv);
		if( !sppidl )		continue;

		CComHeapPtr<ITEMIDLIST_ABSOLUTE> spidl;
		hr = sppidl->GetIDList(&spidl);
		if( S_OK != hr )	continue;

		CComPtr<IShellItem> spsi;
		hr = SHCreateItemFromIDList(spidl, IID_PPV_ARGS(&spsi));
		if( S_OK != hr )	continue;

		CComHeapPtr<WCHAR> pszLocation;
		hr = spsi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszLocation);
		if( S_OK != hr )	continue;

		if( wcscmp(pszLocation, szPath) != 0 )	continue;

		hr = spsv->QueryInterface(IID_PPV_ARGS(ppfv));

		if( hr != S_OK )	continue;

		return true;
	}

	return false;
}

void ImageViewerMainWindow::ToggleSlideshow() {

	if( m_image_filename_cache.size() == 0 )
		return;

	m_fSlideshow = !m_fSlideshow;

	if( m_fSlideshow ) {

		m_timer = SetTimer(m_window, slideshowId, m_slideshow_delay, nullptr);

	} else {

		KillTimer(m_window, m_timer);
		m_timer = 0;
	}
}

void ImageViewerMainWindow::ToggleScale() {

	m_pImageWindow->ToggleScale();
}

void ImageViewerMainWindow::ChangeZoom(int direction) {

	m_pImageWindow->ChangeZoom(direction);
}

void ImageViewerMainWindow::CopyImageToClipboard() {


}
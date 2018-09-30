#include <Windows.h>
#include <windowsx.h>

#include <CommonHeaders.h>

#include "PreviewWindow.h"

void PreviewWindow::CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) {
		
	m_HighlightVisual = CreateVisual();
	
	auto surface = CreateSurface(m_height, m_height);
	ComPtr<ID2D1DeviceContext> dc;
	POINT offset = {};
	auto hr = surface->BeginDraw(nullptr, IID_PPV_ARGS(dc.GetAddressOf()), &offset);
	
	dc->SetDpi(m_dpiX, m_dpiY);
	dc->SetTransform(D2D1::Matrix3x2F::Translation(offset.x * 96 / m_dpiX, offset.y * 96 / m_dpiY));
	dc->Clear(D2D1::ColorF(D2D1::ColorF::Red));

	hr = surface->EndDraw();

	hr = m_HighlightVisual->SetContent(surface.Get());

	hr = m_rootVisual->AddVisual(m_HighlightVisual.Get(), FALSE, nullptr);
}

void PreviewWindow::ReleaseDeviceResources() {

}

void PreviewWindow::RenderWindow() {

	if( m_display >= 0 ) {
		
		int index = m_display - m_base;
		
		m_HighlightVisual->SetOffsetX(index * ( SIDE + PAD ));
	}
}


LRESULT PreviewWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	switch( message ) {

	case WM_CREATE:
		Cls_OnCreate(m_window, (LPCREATESTRUCT) lParam);
		break;

	case WM_SIZE:
		Cls_OnSize(m_window, (UINT) wParam, LOWORD(lParam), HIWORD(lParam));
		break;

		HANDLE_MSG(m_window, WM_LBUTTONDOWN, Cls_OnLButtonDown);
	}

	return __super::MessageHandler(message, wParam, lParam);
}

BOOL PreviewWindow::Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {

	m_cDisplay_max = CalculateDisplayMax(lpCreateStruct->cx);

	return TRUE;
}

void PreviewWindow::Cls_OnSize(HWND hwnd, UINT state, int cx, int cy) {

	auto size = CalculateDisplayMax(cx);
	if( size != m_cDisplay_max ) {
		m_cDisplay_max = size;
		SetPreviewCache(m_current_dir, m_pfilename_cache);
	}

	PositionPreviewImageWindows();
}

void PreviewWindow::Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) {

	int relative_index = (int) keyFlags;

	if( relative_index >= 0 && relative_index < m_pfilename_cache->size() )
		PostMessage(GetParent(m_window), WM_USER, m_base + relative_index, 0);
}

void PreviewWindow::ShowPreviewWindow() {

	if( m_ImageWindows.size() < 2 )	return;

	SetWindowPos(m_window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);

	ShowImagePreviewWindows();
}

void PreviewWindow::HidePreviewWindow() {
	
	ShowWindow(m_window, SW_HIDE);
}

void PreviewWindow::ShowImagePreviewWindows() {

	for( const auto& img_window : m_ImageWindows ) {

		SetWindowPos(img_window->m_window, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
		UpdateWindow(img_window->m_window);
	}
}

void PreviewWindow::SetPreviewCache(const std::wstring& current_dir, const std::vector<std::wstring> * pfilename_cache) {

	if( !pfilename_cache )	return;

	m_current_dir = current_dir;
	m_pfilename_cache = pfilename_cache;

	m_limit = min(m_cDisplay_max, m_pfilename_cache->size());
	m_base = min(m_pfilename_cache->size() - m_limit, max(0, m_display - m_cDisplay_max / 2));
	
	LoadPreviewImageWindows();
}

void PreviewWindow::SetDisplay(int index) {

	m_display = index;

	InvalidateRect(m_window, NULL, FALSE);
}

int PreviewWindow::CalculateDisplayMax(int width) {

	int cDisplay_max = ( width - PAD ) / ( SIDE + PAD );

	m_ImageWindows.reserve(cDisplay_max);

	return cDisplay_max;
}

void PreviewWindow::LoadPreviewImageWindows() {

	if( !m_pfilename_cache )	return;

	if( m_limit < 2 )				return;

	m_ImageWindows.clear();
	
	for( int i = 0; i < m_limit; ++i ) {

		std::wstring img_path = m_current_dir;
		img_path += L"\\" + m_pfilename_cache->at(i + m_base);

		auto pimg_win = std::make_unique<PreviewImageWindow>();
		pimg_win->InitWindow(SIDE, SIDE, nullptr, m_window);
		pimg_win->DisplayImage(img_path.c_str());
		pimg_win->setIndex(i);
		
		m_ImageWindows.push_back(std::move(pimg_win));
	}

	PositionPreviewImageWindows();
}

void PreviewWindow::PositionPreviewImageWindows() {

	auto cImage = m_ImageWindows.size();

	if( cImage < 1 )	return;

	int x = PAD;
	int y = PAD;

	for( const auto& img_window : m_ImageWindows ) {

		SetWindowPos(img_window->m_window, 0, x, y, 0, 0, /*SWP_SHOWWINDOW |*/ SWP_NOZORDER | SWP_NOSIZE);
		/*UpdateWindow(img_window->m_window);*/

		x += SIDE + PAD;
	}
}

void PreviewWindow::Increment() {
	
	//++m_display;

	InvalidateRect(m_window, NULL, FALSE);

	if( m_display == m_pfilename_cache->size() - 1 )	return;

	if( m_display >= m_base + m_cDisplay_max - 1 ) {
		
		m_base = min(m_pfilename_cache->size() - m_cDisplay_max, m_base + m_cDisplay_max - 1);

		LoadPreviewImageWindows();

		ShowImagePreviewWindows();
	}
}

void PreviewWindow::Decrement() {
	
	//--m_display;

	InvalidateRect(m_window, NULL, FALSE);

	if( m_display == 0 )		return;

	if( m_display <= m_base ) {

		m_base = max(0, m_base - m_cDisplay_max + 1);

		LoadPreviewImageWindows();

		ShowImagePreviewWindows();
	}
};

LRESULT PreviewWindow::PreviewImageWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	switch( message ) {

		HANDLE_MSG(m_window, WM_MOUSEWHEEL, Cls_OnMouseWheel);

	}

	return __super::MessageHandler(message, wParam, lParam);
}

void PreviewWindow::PreviewImageWindow::Cls_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys) {

	auto ctrl = GetKeyState(VK_CONTROL) & ( 1 << 15 );

	if( !ctrl ) 
		SendMessage(GetParent(m_window), WM_MOUSEWHEEL, MAKEWPARAM(fwKeys, zDelta), MAKELPARAM(xPos, yPos));	
}
#include <Windows.h>
#include <windowsx.h>

#include <shellscalingapi.h>

#include "ContextWindow.h"

using D2D1::Matrix3x2F;

ContextWindow::~ContextWindow() {

	ReleaseDeviceResources();

}

void ContextWindow::ShowContextWindow(int x, int y) {

	m_mouseOver = -1;
	m_mouseX = -1;
	m_mouseY = -1;

	if( IsWindowVisible(m_window) )	HideContextWindow();

	x += 10;
	y += 10;

	RECT parentRect;
	GetClientRect(GetParent(m_window), &parentRect);
	if( x + m_width > parentRect.right )	x = parentRect.right - m_width;
	if( y + m_height > parentRect.bottom )	y = parentRect.bottom - m_height;

	SetWindowPos(m_window, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

	//ShowWindow(m_window, SW_SHOW);
	//UpdateWindow(m_window);

}

void ContextWindow::HideContextWindow() {

	ShowWindow(m_window, SW_HIDE);

}



void ContextWindow::ReleaseDeviceResources() {

	
}


ComPtr<IDCompositionSurface> ContextWindow::CreateCommonSurface() {

	ComPtr<IDCompositionSurface> surface =
		CreateSurface(LogicalToPhysical(m_buttonWidth, m_dpiX),
			LogicalToPhysical(m_buttonHeight, m_dpiY));

	ComPtr<ID2D1DeviceContext> dc;
	POINT offset = {};

	auto hr = surface->BeginDraw(nullptr,
		__uuidof( dc ),
		reinterpret_cast<void **>( dc.GetAddressOf() ),
		&offset);

	ComPtr<ID2D1SolidColorBrush> m_graybrush;
	hr = dc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightBlue, 1.00f),
		m_graybrush.GetAddressOf());

	dc->SetDpi(m_dpiX, m_dpiY);

	dc->SetTransform(Matrix3x2F::Translation(PhysicalToLogical(offset.x, m_dpiX),
		PhysicalToLogical(offset.y, m_dpiY)));

	dc->Clear();

	D2D1_RECT_F buttonRect = D2D1::RectF(0, 0, m_buttonWidth, m_buttonHeight);
	auto buttonRoundedRect = D2D1::RoundedRect(buttonRect, 10.0f, 10.0f);
	dc->FillRoundedRectangle(&buttonRoundedRect, m_graybrush.Get());

	hr = surface->EndDraw();

	return surface;
}

ComPtr<IDCompositionVisual2> ContextWindow::CreateButtonTextVisual(const WCHAR * szText, int color) {

	ComPtr<IDCompositionVisual2> visual = CreateVisual();

	ComPtr<IDCompositionSurface> surface =
		CreateSurface(LogicalToPhysical(m_buttonWidth, m_dpiX),
			LogicalToPhysical(m_buttonHeight, m_dpiY));

	ComPtr<ID2D1DeviceContext> dc;
	POINT offset = {};

	auto hr = surface->BeginDraw(nullptr,
		__uuidof( dc ),
		reinterpret_cast<void **>( dc.GetAddressOf() ),
		&offset);

	dc->SetDpi(m_dpiX, m_dpiY);

	dc->SetTransform(Matrix3x2F::Translation(PhysicalToLogical(offset.x, m_dpiX),
		PhysicalToLogical(offset.y, m_dpiY)));

	dc->Clear();

	ComPtr<ID2D1SolidColorBrush> brush;
	hr = dc->CreateSolidColorBrush(D2D1::ColorF(color, 1.0f),
		brush.GetAddressOf());

	dc->DrawTextW(szText, wcslen(szText) + 1,
		m_textFormat.Get(), D2D1::RectF(0, 0, m_buttonWidth, m_buttonHeight), brush.Get());

	hr = surface->EndDraw();

	visual->SetContent(surface.Get());

	return visual;
}

void ContextWindow::CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) {
	
	m_buttonPadding = 4.0f;
	m_buttonHeight = ( m_height - ( BUTTON_COUNT - 1 ) * m_buttonPadding ) / (float) BUTTON_COUNT;
	m_buttonWidth = m_width;

	if( !m_textFormat ) {

		ComPtr<IDWriteFactory2> factory;

		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof( factory ),
			reinterpret_cast<IUnknown **>( factory.GetAddressOf() ));

		factory->CreateTextFormat(L"Arial",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_buttonHeight / 3.0f * 2.0f,
			L"en",
			m_textFormat.GetAddressOf());

		m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	ComPtr<IDCompositionSurface> surface =
		CreateCommonSurface();

	for( int index = 0; index < BUTTON_COUNT; ++index ) {
		
		auto& button_obj = m_buttons.at(index);
		
		auto& base_visual = get<0>(button_obj);
		auto& button_text = get<1>(button_obj);
		auto& plain_visual = get<2>(button_obj);
		auto& highlighted_visual = get<3>(button_obj);

		base_visual = CreateVisual();
		auto hr = base_visual->SetOffsetX(PhysicalToLogical(0, m_dpiX));
		hr = base_visual->SetOffsetY(PhysicalToLogical(index * (m_buttonHeight + m_buttonPadding), m_dpiY));
		hr = base_visual->SetContent(surface.Get());
		hr = m_rootVisual->AddVisual(base_visual.Get(), false, nullptr);

		plain_visual = CreateButtonTextVisual(button_text.c_str(), D2D1::ColorF::Black);
		highlighted_visual = CreateButtonTextVisual(button_text.c_str(), D2D1::ColorF::Yellow);
	}

	
}

void ContextWindow::RenderWindow() {

	int index = 0;

	for( auto& button_obj : m_buttons) {

		auto& base_visual = get<0>(button_obj);

		base_visual->RemoveAllVisuals();

		IDCompositionVisual * pchildvisual = nullptr;

		if( index == m_mouseOver ) {

			pchildvisual = get<3>(button_obj).Get();			

		} else {
			
			pchildvisual = get<2>(button_obj).Get();

		}

		base_visual->AddVisual(pchildvisual, FALSE, nullptr);

		++index;
	}
}

void ContextWindow::Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) {

	auto& button = m_buttons.at(m_mouseOver);
	auto& caption = get<1>(button);

	PostMessage(GetParent(m_window), WM_CONTEXTACTION, m_ctrlId, (LPARAM) caption.c_str());

	HideContextWindow();

}

LRESULT ContextWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	switch( message ) {

		HANDLE_MSG(m_window, WM_LBUTTONDOWN, Cls_OnLButtonDown);
		HANDLE_MSG(m_window, WM_MOUSEMOVE, Cls_OnMouseMove);
		
	}

	auto ret = __super::MessageHandler(message, wParam, lParam);
	return ret;
}


void ContextWindow::Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags) {

	m_mouseX = x;
	m_mouseY = y;

	int prev_mouseOver = m_mouseOver;
	int current_mouseOver = GetButtonFromXY(x, y);

	if( current_mouseOver == prev_mouseOver )
		return;

	m_mouseOver = current_mouseOver;

	InvalidateRect(m_window, nullptr, FALSE);

}

int ContextWindow::GetButtonFromXY(int x, int y) {

	return y / ( m_buttonHeight + m_buttonPadding );

}
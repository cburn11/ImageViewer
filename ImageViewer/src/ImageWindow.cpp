#include "ImageWindow.h"

ImageWindow::ImageWindow() : ChildCompositionWindow(L"ImageWindowClass", 1000) {

	auto hr = OleInitialize(NULL);
	
	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(m_WICFactory.GetAddressOf()));

#ifdef _DEBUG
	static unsigned int cConstruction = 0;
	m_id = ++cConstruction;
	std::wstring out = std::to_wstring(m_id);
	out += L": ImageWindow()\n";
	OutputDebugString(out.c_str());
#endif // _DEBUG
}

#ifdef _DEBUG
ImageWindow::~ImageWindow() {
	std::wstring out = std::to_wstring(m_id);
	out += L": ~ImageWindow()\n";
	OutputDebugString(out.c_str());
}
#endif // _DEBUG

void ImageWindow::CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) {
	
	m_ImageVisual.Reset();
	m_ImageVisual = CreateVisual();

	auto hr = m_rootVisual->AddVisual(m_ImageVisual.Get(), FALSE, nullptr);	
	
	m_pCompositionDevice = pCompositionDevice;

	CreateSizedResources();
}

void ImageWindow::ReleaseDeviceResources() {

	m_ImageVisual.Reset();

	m_bitmap.Reset();
}

ComPtr<IDCompositionSurface> ImageWindow::CreateImageSurface() {

	if( !m_WICBitmap ) return nullptr;

	DBG_OUT(L"CreateSurface()", m_cCreateSurface);

	ComPtr<IDCompositionSurface> surface = CreateSurface(m_baseWidth, m_baseHeight);
	if( !surface ) return nullptr;

	ComPtr<ID2D1DeviceContext> dc;
	POINT offset = {};
	auto hr = surface->BeginDraw(nullptr, IID_PPV_ARGS(dc.GetAddressOf()), &offset);
	
	dc->SetTransform(D2D1::Matrix3x2F::Translation(offset.x, offset.y));

	dc->Clear(/*D2D1::ColorF(D2D1::ColorF::LightBlue)*/);	

	if( !m_bitmap ) {
		hr = dc->CreateBitmapFromWicBitmap(m_WICBitmap.Get(), m_bitmap.GetAddressOf());
		DBG_OUT(L"CreateBitmapFromWICBitmap()", m_cCreateBitmap);
	}

	auto size = m_bitmap->GetSize();
	m_imgWidth = size.width;
	m_imgHeight = size.height;

	auto ratio = size.width / size.height;

	float scaled_height, scaled_width;

	m_x = 0;
	m_y = 0;

	if( m_fNoScale ) {

		scaled_width = size.width * m_zoom;
		scaled_height = size.height * m_zoom;

	} else {

		if( ratio > 1 ) {			//	width > height

			scaled_height = min(m_baseWidth / ratio, m_baseHeight);
			scaled_width = scaled_height * ratio;

			m_y = m_baseHeight / 2 - scaled_height / 2;

		} else if( ratio < 1 ) {	//	width < height

			scaled_width = min(m_baseHeight * ratio, m_baseWidth);
			scaled_height = scaled_width / ratio;

			m_x = m_baseWidth / 2 - scaled_width / 2;

		} else {

			scaled_width = min(m_baseWidth, m_baseHeight);
			scaled_height = scaled_width;

		}

		scaled_width *= m_zoom;
		scaled_height *= m_zoom;
	}

	m_scaledWidth = scaled_width;
	m_scaledHeight = scaled_height;

	if( m_angle == 0 || m_angle == 180 ) {

		m_scrollMaxX = m_baseWidth - m_x - 20;
		m_scrollMinX = -m_scaledWidth;

		m_scrollMaxY = m_baseHeight - m_y - 20;
		m_scrollMinY = -m_y - m_scaledHeight + 20;

	} else {

		m_scrollMaxX = m_baseWidth - m_x - 20;
		m_scrollMinX = -m_x - m_scaledWidth + 20;

		m_scrollMaxY = m_baseHeight - 20;
		m_scrollMinY = -m_scaledHeight + 20;

	}

	if( m_scaledHeight > m_baseHeight + 200 && m_height > 200 ) {

		int cx = 10;
		int cy = m_baseHeight * m_baseHeight / m_scaledHeight;
		int x = m_baseWidth - cx;
		int y = ( m_scrollY - m_scrollMinY ) * ( m_baseHeight - cy ) / ( m_scrollMaxY - m_scrollMinY );

		if( m_angle == 0 )
			m_pVertScrollBarWindow->ShowScrollBarWindow(x, m_baseHeight - y - cy, cx, cy);
		else if( m_angle == 90 )
			m_pVertScrollBarWindow->ShowScrollBarWindow(y, x, cy, cx);
		else if( m_angle == 180 )
			m_pVertScrollBarWindow->ShowScrollBarWindow(x, y, cx, cy);
		else
			m_pVertScrollBarWindow->ShowScrollBarWindow(m_baseHeight - y - cy, x, cy, cx);

	} else {

		m_pVertScrollBarWindow->HideScrollBarWindow();
	}

	if( m_scaledWidth > m_baseWidth + 200 && m_width > 200 ) {

		int cy = 10;
		int cx = m_baseWidth * m_baseWidth / m_scaledWidth;
		int y = m_baseHeight - cy;
		int x = ( m_scrollX - m_scrollMinX ) * ( m_baseWidth - cx ) / ( m_scrollMaxX - m_scrollMinX );

		if( m_angle == 0)
			m_pHorzScrollBarWindow->ShowScrollBarWindow(m_baseWidth - x - cx, y, cx, cy);
		else if( m_angle == 90 )
			m_pHorzScrollBarWindow->ShowScrollBarWindow(y, m_baseWidth - x - cx, cy, cx);
		else if( m_angle == 180 )
			m_pHorzScrollBarWindow->ShowScrollBarWindow(x, y, cx, cy);
		else
			m_pHorzScrollBarWindow->ShowScrollBarWindow(y, x, cy, cx);

	} else {

		m_pHorzScrollBarWindow->HideScrollBarWindow();
	}

	D2D1_RECT_F rect;
	rect = D2D1::RectF(0, 0, scaled_width, scaled_height);

	auto transform = D2D1::Matrix4x4F::Translation(m_x + m_scrollX, m_y + m_scrollY, 0);

	dc->DrawBitmap(m_bitmap.Get(), &rect, 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, (const D2D1_RECT_F *) 0, &transform);

	hr = surface->EndDraw();

	return surface;
}

void ImageWindow::RenderWindow() {

	DBG_OUT(L"RenderWindow()", m_cRender);

	if( !m_fRedraw )		return;

	if( !m_ImageVisual )	return;

	auto surface = CreateImageSurface();
	m_ImageVisual->SetContent(surface.Get());	

	m_fRedraw = false;
}

bool ImageWindow::DisplayImage(const WCHAR * szImagePath) {

	if( !szImagePath )	return false;
	if( !m_WICFactory ) return false;

	if( wcslen(szImagePath) == 0 )
		return false;

	ComPtr<IWICBitmapDecoder> decoder;
	HRESULT hr = m_WICFactory->CreateDecoderFromFilename(szImagePath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
	
	if( S_OK != hr ) {

		MessageBox(m_window, szImagePath, L"Image Viewer unable to open file:", MB_ICONERROR);
		return false;
	}

	ComPtr<IWICBitmapFrameDecode> frame;
	if( S_OK !=
		decoder->GetFrame(0, frame.GetAddressOf()) )		return false;

	ComPtr<IWICFormatConverter> converter;
	hr = m_WICFactory->CreateFormatConverter(converter.GetAddressOf());

	if( S_OK != converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGR,
		WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom) ) return false;

	if( S_OK !=
		m_WICFactory->CreateBitmapFromSource(converter.Get(), WICBitmapCacheOnLoad, m_WICBitmap.ReleaseAndGetAddressOf()) )

		return false;

	m_bitmap.Reset();
	Rotate(0.0f);

	InvalidateRect(m_window, nullptr, FALSE);

	return true;
}

void ImageWindow::CreateSizedResources() {

	if( m_baseWidth == m_width && m_baseHeight == m_height )
		return;

	m_baseHeight = m_height;
	m_baseWidth = m_width;

	ComPtr<IDCompositionRotateTransform> protation;
	auto hr = m_pCompositionDevice->CreateRotateTransform(protation.ReleaseAndGetAddressOf());
	protation->SetAngle(90.0f);
	protation->SetCenterX(m_width);
	protation->SetCenterY(m_height);

	ComPtr<IDCompositionTranslateTransform> ptranslation;
	hr = m_pCompositionDevice->CreateTranslateTransform(ptranslation.ReleaseAndGetAddressOf());
	ptranslation->SetOffsetX(-m_height);
	ptranslation->SetOffsetY(-( m_height - m_width ));

	IDCompositionTransform * transforms[2] = { protation.Get(), ptranslation.Get() };

	hr = m_pCompositionDevice->CreateTransformGroup(transforms, 2, m_transformGroup_90.ReleaseAndGetAddressOf());

	hr = m_pCompositionDevice->CreateRotateTransform(protation.ReleaseAndGetAddressOf());
	protation->SetAngle(180.0f);
	protation->SetCenterX(m_width);
	protation->SetCenterY(m_height);

	hr = m_pCompositionDevice->CreateTranslateTransform(ptranslation.ReleaseAndGetAddressOf());
	ptranslation->SetOffsetX(-m_width);
	ptranslation->SetOffsetY(-m_height);

	transforms[0] = protation.Get();
	transforms[1] = ptranslation.Get();

	hr = m_pCompositionDevice->CreateTransformGroup(transforms, 2, m_transformGroup_180.ReleaseAndGetAddressOf());

	hr = m_pCompositionDevice->CreateRotateTransform(protation.ReleaseAndGetAddressOf());
	protation->SetAngle(270.0f);
	protation->SetCenterX(m_width);
	protation->SetCenterY(m_height);

	hr = m_pCompositionDevice->CreateTranslateTransform(ptranslation.ReleaseAndGetAddressOf());
	ptranslation->SetOffsetX(m_height - m_width);
	ptranslation->SetOffsetY(-m_width);

	transforms[0] = protation.Get();
	transforms[1] = ptranslation.Get();

	hr = m_pCompositionDevice->CreateTransformGroup(transforms, 2, m_transformGroup_270.ReleaseAndGetAddressOf());
}

void ImageWindow::Cls_OnSize(HWND hwnd, UINT state, int cx, int cy) {

	if( IsDeviceCreated() )
		CreateSizedResources();

	if( m_angle != 0.0f )
		Rotate(m_angle);

	if( !m_pZoomStatusWindow ) {
		m_pZoomStatusWindow = std::make_unique<ZoomStatusWindow>();
		m_pZoomStatusWindow->InitWindow(100, 50, L"100%", m_window);
	}

	if( !m_pHorzScrollBarWindow ) {
		m_pHorzScrollBarWindow = std::make_unique<ScrollBarWindow>();
		m_pHorzScrollBarWindow->InitWindow(0, 0, nullptr, m_window);
		m_pVertScrollBarWindow = std::make_unique<ScrollBarWindow>();
		m_pVertScrollBarWindow->InitWindow(0, 0, nullptr, m_window);
	}
}

void ImageWindow::Cls_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys) {

	auto ctrl = GetKeyState(VK_CONTROL) & (1 << 15);
	
	if( ctrl ) {

		ChangeZoom(zDelta);

	} else {

		SendMessage(GetParent(m_window), WM_MOUSEWHEEL, MAKEWPARAM(fwKeys, zDelta), MAKELPARAM(xPos, yPos));
	}
}

void ImageWindow::Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) {

	m_fScroll = true;

	m_mouseX = x;
	m_mouseY = y;
	
	SendMessage(GetParent(m_window), WM_LBUTTONDOWN, (WPARAM) m_index, MAKELPARAM(x, y));
}

void ImageWindow::Cls_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags) {

	m_fScroll = false;
}

void ImageWindow::Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags) {

	if( m_fScroll ) {

		int dx = x - m_mouseX;
		int dy = y - m_mouseY;

		m_mouseX = x;
		m_mouseY = y;

		Scroll(dx, dy);
	}
}

LRESULT ImageWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	switch( message ) {

		HANDLE_MSG(m_window, WM_LBUTTONDOWN, Cls_OnLButtonDown);
		HANDLE_MSG(m_window, WM_LBUTTONUP, Cls_OnLButtonUp);
		HANDLE_MSG(m_window, WM_MOUSEWHEEL, Cls_OnMouseWheel);

	case WM_SIZE: {

		auto ret = __super::MessageHandler(message, wParam, lParam);
		Cls_OnSize(m_window, (UINT) wParam, LOWORD(lParam), HIWORD(lParam));

		return ret; }	

	case WM_MOUSEMOVE:

		Cls_OnMouseMove(m_window, LOWORD(lParam), HIWORD(lParam), (UINT) wParam);
		
		//Fall through to WM_RBUTTONDOWN

	case WM_RBUTTONDOWN:

		SendMessage(GetParent(m_window), message, wParam, lParam);
		
		break;
	}

	return __super::MessageHandler(message, wParam, lParam);
}

void ImageWindow::Rotate(float angle) {
	
	if( !m_ImageVisual )	return;

	m_angle = angle;

	auto hr = m_ImageVisual->SetTransform(nullptr);

	m_baseWidth = m_width;
	m_baseHeight = m_height;
	
	 if( m_angle == 90.0f ) {

		auto hr = m_ImageVisual->SetTransform(m_transformGroup_90.Get());

		m_baseWidth = m_height;
		m_baseHeight = m_width;

	} else if( m_angle == 180.0f ) {

		hr = m_ImageVisual->SetTransform(m_transformGroup_180.Get());

	} else if( m_angle == 270.0f ) {

		auto hr = m_ImageVisual->SetTransform(m_transformGroup_270.Get());

		m_baseWidth = m_height;
		m_baseHeight = m_width;

	}

	Reset();
}

void ImageWindow::ToggleScale() {

	m_fNoScale = !m_fNoScale;	

	m_mouseX = 0;
	m_mouseY = 0;
	m_scrollX = 0;
	m_scrollY = 0;

	m_zoom = 1.0;

	Redraw();
	//InvalidateRect(m_window, nullptr, FALSE);
	/*UpdateWindow(m_window);*/
}

void ImageWindow::ChangeZoom(int direction) {

	m_zoom += direction > 0 ? 0.05f : -0.05f;

	m_zoom = max(0.05, min(5.0, m_zoom));

	auto zoom = floor(100.0f * (m_fNoScale ? m_zoom : m_scaledHeight / m_imgHeight) + 0.5);
	std::wstring zoom_text = std::to_wstring((int) zoom);
	zoom_text += L"%";
	m_pZoomStatusWindow->SetText(zoom_text);
	m_pZoomStatusWindow->ShowZoomStatusWindow();

	Redraw();
	//InvalidateRect(m_window, nullptr, FALSE);
	/*UpdateWindow(m_window);*/
}

void ImageWindow::Reset() {

	m_fNoScale = false;

	m_mouseX = 0;
	m_mouseY = 0;
	m_scrollX = 0;
	m_scrollY = 0;

	m_zoom = 1.0;

	Redraw();
	//InvalidateRect(m_window, nullptr, FALSE);
	/*UpdateWindow(m_window);*/
}

void ImageWindow::Scroll(int dx, int dy) {

	if( m_zoom == 1.0 && !m_fNoScale )
		return;

	if( m_angle == 90 ) {

		auto temp = dx;
		dx = dy;
		dy = temp * -1;

	} else if( m_angle == 180 ) {

		dx *= -1;
		dy *= -1;

	} else if( m_angle == 270 ) {

		auto temp = dx;
		dx = dy * -1;
		dy = temp;
	}

	float scrollX = m_scrollX;
	float scrollY = m_scrollY;

	scrollX += dx;
	scrollY += dy;
	
	scrollX = max(m_scrollMinX, min(m_scrollMaxX, scrollX));
	scrollY = max(m_scrollMinY, min(m_scrollMaxY, scrollY));
	
	if( scrollX != m_scrollX || scrollY != m_scrollY ) {

		m_scrollX = scrollX;
		m_scrollY = scrollY;

		//InvalidateRect(m_window, nullptr, FALSE);
		Redraw();
	}

	/*UpdateWindow(m_window);*/
}

ImageWindow::ZoomStatusWindow::ZoomStatusWindow() : ChildCompositionWindow(L"ImageWindowZoomStatusWindow", 1001,
	D2D1::ColorF::White, 0.0f) {

}

void ImageWindow::ZoomStatusWindow::CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) {

	if( !m_textFormat ) {

		ComPtr<IDWriteFactory2> factory;

		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof( factory ),
			reinterpret_cast<IUnknown **>( factory.GetAddressOf() ));

		factory->CreateTextFormat(L"Arial",
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_height / 3.0f * 2.0f,
			L"en",
			m_textFormat.GetAddressOf());

		m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	auto hr = pDeviceContext2D->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 1.0f),
		m_brush.ReleaseAndGetAddressOf());

	m_textVisual = CreateVisual();

	hr = m_rootVisual->AddVisual(m_textVisual.Get(), FALSE, nullptr);
}

void ImageWindow::ZoomStatusWindow::ReleaseDeviceResources() {

}

void ImageWindow::ZoomStatusWindow::RenderWindow() {

	auto surface = CreateSurface(m_width, m_height);

	ComPtr<ID2D1DeviceContext> dc;
	POINT offset = {};
	auto hr = surface->BeginDraw(nullptr, IID_PPV_ARGS(dc.GetAddressOf()), &offset);

	dc->SetTransform(D2D1::Matrix3x2F::Translation(offset.x, offset.y));
	dc->Clear();

	dc->DrawTextW(m_text.c_str(), m_text.length(),
		m_textFormat.Get(), D2D1::RectF(0, 0, m_width, m_height), m_brush.Get());

	hr = surface->EndDraw();
	hr = m_textVisual->SetContent(surface.Get());
}

LRESULT ImageWindow::ZoomStatusWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	switch( message ) {

		HANDLE_MSG(m_window, WM_TIMER, Cls_OnTimer);
	}

	return __super::MessageHandler(message, wParam, lParam);
}

void ImageWindow::ZoomStatusWindow::Cls_OnTimer(HWND hwnd, UINT id) {

	if( id == m_timerId ) {

		KillTimer(m_window, m_timerId);
		HideZoomStatusWindow();
	}
}

void ImageWindow::ZoomStatusWindow::ShowZoomStatusWindow() {

	SetWindowPos(m_window, NULL, 5, 5, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

	m_timerId = SetTimer(m_window, 100, 800, NULL);

	InvalidateRect(m_window, nullptr, FALSE);
	UpdateWindow(m_window);
}

void ImageWindow::ZoomStatusWindow::HideZoomStatusWindow() {

	ShowWindow(m_window, SW_HIDE);
}

void ImageWindow::ZoomStatusWindow::SetText(const std::wstring& text) {

	m_text = text;

	InvalidateRect(m_window, nullptr, FALSE);
	UpdateWindow(m_window);
}

void ImageWindow::ZoomStatusWindow::InitWindow(int width, int height, const WCHAR * szCaption, HWND hwndParent) {

	SetText(szCaption);

	__super::InitWindow(width, height, szCaption, hwndParent);
}

ImageWindow::ScrollBarWindow::ScrollBarWindow() : 
	ChildCompositionWindow(L"ImageWindowScrollBarWindow", 1002, D2D1::ColorF::Red, 0.5f)	{

}

void ImageWindow::ScrollBarWindow::CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) {

}

void ImageWindow::ScrollBarWindow::ReleaseDeviceResources() {

}

void ImageWindow::ScrollBarWindow::RenderWindow() {

}

LRESULT ImageWindow::ScrollBarWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

	return __super::MessageHandler(message, wParam, lParam);
}

void ImageWindow::ScrollBarWindow::ShowScrollBarWindow(int x, int y, int cx, int cy) {

	if( IsWindowVisible(m_window) ) {

		SetWindowPos(m_window, NULL, x, y, cx, cy, SWP_NOZORDER | SWP_NOREDRAW);

	} else {

		SetWindowPos(m_window, NULL, x, y, cx, cy, SWP_SHOWWINDOW);

		InvalidateRect(m_window, nullptr, FALSE);
		UpdateWindow(m_window);
	}
}

void ImageWindow::ScrollBarWindow::HideScrollBarWindow() {

	ShowWindow(m_window, SW_HIDE);
}
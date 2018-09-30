#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <WindowClass.h>

#include <shellscalingapi.h>

#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>

#include <dcomp.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "shcore")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")


struct ComException {

	HRESULT result;

	ComException(HRESULT const value) : result(value) {

	}
};

template <typename T>
static float PhysicalToLogical(T const pixel, float const dpi) {
	return pixel * 96.0f / dpi;
}

template <typename T>
static float LogicalToPhysical(T const pixel, float const dpi) {
	return pixel * dpi / 96.0f;
}

class CompositionWindow {

	private:

	bool m_NeedSurfaceResize = true;

	ComPtr<ID3D11Device> m_device3D;
	ComPtr<IDCompositionDesktopDevice> m_device;
	ComPtr<IDCompositionTarget> m_target;

	ComPtr<IDCompositionVisual> m_backgroundVisual;	

	ComPtr<IDCompositionVisual> CreateBackgroundVisual() {

		ComPtr<IDCompositionVisual> visual = CreateVisual();

		ComPtr<IDCompositionSurface> surface = CreateSurface(10, 10);

		visual->SetContent(surface.Get());

		ComPtr<ID2D1DeviceContext> dc;
		POINT offset = {};
		auto hr = surface->BeginDraw(nullptr, IID_PPV_ARGS(dc.GetAddressOf()), &offset);

		dc->SetDpi(m_dpiX, m_dpiY);
		dc->SetTransform(D2D1::Matrix3x2F::Translation(offset.x * 96 / m_dpiX, offset.y * 96 / m_dpiY));
		dc->Clear(D2D1::ColorF(m_colorf, m_opacity));

		hr = surface->EndDraw();

		m_NeedSurfaceResize = true;

		return visual;
	}

	void CreateDevice3D() {

		if( IsDeviceCreated() )
			return;

		unsigned flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT |
			D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef _DEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		auto hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
			nullptr, 0, D3D11_SDK_VERSION, m_device3D.GetAddressOf(), nullptr, nullptr);
	}

	ComPtr<ID2D1Device> CreateDevice2D() {
		ComPtr<IDXGIDevice3> deviceX;
		auto hr = m_device3D.As(&deviceX);

		D2D1_CREATION_PROPERTIES properties = {};

#ifdef _DEBUG
		properties.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

		ComPtr<ID2D1Device> device2D;

		hr = D2D1CreateDevice(deviceX.Get(), properties, device2D.GetAddressOf());

		return device2D;
	}

protected:

	bool IsDeviceCreated() const {

		return m_device3D;
	}

	void IntlCreateDeviceResources(HWND hwnd) {

		if( IsDeviceCreated() )
			return;

		CreateDevice3D();

		ComPtr<ID2D1Device> const device2D = CreateDevice2D();

		auto hr = DCompositionCreateDevice2(device2D.Get(), IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));

		hr = m_device->CreateTargetForHwnd(hwnd, false, m_target.ReleaseAndGetAddressOf());

		m_rootVisual = CreateVisual();
		hr = m_target->SetRoot(m_rootVisual.Get());

		m_backgroundVisual = CreateBackgroundVisual();
		m_rootVisual->AddVisual(m_backgroundVisual.Get(), FALSE, nullptr);

		ComPtr<ID2D1DeviceContext> context2D;
		hr = device2D->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, context2D.GetAddressOf());

		CreateDeviceResources(context2D.Get(), m_device.Get());
	}

	void IntlReleaseDeviceResources() {

		m_device3D.Reset();

		ReleaseDeviceResources();
	}

	void IntlRenderWindow() {

		if( !IsDeviceCreated() )
			return;

		if( m_NeedSurfaceResize ) {

			m_NeedSurfaceResize = false;

			m_backgroundVisual->SetTransform(
				D2D1::Matrix3x2F::Scale(D2D1::SizeF(m_width, m_height), D2D1::Point2F(0, 0))
			);
		}

		RenderWindow();
	}

	void ScaleWindow(int physicalX, int physicalY) {

		m_width = PhysicalToLogical(physicalX, m_dpiX);
		m_height = PhysicalToLogical(physicalY, m_dpiY);

	}

	void GetDPI(HWND hwnd) {

		HMONITOR const monitor = MonitorFromWindow(hwnd,
			MONITOR_DEFAULTTONEAREST);

		unsigned dpiX = 0;
		unsigned dpiY = 0;

		auto hr = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

		m_dpiX = static_cast<float>( dpiX );
		m_dpiY = static_cast<float>( dpiY );
	}

	void Cls_OnPaint(HWND hwnd) {

		try {

			if( !IsDeviceCreated() ) {

				IntlCreateDeviceResources(hwnd);
			}

			auto hr = m_device3D->GetDeviceRemovedReason();
			if( S_OK != hr ) throw ComException{ hr };

			IntlRenderWindow();
			hr = m_device->Commit();
			if( S_OK != hr ) throw ComException{ hr };

			ValidateRect(hwnd, nullptr);

		}
		catch( ComException const & e ) {

			IntlReleaseDeviceResources();
		}
	}

	void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy) {

		ScaleWindow(cx, cy);

		m_NeedSurfaceResize = true;

		UpdateWindow(hwnd);
	}

	BOOL Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {

		GetDPI(hwnd);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		ScaleWindow(clientRect.right, clientRect.bottom);

		SetWindowPos(hwnd, nullptr, 0, 0,
			m_width, m_height,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

		return TRUE;
	}

	void DpiChangedHandler(HWND hwnd, WPARAM const wparam, LPARAM const lparam) {

		GetDPI(hwnd);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		ScaleWindow(clientRect.right, clientRect.bottom);

		SetWindowPos(hwnd, nullptr, 0, 0,
			m_width, m_height,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

		InvalidateRect(hwnd, nullptr, FALSE);

		UpdateWindow(hwnd);
	}

public:

	float m_dpiX = 0.0f;
	float m_dpiY = 0.0f;

	float m_width;
	float m_height;

	D2D1::ColorF::Enum m_colorf;
	float m_opacity;

	ComPtr<IDCompositionVisual2> m_rootVisual;

	virtual void CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) = 0;
	virtual void ReleaseDeviceResources() = 0;
	virtual void RenderWindow() = 0;

	CompositionWindow(D2D1::ColorF::Enum colorf = D2D1::ColorF::White, float opacity = 1.0f) :
		m_colorf{ colorf }, m_opacity{ opacity } {

	}

	ComPtr<IDCompositionVisual2> CreateVisual() {

		ComPtr<IDCompositionVisual2> visual;

		auto hr = m_device->CreateVisual(visual.GetAddressOf());

		return visual;
	}

	template <typename T>
	ComPtr<IDCompositionSurface> CreateSurface(T const width, T const height) {

		ComPtr<IDCompositionSurface> surface;

		auto hr = m_device->CreateSurface(static_cast<unsigned>( width ), static_cast<unsigned>( height ),
			DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, surface.GetAddressOf());

		return surface;
	}

};

template <typename Derived>
class ApplicationCompositionWindow :
	public CompositionWindow, public ApplicationWindow<Derived> {

public:

	using BasicWindow<Derived>::wc;
	using BasicWindow<Derived>::m_window;

	ApplicationCompositionWindow(D2D1::ColorF::Enum colorf	= D2D1::ColorF::White, 
		float opacity				= 1.0f) :
			CompositionWindow(colorf, opacity) {

		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hInstance = (HINSTANCE) GetModuleHandle(nullptr);
		wc.lpfnWndProc = BasicWindow<Derived>::WndProc;
		wc.lpszClassName = L"CompositionAppMainWindow";
		wc.style = CS_VREDRAW | CS_HREDRAW;

		auto reg = RegisterClass(&wc);

	}

	void InitWindow(int width, int height, const WCHAR * szCaption) {
		
		HWND hwnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName, L"Composition App", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, wc.hInstance, this);

		ShowWindow(hwnd, SW_SHOW);
	}

	LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

		switch( message ) {

			HANDLE_MSG(m_window, WM_PAINT, Cls_OnPaint);
			HANDLE_MSG(m_window, WM_CREATE, Cls_OnCreate);
			HANDLE_MSG(m_window, WM_SIZE, Cls_OnSize);

		case WM_DPICHANGED:
			DpiChangedHandler(m_window, wParam, lParam);

		}

		auto ret = __super::MessageHandler(message, wParam, lParam);

		return ret;
	}
};

template <typename Derived>
class ChildCompositionWindow :
	public BasicWindow<Derived>, public CompositionWindow {

protected:

	UINT m_ctrlId;

public:

	using CompositionWindow::CompositionWindow;

	using BasicWindow<Derived>::wc;
	using BasicWindow<Derived>::m_window;

	ChildCompositionWindow(const WCHAR * szClassName, UINT ctrlId,
		D2D1::ColorF::Enum colorf	= D2D1::ColorF::White, 
		float opacity				= 1.0f) :
			CompositionWindow(colorf, opacity) {

		wc.hInstance = (HINSTANCE) GetModuleHandle(nullptr);
		wc.lpfnWndProc = BasicWindow<Derived>::WndProc;
		wc.lpszClassName = szClassName;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.style = CS_VREDRAW | CS_HREDRAW;

		auto reg = RegisterClass(&wc);
		
		m_ctrlId = ctrlId;
	}

	virtual ~ChildCompositionWindow() { DestroyWindow(m_window); }

	void InitWindow(int width, int height, const WCHAR * szCaption, HWND hwndParent) {

		HWND hwnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName, szCaption, WS_CHILD,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height, hwndParent, NULL, wc.hInstance, this);

	}

	LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) {

		switch( message ) {

			HANDLE_MSG(m_window, WM_PAINT, Cls_OnPaint);
			HANDLE_MSG(m_window, WM_CREATE, Cls_OnCreate);
			HANDLE_MSG(m_window, WM_SIZE, Cls_OnSize);

		case WM_DPICHANGED:
			DpiChangedHandler(m_window, wParam, lParam);

		}

		return DefWindowProc(m_window, message, wParam, lParam);
	}
};
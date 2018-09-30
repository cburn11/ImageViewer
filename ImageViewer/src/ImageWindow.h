#pragma once

#include <CompositionWindow.h>

#include <memory>
#include <string>

#include <wincodec.h>
#pragma comment(lib, "Windowscodecs.lib")

#include <dwrite_2.h>
#pragma comment(lib, "dwrite")

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#ifdef _DEBUG

#define DBG_OUT(FuncCall, Count) DebugStmt(FuncCall, ++Count)

#else

#define DBG_OUT(FuncCall, Count) 

#endif // _DEBUG

class ImageWindow : public ChildCompositionWindow<ImageWindow> {

	class ZoomStatusWindow;
	class ScrollBarWindow;

	float m_angle = 0.0f;

	float m_zoom = 1.0f;

	float m_x = 0;
	float m_y = 0;

	int m_mouseX = 0;
	int m_mouseY = 0;

	float m_scrollX = 0.0f;
	float m_scrollY = 0.0f;

	float m_scrollMaxX = 0.0f;
	float m_scrollMinX = 0.0f;
	float m_scrollMaxY = 0.0f;
	float m_scrollMinY = 0.0f;

	float m_baseWidth;
	float m_baseHeight;

	float m_scaledWidth;
	float m_scaledHeight;

	float m_imgWidth;
	float m_imgHeight;

	int m_index;

	bool m_fNoScale = false;
	bool m_fScroll = false;

	bool m_fRedraw = true;

	IDCompositionDevice2 * m_pCompositionDevice;

	ComPtr<IDCompositionTransform> m_transformGroup_90;
	ComPtr<IDCompositionTransform> m_transformGroup_180;
	ComPtr<IDCompositionTransform> m_transformGroup_270;

	ComPtr<IWICImagingFactory> m_WICFactory;
	ComPtr<IWICBitmap> m_WICBitmap;

	ComPtr<ID2D1Bitmap> m_bitmap;

	ComPtr<IDCompositionVisual>			m_ImageVisual;

	std::unique_ptr<ZoomStatusWindow>	m_pZoomStatusWindow;
	std::unique_ptr<ScrollBarWindow>	m_pHorzScrollBarWindow;
	std::unique_ptr<ScrollBarWindow>	m_pVertScrollBarWindow;

	ComPtr<IDCompositionSurface> CreateImageSurface();

	void CreateSizedResources();
	void Redraw() { m_fRedraw = true;  InvalidateRect(m_window, nullptr, FALSE); }

	void CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) override;
	void ReleaseDeviceResources() override;
	void RenderWindow() override;

	void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy);
	void Cls_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys);
	void Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	void Cls_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
	void Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);

#ifdef _DEBUG

	unsigned int m_cRender = 0;
	unsigned int m_cCreateSurface = 0;
	unsigned int m_cCreateBitmap = 0;

	unsigned int m_id;

	void DebugStmt(const WCHAR * szFuncCall, unsigned int counter) {
		static WCHAR szDbgOut[256]{ 0 };
		swprintf_s(szDbgOut, L"%3i: %5i, %s\n", m_id, counter, szFuncCall);
		OutputDebugString(szDbgOut);
	}
#endif //	_DEBUG

public:

	ImageWindow();

#ifdef _DEBUG
	virtual ~ImageWindow();
#endif // _DEBUG

	bool DisplayImage(const WCHAR * szImagePath);

	void Rotate(float angle);
	void setIndex(int index) { m_index = index; }
	void ToggleScale();
	void ChangeZoom(int direction);
	void Reset();
	void Scroll(int dx, int dy);

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override;

};

class ImageWindow::ZoomStatusWindow : public ChildCompositionWindow<ZoomStatusWindow> {

	std::wstring					m_text;

	UINT							m_timerId;

	ComPtr<IDCompositionVisual>		m_textVisual;
	ComPtr<IDWriteTextFormat>		m_textFormat;
	ComPtr<ID2D1SolidColorBrush>	m_brush;

	void CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) override;
	void ReleaseDeviceResources() override;
	void RenderWindow() override;

	void Cls_OnTimer(HWND hwnd, UINT id);

public:

	ZoomStatusWindow();

	void ShowZoomStatusWindow();
	void HideZoomStatusWindow();
	void SetText(const std::wstring& text);

	void InitWindow(int width, int height, const WCHAR * szCaption, HWND hwndParent);

	LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) override;
};

class ImageWindow::ScrollBarWindow : public ChildCompositionWindow<ScrollBarWindow> {

	void CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) override;
	void ReleaseDeviceResources() override;
	void RenderWindow() override;

public:

	ScrollBarWindow();

	void ShowScrollBarWindow(int x, int y, int cx, int cy);
	void HideScrollBarWindow();

	LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) override;
};
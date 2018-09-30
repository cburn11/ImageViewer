#pragma once

#include <Windows.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#include <vector>
using std::vector;

#include <tuple>
using std::tuple;
using std::make_tuple;
using std::get;

#include <string>
using std::wstring;

#include <dwrite_2.h>

#include "CompositionWindow.h"

#define WM_CONTEXTACTION	(WM_USER + 13)

class ContextWindow : public ChildCompositionWindow<ContextWindow> {

	int BUTTON_COUNT;

	float m_buttonPadding;
	float m_buttonHeight;
	float m_buttonWidth;

	int m_mouseX = -1;
	int m_mouseY = -1;
	int m_mouseOver = -1;

	ComPtr<IDWriteTextFormat> m_textFormat;
	
	using visual_ptr = ComPtr<IDCompositionVisual2>;
	//	Base visual, button caption, plain visual, highlighted visual
	using button_tuple = tuple<visual_ptr, wstring, visual_ptr, visual_ptr>;
	vector<button_tuple> m_buttons;
	
	void CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) override;
	void ReleaseDeviceResources() override;
	void RenderWindow() override;

	ComPtr<IDCompositionVisual2> CreateButtonTextVisual(const WCHAR * szText, int color);
	ComPtr<IDCompositionSurface> CreateCommonSurface();

	void Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	void Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);

	int GetButtonFromXY(int x, int y);

public:
	
	template <typename T, size_t N>
	ContextWindow(UINT ctrlId, const T(&button_text)[N]) :
		ChildCompositionWindow(L"ContextMenu", ctrlId,
			D2D1::ColorF::White, 0.0f) {

		m_ctrlId = ctrlId;

		BUTTON_COUNT = N;
		m_buttons.reserve(N);

		for( int index = 0; index < BUTTON_COUNT; ++index ) {

			m_buttons.push_back(make_tuple(nullptr, button_text[index], nullptr, nullptr));

		}

	}

	~ContextWindow();

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override;

	void ShowContextWindow(int x, int y);
	void HideContextWindow();

};
#pragma once

#include <vector>
#include <algorithm>
#include <memory>
#include <string>

#include <CompositionWindow.h>

#include "ImageWindow.h"

class PreviewWindow : public ChildCompositionWindow<PreviewWindow> {

	/*
	 *	Subclass to disable zoom on CTRL + MouseWheel
	 */
	class PreviewImageWindow : public ImageWindow {

		void Cls_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys);

	public:

		LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override;
	};

	int m_cDisplay_max	= -1;
	int m_display		= -1;
	int m_base			= -1;
	int m_limit			= -1;

	std::wstring	m_current_dir;
	const std::vector<std::wstring> * m_pfilename_cache = nullptr;

	std::vector<std::unique_ptr<PreviewImageWindow>> m_ImageWindows;

	ComPtr<IDCompositionVisual>				m_HighlightVisual;

	BOOL Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

	void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy);
	void Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

	void CreateDeviceResources(ID2D1DeviceContext * pDeviceContext2D, IDCompositionDevice2 * pCompositionDevice) override;
	void ReleaseDeviceResources() override;
	void RenderWindow() override;

	int CalculateDisplayMax(int width);

	void LoadPreviewImageWindows();
	void PositionPreviewImageWindows();
	void ShowImagePreviewWindows();
	
public:

	static const int SIDE	= 120;
	static const int PAD	= 5;
	static const int HEIGHT = SIDE + 2 * PAD;

	using ChildCompositionWindow::ChildCompositionWindow;	

	void SetDisplay(int index);
	void SetPreviewCache(const std::wstring& current_dir, const std::vector<std::wstring> * pfilename_cache);
	void Increment();
	void Decrement();
	void ShowPreviewWindow();
	void HidePreviewWindow();

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override;
};
#pragma once

#include <WindowClass.h>

#include <Ole2.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlalloc.h>

#include <string>
#include <vector>
#include <memory>

#include "ImageWindow.h"
#include "ContextWindow.h"
#include "PreviewWindow.h"

#include "ApplicationSettings.h"

#define IVMW_RESIZEPREVIEW (WM_USER + 1)

class ImageViewerMainWindow : public ApplicationWindow<ImageViewerMainWindow> {
	
	static const unsigned int slideshowId = 1001;
	static const unsigned int previewResizeId = 1002;

	std::unique_ptr<ImageWindow>	m_pImageWindow;
	
	std::unique_ptr<ContextWindow>	m_pContextWindow;
	std::unique_ptr<PreviewWindow>	m_pPreviewWindow;

	std::unique_ptr<ApplicationSettings> m_pApplicationSettings;

	std::wstring	m_current_dir;
	std::wstring	m_current_image_name;
	std::vector<std::wstring> m_image_filename_cache;
	UINT m_index;

	float m_angle = 0.0;

	bool m_fSlideshow = false;
	UINT m_slideshow_delay = 10000;
	UINT_PTR m_timer;

	UINT_PTR m_timerPreviewResize;

	CComPtr<IShellWindows> m_spWindows;

	BOOL Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

	void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy);
	void Cls_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	void Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	void Cls_OnTimer(HWND hwnd, UINT id);
	void Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
	void Cls_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys);
	void Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

	void DisplayImage(const WCHAR * szImagePath);
	void DisplayCurrentIndex();
	void CacheImagePaths();
	void OpenExplorer();
	void Rotate();
	void ToggleSlideshow();
	void ToggleScale();
	void ChangeZoom(int direction);
	void CopyImageToClipboard();

	bool NextImage();
	bool PreviousImage();
	bool GetFolderViewFromPath(const WCHAR *, IFolderView2 **);

public:

	ImageViewerMainWindow(const WCHAR * szCmdLine);

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) override;
};
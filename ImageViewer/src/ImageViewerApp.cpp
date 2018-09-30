#include <Windows.h>

#include "WindowClass.h"
#include "ImageViewerMainWindow.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, WCHAR * szCmdLine, int) {

	ImageViewerMainWindow main_window{szCmdLine};

	ShowWindow(main_window.m_window, SW_SHOW);
	UpdateWindow(main_window.m_window);

	main_window.run();

	return 0;
}
#pragma once

#include <Windows.h>

#define APPLICATION_SUBKEY	L"SOFTWARE\\Clifford\\ImageViewer"

struct ApplicationSettings {

	UINT		width	= 800;
	UINT		height	= 1200;
	UINT		x		= 100;
	UINT		y		= 100;

	ApplicationSettings();

	~ApplicationSettings();

};
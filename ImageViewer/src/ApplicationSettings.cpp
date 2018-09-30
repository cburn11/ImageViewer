#include <Windows.h>

#include "ApplicationSettings.h"

ApplicationSettings::ApplicationSettings() {
	
	HKEY	hApplicationKey;

	auto lRes = RegOpenKey(HKEY_CURRENT_USER, APPLICATION_SUBKEY, &hApplicationKey);
	if( lRes == ERROR_SUCCESS ) {

		DWORD size = sizeof(width);
		lRes = RegGetValue(hApplicationKey, nullptr, L"width", RRF_RT_REG_DWORD, 
			nullptr, &width, &size);

		lRes = RegGetValue(hApplicationKey, nullptr, L"height", RRF_RT_REG_DWORD,
			nullptr, &height, &size);

		lRes = RegGetValue(hApplicationKey, nullptr, L"x", RRF_RT_REG_DWORD,
			nullptr, &x, &size);

		lRes = RegGetValue(hApplicationKey, nullptr, L"y", RRF_RT_REG_DWORD,
			nullptr, &y, &size);

		RegCloseKey(hApplicationKey);

	} else if( ERROR_FILE_NOT_FOUND == lRes ) {

		//	Create Recent files for current user

		HKEY hPackageKey;
		lRes = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Clifford", &hPackageKey);

		RegCloseKey(hPackageKey);

		lRes = RegCreateKey(HKEY_CURRENT_USER, APPLICATION_SUBKEY, &hApplicationKey);

		RegCloseKey(hApplicationKey);
	}
}

ApplicationSettings::~ApplicationSettings() {

	HKEY	hApplicationKey;

	auto lRes = RegOpenKey(HKEY_CURRENT_USER, APPLICATION_SUBKEY, &hApplicationKey);
	if( lRes == ERROR_SUCCESS ) {

		lRes = RegSetValueEx(hApplicationKey, L"width", 0, REG_DWORD, (BYTE *) &width, sizeof(width));
		lRes = RegSetValueEx(hApplicationKey, L"height", 0, REG_DWORD, (BYTE *) &height, sizeof(height));
		lRes = RegSetValueEx(hApplicationKey, L"x", 0, REG_DWORD, (BYTE *) &x, sizeof(x));
		lRes = RegSetValueEx(hApplicationKey, L"y", 0, REG_DWORD, (BYTE *) &y, sizeof(y));

		RegCloseKey(hApplicationKey);
	}
}
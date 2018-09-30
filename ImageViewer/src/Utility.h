#pragma once

#include <Windows.h>

#include <shlobj.h>

#include <string>
using std::wstring;

wstring SanitizeCommandLine(const WCHAR * szCmdLine);

wstring GetDirFromPath(const WCHAR * szPath);

wstring GetFilenameFromPath(const WCHAR * szPath);

wstring GetExtensionFromFilename(const WCHAR * szFilename);
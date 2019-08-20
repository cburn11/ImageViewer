#include <Windows.h>

#include <shlobj.h>

#include <regex>
using std::wregex;
using std::wsmatch;
using std::regex_search;

#include <string>
using std::wstring;

wstring SanitizeCommandLine(const WCHAR * szCmdLine) {

	wstring cmdline{ szCmdLine };

	wregex	RegExp{ LR"(([A-Z]|[a-z]|:|\\|\.|\s|\d|-|_|,|%|\)|\()+)" };
	wsmatch	match;

	auto res = regex_search(cmdline, match, RegExp);
	if( res )
		return wstring{ match[0].first, match[0].second };
	else
		return L"";

}

wstring GetDirFromPath(const WCHAR * szPath) {

	if( !szPath ) return L"";

	auto cch = wcslen(szPath);
	auto szCachedPath = ( WCHAR * ) new WCHAR[cch + 1]{ 0 };
	if( !szCachedPath ) return L"";

	wcscpy_s(szCachedPath, cch + 1, szPath);

	wstring dir{ L"" };

	WCHAR * last_slash = wcsrchr(szCachedPath, L'\\');
	if( last_slash ) {
		*last_slash = L'\x00';
	}

	dir = szCachedPath;

	delete[] szCachedPath;

	return dir;
}

wstring GetFilenameFromPath(const WCHAR * szPath) {

	if( !szPath ) return L"";

	wstring filename{ L"" };

	const WCHAR * last_slash = wcsrchr(szPath, L'\\');

	if( last_slash ) {
		++last_slash;
		filename = last_slash;
	}

	return filename;
}

wstring GetExtensionFromFilename(const WCHAR * szFilename) {

	if( !szFilename )
		return wstring{ L"" };

	auto sz_ext = wcsrchr(szFilename, L'.');
	if( !sz_ext )
		return wstring{ L"" };

	return wstring{ sz_ext + 1 };
}
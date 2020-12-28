#include "pch.h"
#include "utils.h"

#include <algorithm>
#include <Shlwapi.h>

using namespace std;

string WideCharToUtf8(WCHAR* wcPath)
{
	CHAR mbPath[TC_MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, wcPath, static_cast<int>(wcslen(wcPath)) + 1, mbPath, TC_MAX_PATH, nullptr, nullptr);
	return string(mbPath);
}

string GetUtf8FileNameFromPath(WCHAR* wcPath)
{
	WCHAR wcName[TC_MAX_PATH];
	wcscpy_s(wcName, wcPath);
	PathStripPath(wcName);
	return WideCharToUtf8(wcName);
}

wstring GetStringResource(HINSTANCE hInst, UINT resourceId)
{
	WCHAR wcString[MAX_STRING_RESOURCE];
	LoadString(hInst, resourceId, wcString, MAX_STRING_RESOURCE);
	return wstring(wcString);
}

void OutputDebugFormat(LPCWSTR format, ...)
{
	WCHAR logMessage[MAX_LOG_LENGTH];

	va_list args;
	va_start(args, format);
	StringCchVPrintf(logMessage, MAX_LOG_LENGTH, format, args);
	va_end(args);
	
	OutputDebugString(logMessage);
}

string LocalPathToMegaPath(WCHAR* wcLocalPath)
{
	string str = WideCharToUtf8(wcLocalPath);
	std::replace(str.begin(), str.end(), '\\', '/');
	return str;
}

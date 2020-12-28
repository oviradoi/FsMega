#pragma once
#include <string>

std::string LocalPathToMegaPath(WCHAR* localPath);
std::string WideCharToUtf8(WCHAR* wcPath);
std::string GetUtf8FileNameFromPath(WCHAR* wcPath);
std::wstring GetStringResource(HINSTANCE hInst, UINT resourceId);
void OutputDebugFormat(LPCWSTR format, ...);
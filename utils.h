#pragma once
#include <string>

std::string LocalPathToMegaPath(const WCHAR* localPath);
std::string WideCharToUtf8(const std::wstring& wString);
std::string WideCharToUtf8(const WCHAR* wcPath);
std::string GetUtf8FileNameFromPath(const WCHAR* wcPath);
std::wstring GetStringResource(HINSTANCE hInst, UINT resourceId);
std::wstring ConstCharToWstring(const char* path);
void OutputDebugFormat(LPCWSTR format, ...);
std::string GetFsMegaTempPath();
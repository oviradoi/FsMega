#include "pch.h"

#include "FsMegaLogger.h"

#include <strsafe.h>

FsMegaLogger::FsMegaLogger()
{
	_mutex = CreateMutex(nullptr, FALSE, nullptr);
	mega::MegaApi::addLoggerObject(this);
}

FsMegaLogger::~FsMegaLogger()
{
	mega::MegaApi::removeLoggerObject(this);
	CloseHandle(_mutex);	
}

void FsMegaLogger::log(const char* time, int logLevel, const char* source, const char* message)
{
	WaitForSingleObject(_mutex, INFINITE);
	WCHAR logMessage[1024];
	WCHAR wcSource[1024];
	WCHAR wcMessage[1024];
	MultiByteToWideChar(CP_UTF8, 0, source, (int)strlen(source) + 1, wcSource, 1024);
	MultiByteToWideChar(CP_UTF8, 0, message, (int)strlen(message) + 1, wcMessage, 1024);
	StringCchPrintf(logMessage, 1024, _T("%s - %s\n"), wcSource, wcMessage);
	OutputDebugString(logMessage);	
	ReleaseMutex(_mutex);
}

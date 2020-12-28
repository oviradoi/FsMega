#include "pch.h"
#include "RequestListener.h"

#include <memory>

#include "utils.h"

#include <strsafe.h>

using namespace mega;

RequestListener::RequestListener(tProgressProcW progressProc, int pluginNr)
{
	OutputDebugFormat(_T("Creating request listener %p\n"), this);
	_errorCode = 0;	
	_finishedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	_progressEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	_progressProc = progressProc;
	_pluginNr = pluginNr;
	_progress = 0;
}

RequestListener::~RequestListener()
{
	OutputDebugFormat(_T("Destroying request listener %p\n"), this);
	CloseHandle(_finishedEvent);
	CloseHandle(_progressEvent);
}

void RequestListener::WaitAndNotify() const
{
	HANDLE handles[] = { _finishedEvent, _progressEvent };

	while (true)
	{
		const DWORD result = WaitForMultipleObjects(2, handles, FALSE, 500);
		if (result == WAIT_OBJECT_0)
		{
			break;
		}
		else if (result == WAIT_OBJECT_0 + 1 || result == WAIT_TIMEOUT)
		{
			_progressProc(_pluginNr, const_cast<WCHAR*>(_T("\\")), const_cast<WCHAR*>(_T("\\")), _progress);
		}
	}
}

bool RequestListener::HasError() const
{
	return _errorCode != 0;
}

void RequestListener::onRequestStart(MegaApi* api, MegaRequest* request)
{
	OutputDebugFormat(_T("onRequestStart type=%d\n"), request->getType());
}

void RequestListener::onRequestUpdate(MegaApi* api, MegaRequest* request)
{
	OutputDebugFormat(_T("onRequestUpdate type=%d\n"), request->getType());
	const double trans = static_cast<double>(request->getTransferredBytes());
	const double total = static_cast<double>(request->getTotalBytes());
	const int progress = static_cast<int>(100. * trans / total);

	UpdateProgress(progress);
}

void RequestListener::onRequestFinish(MegaApi* api, MegaRequest* request, MegaError* e)
{
	const int type = request->getType();
	_errorCode = e->getErrorCode();
	OutputDebugFormat(_T("onRequestFinish type=%d error=%d\n"), type, _errorCode);
	SetEvent(_finishedEvent);
}

void RequestListener::UpdateProgress(int newProgress)
{
	_progress = max(_progress, newProgress);

	OutputDebugFormat(_T("Update progress %p\n"), this);
	SetEvent(_progressEvent);
}

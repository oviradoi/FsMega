#include "pch.h"
#include "RequestListener.h"

#include <memory>

#include "utils.h"

#include <strsafe.h>

using namespace mega;

RequestListener::RequestListener(tProgressProcW progressProc, int pluginNr)
{
	OutputDebugFormat(_T("FsMega: Creating request listener %p\n"), this);
	_errorCode = 0;	
	_finishedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	_progressEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	_progressProc = progressProc;
	_pluginNr = pluginNr;
	_progress = 0;
	_aborted = false;
	_hasAccountInfo = false;
	_storageUsed = 0;
	_storageMax = 0;
}

RequestListener::~RequestListener()
{
	OutputDebugFormat(_T("FsMega: Destroying request listener %p\n"), this);
	CloseHandle(_finishedEvent);
	CloseHandle(_progressEvent);
}

void RequestListener::WaitAndNotify()
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
			std::wstring message = _errorMessage.empty() ? _T("\\") : _errorMessage.c_str();
			const int wantToAbort = _progressProc(_pluginNr, const_cast<WCHAR*>(message.c_str()), const_cast<WCHAR*>(_T("\\")), _progress);
			if (wantToAbort)
			{
				OutputDebugFormat(_T("FsMega: Aborting request in listener %p\n"), this);
				_aborted = true;
				SetEvent(_finishedEvent);
			}
		}
	}
}

bool RequestListener::HasError() const
{
	return _errorCode != 0;
}

bool RequestListener::WasAborted() const
{
	return _aborted;
}

bool RequestListener::HasAccountInfo() const
{
	return _hasAccountInfo;
}

long long RequestListener::GetStorageUsed() const
{
	return _storageUsed;
}

long long RequestListener::GetStorageMax() const
{
	return _storageMax;
}

std::wstring RequestListener::GetErrorMessage() const
{
	return _errorMessage;
}

void RequestListener::onRequestStart(MegaApi* api, MegaRequest* request)
{
	OutputDebugFormat(_T("FsMega: onRequestStart type=%d\n"), request->getType());
}

void RequestListener::onRequestUpdate(MegaApi* api, MegaRequest* request)
{
	OutputDebugFormat(_T("FsMega: onRequestUpdate type=%d\n"), request->getType());
	const double trans = static_cast<double>(request->getTransferredBytes());
	const double total = static_cast<double>(request->getTotalBytes());
	const int progress = static_cast<int>(100. * trans / total);

	UpdateProgress(progress);
}

void RequestListener::onRequestFinish(MegaApi* api, MegaRequest* request, MegaError* e)
{
	MegaAccountDetails* accountDetails = request->getMegaAccountDetails();
	if (accountDetails != nullptr)
	{
		_hasAccountInfo = true;
		_storageUsed = accountDetails->getStorageUsed();
		_storageMax = accountDetails->getStorageMax();
	}
	const int type = request->getType();
	_errorCode = e->getErrorCode();
	_errorMessage = ConstCharToWstring(e->getErrorString());
	OutputDebugFormat(_T("FsMega: onRequestFinish type=%d errorCode=%d errorString=%s\n"), type, _errorCode, _errorMessage.c_str());
	SetEvent(_finishedEvent);
}

void RequestListener::onRequestTemporaryError(MegaApi* api, MegaRequest* request, MegaError* error)
{
	const int type = request->getType();
	const int retry = request->getNumRetry();
	_errorCode = error->getErrorCode();
	_errorMessage = ConstCharToWstring(error->getErrorString());
	OutputDebugFormat(_T("FsMega: onRequestTemporaryError type=%d errorCode=%d errorString=%s retry=%d\n"), type, _errorCode, _errorMessage.c_str(), retry);
	SetEvent(_progressEvent);
}

void RequestListener::UpdateProgress(int newProgress)
{
	_progress = max(_progress, newProgress);

	OutputDebugFormat(_T("FsMega: Update progress %p\n"), this);
	SetEvent(_progressEvent);
}

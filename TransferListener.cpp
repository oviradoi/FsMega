#include "pch.h"
#include "TransferListener.h"
#include "utils.h"

#include <strsafe.h>
#include <memory>

using namespace std;
using namespace mega;

TransferListener::TransferListener(tProgressProcW progressProc, int pluginNr, WCHAR* localName, WCHAR* remoteName)
{
	OutputDebugFormat(_T("FsMega: Creating transfer listener %p\n"), this);
	
	_progressEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	_finishedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	_progressProc = progressProc;
	_pluginNr = pluginNr;
	_localName = localName;
	_remoteName = remoteName;
	_progress = -1;
	_cancelled = false;
	_failed = false;
	_readError = false;
	_writeError = false;
	_api = nullptr;
	_transferTag = 0;
}

TransferListener::~TransferListener()
{
	OutputDebugFormat(_T("FsMega: Destroying transfer listener %p\n"), this);
	CloseHandle(_progressEvent);
	CloseHandle(_finishedEvent);
}

void TransferListener::WaitAndNotify() const
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
			const int wantToAbort = _progressProc(_pluginNr, _localName, _remoteName, _progress);
			if (wantToAbort == 1 && _api != nullptr && _transferTag != -1)
			{
				unique_ptr<MegaTransfer> transfer(_api->getTransferByTag(_transferTag));
				if (transfer != nullptr)
				{
					_api->cancelTransfer(transfer.get());
				}
			}
		}
	}
}

bool TransferListener::WasCancelled() const
{
	return _cancelled;
}

bool TransferListener::HasFailed() const
{
	return _failed;
}

bool TransferListener::HasReadError() const
{
	return _readError;
}

bool TransferListener::HasWriteError() const
{
	return _writeError;
}

void TransferListener::onTransferStart(MegaApi* api, MegaTransfer* transfer)
{
	_api = api;
	_transferTag = transfer->getTag();
	UpdateProgress(0);

	OutputDebugFormat(_T("FsMega: onTransferStart listener %p\n"), this);
}

void TransferListener::onTransferUpdate(MegaApi* api, MegaTransfer* transfer)
{
	const double trans = static_cast<double>(transfer->getTransferredBytes());
	const double total = static_cast<double>(transfer->getTotalBytes());
	const int progress = static_cast<int>(100. * trans / total);

	UpdateProgress(progress);
}

void TransferListener::onTransferFinish(MegaApi* api, MegaTransfer* transfer, MegaError* error)
{
	_failed = transfer->getState() == MegaTransfer::STATE_FAILED;
	_cancelled = transfer->getState() == MegaTransfer::STATE_CANCELLED;
	_readError = error->getErrorCode() == MegaError::API_EREAD;
	_writeError = error->getErrorCode() == MegaError::API_EWRITE;
	OutputDebugFormat(_T("FsMega: onTransferFinish listener %p\n"), this);
	UpdateProgress(100);
	SetEvent(_finishedEvent);
}

void TransferListener::UpdateProgress(int newProgress)
{
	_progress = max(_progress, newProgress);

	OutputDebugFormat(_T("FsMega: Update progress %p\n"), this);
	SetEvent(_progressEvent);
}

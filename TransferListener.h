#pragma once

#include "megaapi.h"

class TransferListener final : public mega::MegaTransferListener
{
public:
	TransferListener(tProgressProcW progressProc, int pluginNr, WCHAR* localName, WCHAR* remoteName);
	TransferListener(TransferListener&) = delete;
	TransferListener(TransferListener&&) = delete;
	TransferListener& operator=(const TransferListener&) = delete;
	TransferListener& operator=(const TransferListener&&) = delete;
	~TransferListener() override;

	void WaitAndNotify() const;
	bool WasCancelled() const;
	bool HasFailed() const;
	bool HasReadError() const;
	bool HasWriteError() const;
private:
	void onTransferStart(mega::MegaApi* api, mega::MegaTransfer* transfer) override;
	void onTransferUpdate(mega::MegaApi* api, mega::MegaTransfer* transfer) override;
	void onTransferFinish(mega::MegaApi* api, mega::MegaTransfer* transfer, mega::MegaError* error) override;
	void UpdateProgress(int newProgress);
	
private:
	HANDLE _progressEvent;
	HANDLE _finishedEvent;
	tProgressProcW _progressProc;
	int _pluginNr;
	WCHAR* _localName;
	WCHAR* _remoteName;
	volatile long _progress;
	bool _cancelled;
	bool _failed;
	bool _readError;
	bool _writeError;

	mega::MegaApi* _api;
	int _transferTag;
};


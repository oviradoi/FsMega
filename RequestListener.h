#pragma once
#include "megaapi.h"

class RequestListener final : public mega::MegaRequestListener
{
public:
	RequestListener(tProgressProcW progressProc, int pluginNr);
	RequestListener(RequestListener&) = delete;
	RequestListener(RequestListener&&) = delete;
	RequestListener& operator=(const RequestListener&) = delete;
	RequestListener& operator=(const RequestListener&&) = delete;
	~RequestListener() override;

	void WaitAndNotify();
	bool HasError() const;
	bool WasAborted() const;
	bool HasAccountInfo() const;
	long long GetStorageUsed() const;
	long long GetStorageMax() const;
	std::wstring GetErrorMessage() const;

private:
	void onRequestStart(mega::MegaApi* api, mega::MegaRequest* request) override;
	void onRequestUpdate(mega::MegaApi* api, mega::MegaRequest* request) override;
	void onRequestFinish(mega::MegaApi* api, mega::MegaRequest* request, mega::MegaError* e) override;
	void onRequestTemporaryError(mega::MegaApi* api, mega::MegaRequest* request, mega::MegaError* error) override;
	void UpdateProgress(int newProgress);

private:
	HANDLE _progressEvent;
	HANDLE _finishedEvent;
	tProgressProcW _progressProc;
	int _pluginNr;
	int _errorCode;
	bool _aborted;
	bool _hasAccountInfo;
	long long _storageUsed;
	long long _storageMax;
	std::wstring _errorMessage;
	volatile int _progress;
};


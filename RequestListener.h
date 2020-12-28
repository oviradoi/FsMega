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

	void WaitAndNotify() const;
	bool HasError() const;

private:
	void onRequestStart(mega::MegaApi* api, mega::MegaRequest* request) override;
	void onRequestUpdate(mega::MegaApi* api, mega::MegaRequest* request) override;
	void onRequestFinish(mega::MegaApi* api, mega::MegaRequest* request, mega::MegaError* e) override;
	void UpdateProgress(int newProgress);

private:
	HANDLE _progressEvent;
	HANDLE _finishedEvent;
	tProgressProcW _progressProc;
	int _pluginNr;
	int _errorCode;
	volatile int _progress;
};


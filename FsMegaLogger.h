#pragma once
#include "megaapi.h"

class FsMegaLogger final : public mega::MegaLogger
{
public:
	FsMegaLogger();
	FsMegaLogger(FsMegaLogger&) = delete;
	FsMegaLogger(FsMegaLogger&&) = delete;
	FsMegaLogger& operator=(const FsMegaLogger&) = delete;
	FsMegaLogger& operator=(const FsMegaLogger&&) = delete;
	~FsMegaLogger() override;
	void log(const char* time, int logLevel, const char* source, const char* message) override;
private:
	HANDLE _mutex;
};

#pragma once

#include "megaapi.h"
#include "Enumerator.h"

class CFsMega final
{
public:
	CFsMega();
	~CFsMega();
	CFsMega(CFsMega&) = delete;
	CFsMega(CFsMega&&) = delete;
	CFsMega& operator=(const CFsMega&) = delete;
	CFsMega& operator=(const CFsMega&&) = delete;
	
	void Init(HINSTANCE pluginInstance, int pluginNr, tProgressProcW progressProc, tLogProcW logProc, tRequestProcW requestProc);
	void Disconnect();

	Enumerator* GetEnumerator(WCHAR* path);
	BOOL MkDir(WCHAR* path);
	BOOL Delete(WCHAR* path);
	int UploadFile(WCHAR* localName, WCHAR* remoteName, int copyFlags);
	int DownloadFile(WCHAR* localName, WCHAR* remoteName, int copyFlags);
	int RenameMove(WCHAR* oldName, WCHAR* newName, bool move, bool overwrite);
	void ShowAboutDialog(HWND mainWnd);
	void SetDefaultIniFilename(const char* defaultIniName);
	void SetCryptCallback(tCryptProcW pCryptProc, int cryptoNr, int cryptoFlags);

private:
	void InitializeMegaApi();
	void Connect();
	bool FileExists(WCHAR* path);
	std::unique_ptr<mega::MegaNode> GetParentNode(WCHAR* path);
	void LogMessage(int msgType, const WCHAR* format, ...) const;

private:
	HINSTANCE _pluginInstance;
	int _pluginNr;
	tProgressProcW _progressProc;
	tLogProcW _logProc;
	tCryptProcW _cryptProc;
	int _cryptoNr;
	int _cryptoFlags;
	tRequestProcW _requestProc;
	std::wstring _iniPath;

	mega::MegaApi* _megaApi;
};

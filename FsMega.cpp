#include "pch.h"
#include "FsMega.h"
#include "utils.h"

#include <algorithm>
#include <format>

#include "AboutDialog.h"
#include "LoginDialog.h"
#include "RequestListener.h"
#include "resource.h"
#include "TransferListener.h"

using namespace std;
using namespace mega;

constexpr TCHAR DefaultIniFilename[MAX_PATH] = _T("FsMega.ini");

CFsMega::CFsMega()
{
	_pluginInstance = nullptr;
	_pluginNr = -1;
	_progressProc = nullptr;
	_logProc = nullptr;
	_requestProc = nullptr;
	_iniPath = DefaultIniFilename;
	_megaApi = nullptr;
	_cryptProc = nullptr;
	_cryptoNr = 0;
	_cryptoFlags = 0;
	_lastQuotaRefresh = 0;
	_lastStorageUsed = 0;
	_lastStorageMax = 0;
}

CFsMega::~CFsMega()
{
	if (_megaApi != nullptr)
	{
		delete _megaApi;
		_megaApi = nullptr;
	}
}

void CFsMega::InitializeMegaApi()
{
	if (_megaApi != nullptr)
	{
		delete _megaApi;
		_megaApi = nullptr;
	}

	const string tempPath = GetFsMegaTempPath();
	_megaApi = new MegaApi(MEGA_API_KEY, tempPath.c_str());
}

void CFsMega::Init(HINSTANCE pluginInstance, int pluginNr, tProgressProcW progressProc, tLogProcW logProc, tRequestProcW requestProc)
{
	_pluginInstance = pluginInstance;
	_pluginNr = pluginNr;
	_progressProc = progressProc;
	_logProc = logProc;
	_requestProc = requestProc;

	InitializeMegaApi();
}

void CFsMega::Connect()
{
	HWND mainWnd = GetActiveWindow();
	CLoginDialog loginDialog(_pluginInstance, _iniPath, _pluginNr, _cryptProc, _cryptoNr, _cryptoFlags);
	INT_PTR result = loginDialog.ShowLoginDialog(mainWnd);

	if (result != IDOK)
	{
		return;
	}

	const string username = WideCharToUtf8(loginDialog.GetUsername());
	const string password = WideCharToUtf8(loginDialog.GetPassword());
	const string multifactorKey = WideCharToUtf8(loginDialog.GetMultifactorKey());

	_lastQuotaRefresh = 0;
	_lastStorageMax = 0;
	_lastStorageUsed = 0;

	RequestListener loginListener(_progressProc, _pluginNr);
	if (multifactorKey.empty())
	{
		_megaApi->login(username.c_str(), password.c_str(), &loginListener);
	}
	else
	{
		_megaApi->multiFactorAuthLogin(username.c_str(), password.c_str(), multifactorKey.c_str(), &loginListener);
	}

	loginListener.WaitAndNotify();
	if (loginListener.HasError() || loginListener.WasAborted())
	{
		const wstring pluginName = GetStringResource(_pluginInstance, IDS_PLUGINNAME);
		const wstring errorMessage = loginListener.GetErrorMessage();

		if (!errorMessage.empty())
		{
			wstring loginErrorDetail = GetStringResource(_pluginInstance, IDS_LOGINERRORDETAIL);
			wstring formattedMessage = vformat(loginErrorDetail, make_wformat_args(errorMessage));
			LogMessage(MSGTYPE_IMPORTANTERROR, formattedMessage.c_str());
			MessageBox(GetActiveWindow(), formattedMessage.c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);			
		}
		else
		{
			const wstring loginError = GetStringResource(_pluginInstance, IDS_LOGINERROR);
			LogMessage(MSGTYPE_IMPORTANTERROR, loginError.c_str());
			MessageBox(GetActiveWindow(), loginError.c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);			
		}
		
		InitializeMegaApi();
	}
	else
	{
		RequestListener fetchNodesListener(_progressProc, _pluginNr);
		_megaApi->fetchNodes(&fetchNodesListener);
		fetchNodesListener.WaitAndNotify();

		if (fetchNodesListener.HasError() || fetchNodesListener.WasAborted())
		{
			const wstring pluginName = GetStringResource(_pluginInstance, IDS_PLUGINNAME);
			const wstring fetchNodesError = GetStringResource(_pluginInstance, IDS_FETCHNODESERROR);
			MessageBox(GetActiveWindow(), fetchNodesError.c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
			LogMessage(MSGTYPE_IMPORTANTERROR, fetchNodesError.c_str());
			InitializeMegaApi();
		}
		else
		{
			LogMessage(MSGTYPE_CONNECT, _T("CONNECT /"));
		}
	}
}

void CFsMega::Disconnect() const
{
	RequestListener logoutListener(_progressProc, _pluginNr);
	_megaApi->logout(&logoutListener);
	logoutListener.WaitAndNotify();
	LogMessage(MSGTYPE_DISCONNECT, _T("Disconnected"));
}

void CFsMega::LogStorageQuota()
{
	// refresh every 60 seconds
	if (GetTickCount64() - _lastQuotaRefresh > 60000)
	{
		RequestListener accountDetailsListener(_progressProc, _pluginNr);
		_megaApi->getSpecificAccountDetails(true, false, false, -1, &accountDetailsListener);
		accountDetailsListener.WaitAndNotify();

		if (accountDetailsListener.HasAccountInfo())
		{
			_lastQuotaRefresh = GetTickCount64();
			_lastStorageUsed = accountDetailsListener.GetStorageUsed();
			_lastStorageMax = accountDetailsListener.GetStorageMax();
		}
		else
		{
			_lastStorageUsed = _lastStorageMax = 0;
		}
	}

	WCHAR wcStorageUsed[MAX_PATH];
	WCHAR wcStorageMax[MAX_PATH];
	WCHAR wcStorageFree[MAX_PATH];
	StrFormatByteSize64(_lastStorageUsed, wcStorageUsed, MAX_PATH);
	StrFormatByteSize64(_lastStorageMax, wcStorageMax, MAX_PATH);
	StrFormatByteSize64(_lastStorageMax - _lastStorageUsed, wcStorageFree, MAX_PATH);
	LogMessage(MSGTYPE_DETAILS, _T("Space free: %s used: %s total: %s"), wcStorageFree, wcStorageUsed, wcStorageMax);
}


Enumerator* CFsMega::GetEnumerator(WCHAR* path)
{
	if (!_megaApi->isLoggedIn())
	{
		Connect();
	}

	if (_megaApi->isLoggedIn())
	{
		LogMessage(MSGTYPE_DETAILS, _T("LIST %s"), path);
		LogStorageQuota();

		const string megaPath = LocalPathToMegaPath(path);
		MegaNode* node = _megaApi->getNodeByPath(megaPath.c_str());
		if (node != nullptr)
		{
			MegaNodeList* children = _megaApi->getChildren(node);
			return new Enumerator(children);
		}
	}
	return nullptr;
}

BOOL CFsMega::MkDir(WCHAR* path)
{
	const string strName = GetUtf8FileNameFromPath(path);
	
	const unique_ptr<MegaNode> parent = GetParentNode(path);
	if (parent)
	{
		RequestListener createFolderListener(_progressProc, _pluginNr);
		_megaApi->createFolder(strName.c_str(), parent.get(), &createFolderListener);
		createFolderListener.WaitAndNotify();
		_lastQuotaRefresh = 0;

		LogMessage(MSGTYPE_OPERATIONCOMPLETE, _T("MKDIR %s"), path);

		return TRUE;
	}

	return FALSE;
}

BOOL CFsMega::Delete(WCHAR* path)
{
	const string str = LocalPathToMegaPath(path);
	const unique_ptr<MegaNode> node(_megaApi->getNodeByPath(str.c_str()));
	if (node)
	{
		RequestListener deleteListener(_progressProc, _pluginNr);
		_megaApi->remove(node.get(), &deleteListener);
		deleteListener.WaitAndNotify();
		_lastQuotaRefresh = 0;

		LogMessage(MSGTYPE_OPERATIONCOMPLETE, _T("DELETE %s"), path);
		
		return TRUE;
	}

	return FALSE;
}

bool CFsMega::FileExists(WCHAR* path) const
{
	const string str = LocalPathToMegaPath(path);
	const unique_ptr<MegaNode> node(_megaApi->getNodeByPath(str.c_str()));
	return node != nullptr;
}

unique_ptr<MegaNode> CFsMega::GetParentNode(WCHAR* path) const
{
	WCHAR wcPath[TC_MAX_PATH];
	wcscpy_s(wcPath, path);

	PathRemoveFileSpec(wcPath);

	const string strPath = LocalPathToMegaPath(wcPath);

	return unique_ptr<MegaNode>(_megaApi->getNodeByPath(strPath.c_str()));
}

void CFsMega::LogMessage(int msgType, const WCHAR* format, ...) const
{
	WCHAR logMessage[MAX_LOG_LENGTH];
	va_list args;
	va_start(args, format);
	StringCchVPrintf(logMessage, MAX_LOG_LENGTH, format, args);
	va_end(args);
	
	_logProc(_pluginNr, msgType, logMessage);
}

int CFsMega::UploadFile(WCHAR* localName, WCHAR* remoteName, int copyFlags)
{
	if (FileExists(remoteName) && !(copyFlags & FS_COPYFLAGS_OVERWRITE))
	{
		return FS_FILE_EXISTS;
	}
	
	const string strLocalName = WideCharToUtf8(localName);
	const string strRemoteName = GetUtf8FileNameFromPath(remoteName);

	const unique_ptr<MegaNode> parent = GetParentNode(remoteName);
	if (parent)
	{
		TransferListener listener(_progressProc, _pluginNr, localName, remoteName);
		_megaApi->startUpload(strLocalName.c_str(), parent.get(), strRemoteName.c_str(), MegaApi::INVALID_CUSTOM_MOD_TIME, nullptr, false, false, nullptr, &listener);
		listener.WaitAndNotify();

		_lastQuotaRefresh = 0;

		if (listener.WasCancelled())
		{
			return FS_FILE_USERABORT;
		}
		if (listener.HasFailed())
		{
			if (listener.HasReadError())
			{
				return FS_FILE_READERROR;
			}

			return FS_FILE_WRITEERROR;
		}

		LogMessage(MSGTYPE_TRANSFERCOMPLETE, _T("Upload complete: %s -> %s"), localName, remoteName);

		if (copyFlags & FS_COPYFLAGS_MOVE)
		{
			if (!DeleteFile(localName))
			{
				return FS_FILE_WRITEERROR;
			}
		}

		return FS_FILE_OK;
	}

	OutputDebugString(_T("Could not upload\n"));
	return FS_FILE_WRITEERROR;
}

int CFsMega::DownloadFile(WCHAR* localName, WCHAR* remoteName, int copyFlags)
{
	const bool fileExists = PathFileExists(localName);

	if (fileExists && !(copyFlags & FS_COPYFLAGS_OVERWRITE))
	{
		return FS_FILE_EXISTS;
	}
	
	const string strLocalName = WideCharToUtf8(localName);
	const string strRemoteName = LocalPathToMegaPath(remoteName);

	const unique_ptr<MegaNode> node(_megaApi->getNodeByPath(strRemoteName.c_str()));
	if (node)
	{
		TransferListener listener(_progressProc, _pluginNr, localName, remoteName);
		_megaApi->startDownload(node.get(), strLocalName.c_str(), nullptr, nullptr, false, nullptr, MegaTransfer::COLLISION_CHECK_ASSUMESAME, MegaTransfer::COLLISION_RESOLUTION_OVERWRITE, false, &listener);
		listener.WaitAndNotify();

		if (listener.WasCancelled())
		{
			return FS_FILE_USERABORT;
		}
		if (listener.HasFailed())
		{
			if (listener.HasReadError())
			{
				return FS_FILE_READERROR;
			}

			return FS_FILE_WRITEERROR;
		}

		LogMessage(MSGTYPE_TRANSFERCOMPLETE, _T("Download complete: %s -> %s"), remoteName, localName);

		if (copyFlags & FS_COPYFLAGS_MOVE)
		{
			if (!Delete(remoteName))
			{
				return FS_FILE_READERROR;
			}
		}

		return FS_FILE_OK;
	}

	return FS_FILE_READERROR;
}

int CFsMega::RenameMove(WCHAR* oldName, WCHAR* newName, bool move, bool overwrite)
{
	if (FileExists(newName) && !overwrite)
	{
		return FS_FILE_EXISTS;
	}

	const string strOldName = LocalPathToMegaPath(oldName);
	const unique_ptr<MegaNode> oldNode(_megaApi->getNodeByPath(strOldName.c_str()));
	const string strNewName = GetUtf8FileNameFromPath(newName);
	const unique_ptr<MegaNode> oldParent(GetParentNode(oldName));
	const unique_ptr<MegaNode> newParent(GetParentNode(newName));

	if (!oldNode)
	{
		return FS_FILE_NOTFOUND;
	}

	if (!newParent)
	{
		return FS_FILE_WRITEERROR;
	}

	RequestListener listener(_progressProc, _pluginNr);

	if (move)
	{
		if (oldParent->getHandle() == newParent->getHandle())
		{
			_megaApi->renameNode(oldNode.get(), strNewName.c_str(), &listener);
		}
		else
		{
			_megaApi->moveNode(oldNode.get(), newParent.get(), strNewName.c_str(), &listener);
		}
	}
	else
	{
		_megaApi->copyNode(oldNode.get(), newParent.get(), strNewName.c_str(), &listener);
	}

	listener.WaitAndNotify();

	_lastQuotaRefresh = 0;

	LogMessage(MSGTYPE_OPERATIONCOMPLETE, _T("Rename/move complete: %s -> %s"), oldName, newName);

	return listener.HasError() ? FS_FILE_WRITEERROR : FS_FILE_OK;
}

void CFsMega::ShowAboutDialog(HWND mainWnd) const
{
	CAboutDialog aboutDialog(_pluginInstance, ConstCharToWstring(_megaApi->getVersion()));
	aboutDialog.ShowAboutDialog(mainWnd);
}

void CFsMega::SetDefaultIniFilename(const char* defaultIniName)
{
	WCHAR fullIniPath[MAX_PATH];
	MultiByteToWideChar(CP_ACP, NULL, defaultIniName, MAX_PATH, fullIniPath, MAX_PATH);
	PathRemoveFileSpec(fullIniPath);
	PathCombine(fullIniPath, fullIniPath, DefaultIniFilename);
	_iniPath = fullIniPath;
}

void CFsMega::SetCryptCallback(tCryptProcW pCryptProc, int cryptoNr, int cryptoFlags)
{
	_cryptProc = pCryptProc;
	_cryptoNr = cryptoNr;
	_cryptoFlags = cryptoFlags;
}

#include "pch.h"
#include "FsMega.h"
#include "utils.h"

#include <algorithm>

#include "RequestListener.h"
#include "resource.h"
#include "TransferListener.h"

using namespace std;
using namespace mega;

CFsMega::CFsMega() :_megaApi(MEGA_API_KEY)
{
	_pluginInstance = nullptr;
	_pluginNr = -1;
	_progressProc = nullptr;
	_logProc = nullptr;
	_requestProc = nullptr;
}

void CFsMega::Init(HINSTANCE pluginInstance, int pluginNr, tProgressProcW progressProc, tLogProcW logProc, tRequestProcW requestProc)
{
	_pluginInstance = pluginInstance;
	_pluginNr = pluginNr;
	_progressProc = progressProc;
	_logProc = logProc;
	_requestProc = requestProc;
}

void CFsMega::Connect()
{
	WCHAR wcUsername[MAX_PATH];
	WCHAR wcPassword[MAX_PATH];
	wcUsername[0] = 0;
	wcPassword[0] = 0;
	
	const BOOL hasUsername = _requestProc(_pluginNr, RT_UserName, nullptr, nullptr, wcUsername, MAX_PATH);
	const BOOL hasPassword = hasUsername && _requestProc(_pluginNr, RT_Password, nullptr, nullptr, wcPassword, MAX_PATH);

	if (!hasUsername || !hasPassword)
	{
		return;
	}

	const string username = WideCharToUtf8(wcUsername);
	const string password = WideCharToUtf8(wcPassword);

	RequestListener loginListener(_progressProc, _pluginNr);
	_megaApi.login(username.c_str(), password.c_str(), &loginListener);
	loginListener.WaitAndNotify();
	if (loginListener.HasError())
	{
		const wstring pluginName = GetStringResource(_pluginInstance, IDS_PLUGINNAME);
		const wstring loginError = GetStringResource(_pluginInstance, IDS_LOGINERROR);
		MessageBox(GetActiveWindow(), loginError.c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
	}
	else
	{
		LogMessage(MSGTYPE_CONNECT, _T("CONNECT /"));

		RequestListener fetchNodesListener(_progressProc, _pluginNr);
		_megaApi.fetchNodes(&fetchNodesListener);
		fetchNodesListener.WaitAndNotify();

		LogMessage(MSGTYPE_CONNECTCOMPLETE, _T("Connected"));
	}
}

void CFsMega::Disconnect()
{
	RequestListener logoutListener(_progressProc, _pluginNr);
	_megaApi.logout(&logoutListener);
	logoutListener.WaitAndNotify();
	LogMessage(MSGTYPE_DISCONNECT, _T("Disconnected"));
}

Enumerator* CFsMega::GetEnumerator(WCHAR* path)
{
	if (!_megaApi.isLoggedIn())
	{
		Connect();
	}
	if (_megaApi.isLoggedIn())
	{
		LogMessage(MSGTYPE_DETAILS, _T("LIST %s"), path);

		const string megaPath = LocalPathToMegaPath(path);
		MegaNode* node = _megaApi.getNodeByPath(megaPath.c_str());
		if (node != nullptr)
		{
			MegaNodeList* children = _megaApi.getChildren(node);
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
		_megaApi.createFolder(strName.c_str(), parent.get(), &createFolderListener);
		createFolderListener.WaitAndNotify();

		LogMessage(MSGTYPE_OPERATIONCOMPLETE, _T("MKDIR %s"), path);

		return TRUE;
	}

	return FALSE;
}

BOOL CFsMega::Delete(WCHAR* path)
{
	const string str = LocalPathToMegaPath(path);
	const unique_ptr<MegaNode> node(_megaApi.getNodeByPath(str.c_str()));
	if (node)
	{
		RequestListener deleteListener(_progressProc, _pluginNr);
		_megaApi.remove(node.get(), &deleteListener);
		deleteListener.WaitAndNotify();

		LogMessage(MSGTYPE_OPERATIONCOMPLETE, _T("DELETE %s"), path);
		
		return TRUE;
	}

	return FALSE;
}

bool CFsMega::FileExists(WCHAR* path)
{
	const string str = LocalPathToMegaPath(path);
	const unique_ptr<MegaNode> node(_megaApi.getNodeByPath(str.c_str()));
	return node != nullptr;
}

unique_ptr<MegaNode> CFsMega::GetParentNode(WCHAR* path)
{
	WCHAR wcPath[TC_MAX_PATH];
	wcscpy_s(wcPath, path);

	PathRemoveFileSpec(wcPath);

	const string strPath = LocalPathToMegaPath(wcPath);

	return unique_ptr<MegaNode>(_megaApi.getNodeByPath(strPath.c_str()));
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
		_megaApi.startUpload(strLocalName.c_str(), parent.get(), strRemoteName.c_str(), &listener);
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

	const unique_ptr<MegaNode> node(_megaApi.getNodeByPath(strRemoteName.c_str()));
	if (node)
	{
		TransferListener listener(_progressProc, _pluginNr, localName, remoteName);
		_megaApi.startDownload(node.get(), strLocalName.c_str(), &listener);
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
	const unique_ptr<MegaNode> oldNode(_megaApi.getNodeByPath(strOldName.c_str()));
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
			_megaApi.renameNode(oldNode.get(), strNewName.c_str(), &listener);
		}
		else
		{
			_megaApi.moveNode(oldNode.get(), newParent.get(), strNewName.c_str(), &listener);
		}
	}
	else
	{
		_megaApi.copyNode(oldNode.get(), newParent.get(), strNewName.c_str(), &listener);
	}

	listener.WaitAndNotify();

	LogMessage(MSGTYPE_OPERATIONCOMPLETE, _T("Rename/move complete: %s -> %s"), oldName, newName);

	return listener.HasError() ? FS_FILE_WRITEERROR : FS_FILE_OK;
}

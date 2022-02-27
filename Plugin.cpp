#include "pch.h"

#include "resource.h"

#include "FsMega.h"
#include "FsMegaLogger.h"
#include "utils.h"

//FsMegaLogger megaLogger;
CFsMega* fsMega = nullptr;

int __stdcall FsInitW(int PluginNr, tProgressProcW pProgressProcW, tLogProcW pLogProcW, tRequestProcW pRequestProcW)
{
    HMODULE hModule;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(&fsMega), &hModule);
    fsMega = new CFsMega();
    fsMega->Init(hModule, PluginNr, pProgressProcW, pLogProcW, pRequestProcW);
    return 0;
}

void __stdcall FsGetDefRootName(char* DefRootName, int maxlen)
{
    HMODULE hModule;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(&fsMega), &hModule);
    LoadStringA(hModule, IDS_PLUGINNAME, DefRootName, maxlen);
}

HANDLE __stdcall FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW* FindData)
{
    Enumerator* enumerator = fsMega->GetEnumerator(Path);
    if (enumerator != nullptr)
    {
        if (enumerator->FindFirst(FindData))
        {
            return enumerator;
        }
        SetLastError(ERROR_NO_MORE_FILES);
        delete enumerator;
        return INVALID_HANDLE_VALUE;
    }

    SetLastError(ERROR_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

BOOL __stdcall FsFindNextW(HANDLE Hdl, WIN32_FIND_DATAW* FindData)
{
    Enumerator* enumerator = static_cast<Enumerator*>(Hdl);
    return enumerator->FindNext(FindData);
}

int __stdcall FsFindClose(HANDLE Hdl)
{
    Enumerator* enumerator = static_cast<Enumerator*>(Hdl);
    delete enumerator;
    return 0;
}

BOOL __stdcall FsDisconnectW(WCHAR* DisconnectRoot)
{
    fsMega->Disconnect();
    return TRUE;
}

BOOL __stdcall FsMkDirW(WCHAR* Path)
{
    return fsMega->MkDir(Path);
}

BOOL __stdcall FsRemoveDirW(WCHAR* RemoteName)
{
    return fsMega->Delete(RemoteName);
}

BOOL __stdcall FsDeleteFileW(WCHAR* RemoteName)
{
    return fsMega->Delete(RemoteName);
}

int __stdcall FsPutFileW(WCHAR* LocalName, WCHAR* RemoteName, int CopyFlags)
{
    return fsMega->UploadFile(LocalName, RemoteName, CopyFlags);
}

int __stdcall FsGetFileW(WCHAR* RemoteName, WCHAR* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
    return fsMega->DownloadFile(LocalName, RemoteName, CopyFlags);
}

int __stdcall FsGetBackgroundFlags()
{
    return BG_DOWNLOAD | BG_UPLOAD;
}

int __stdcall FsRenMovFileW(WCHAR* OldName, WCHAR* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
    return fsMega->RenameMove(OldName, NewName, Move, OverWrite);
}

int __stdcall FsInit(int PluginNr, tProgressProc pProgressProcW, tLogProc pLogProcW, tRequestProc pRequestProcW)
{
    return -1;
}

HANDLE __stdcall FsFindFirst(char* Path, WIN32_FIND_DATA* FindData)
{
    SetLastError(ERROR_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

BOOL __stdcall FsFindNext(HANDLE Hdl, WIN32_FIND_DATA* FindData)
{
    return FALSE;
}

int __stdcall FsExecuteFileW(HWND MainWin, WCHAR* RemoteName, WCHAR* Verb)
{
    std::wstring verb(Verb);
    if (verb == std::wstring(_T("properties")))
    {
        fsMega->ShowAboutDialog(MainWin);
        return FS_EXEC_OK;
    }
    return FS_EXEC_YOURSELF;
}

void __stdcall FsSetDefaultParams(FsDefaultParamStruct* dps)
{
    fsMega->SetDefaultIniFilename(dps->DefaultIniName);
}

void __stdcall FsSetCryptCallbackW(tCryptProcW pCryptProcW, int CryptoNr, int Flags)
{
    fsMega->SetCryptCallback(pCryptProcW, CryptoNr, Flags);
}
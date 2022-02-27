#include "pch.h"
#include "utils.h"
#include "resource.h"
#include "AboutDialog.h"

#include <format>

using namespace std;

CAboutDialog::CAboutDialog(HINSTANCE pluginInstance, wstring megaVersion)
{
	_pluginInstance = pluginInstance;
	_megaVersion = megaVersion;
}

INT_PTR CAboutDialog::ShowAboutDialog(HWND mainWnd)
{
	return DialogBoxParam(_pluginInstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), mainWnd, AboutDlgProc , (LPARAM)this);
}

INT_PTR CALLBACK CAboutDialog::AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		CAboutDialog* lpThis = (CAboutDialog*)lParam;
		lpThis->InitDialog(hDlg);
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}

void CAboutDialog::InitDialog(HWND hDlg)
{
	// Load resources
	wstring pluginName = GetStringResource(_pluginInstance, IDS_PLUGINNAME);
	wstring caption = GetStringResource(_pluginInstance, IDS_ABOUTCAPTION);
	wstring title = GetStringResource(_pluginInstance, IDS_ABOUTTITLE);
	// Format caption
	wstring formattedCaption = format(caption, pluginName);
	SetDlgItemText(hDlg, IDC_STATIC_CAPTION, formattedCaption.c_str());
	// Format title
	wstring formattedTitle = format(title, formattedCaption);
	SetWindowText(hDlg, formattedTitle.c_str());
	// Load icon
	HICON hIcon = (HICON)LoadImage(_pluginInstance, MAKEINTRESOURCE(IDI_MEGA), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED);
	SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	SetVersionString(hDlg);
	SetBuiltUsingString(hDlg);
}

void CAboutDialog::SetVersionString(HWND hDlg)
{
	TCHAR moduleFileName[MAX_PATH];
	GetModuleFileName(_pluginInstance, moduleFileName, MAX_PATH);
	DWORD dummyZero;
	DWORD versionSize = GetFileVersionInfoSize(moduleFileName, &dummyZero);
	void* pVersion = malloc(versionSize);
	if (pVersion != NULL)
	{
		GetFileVersionInfo(moduleFileName, NULL, versionSize, pVersion);

		UINT length;
		VS_FIXEDFILEINFO* pFixInfo;
		if (VerQueryValue(pVersion, _T("\\"), (LPVOID*)&pFixInfo, &length))
		{
			auto v1 = HIWORD(pFixInfo->dwFileVersionMS);
			auto v2 = LOWORD(pFixInfo->dwFileVersionMS);
			auto v3 = HIWORD(pFixInfo->dwFileVersionLS);
			auto v4 = LOWORD(pFixInfo->dwFileVersionLS);

			wstring moduleVersion = GetStringResource(_pluginInstance, IDS_ABOUTVERSION);
			wstring formattedVersion = format(moduleVersion, v1, v2, v3, v4);

			SetDlgItemText(hDlg, IDC_STATIC_VERSION, formattedVersion.c_str());
		}

		free(pVersion);
	}
}

void CAboutDialog::SetBuiltUsingString(HWND hDlg)
{
	wstring builtUsing = GetStringResource(_pluginInstance, IDS_BUILTUSING);
	wstring formattedBuiltUsing = format(builtUsing, _megaVersion);

	SetDlgItemText(hDlg, IDC_STATIC_BUILTUSING, formattedBuiltUsing.c_str());
}

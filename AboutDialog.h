#pragma once

class CAboutDialog final
{
public:
	CAboutDialog(HINSTANCE pluginInstance, std::wstring megaVersion);
	CAboutDialog(CAboutDialog&) = delete;
	CAboutDialog(CAboutDialog&&) = delete;
	CAboutDialog& operator=(const CAboutDialog&) = delete;
	CAboutDialog& operator=(const CAboutDialog&&) = delete;

	INT_PTR ShowAboutDialog(HWND mainWnd);

private:
	static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void InitDialog(HWND hDlg);
	void SetVersionString(HWND hDlg);
	void SetBuiltUsingString(HWND hDlg);
private:
	HINSTANCE _pluginInstance;
	std::wstring _megaVersion;
};
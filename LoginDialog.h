#pragma once

class CLoginDialog final
{
public:
	CLoginDialog(HINSTANCE pluginInstance, std::wstring iniPath, int pluginNr, tCryptProcW pCryptProc, int cryptoNr, int cryptoFlags);
	CLoginDialog(CLoginDialog&) = delete;
	CLoginDialog(CLoginDialog&&) = delete;
	CLoginDialog& operator=(const CLoginDialog&) = delete;
	CLoginDialog& operator=(const CLoginDialog&&) = delete;

	INT_PTR ShowLoginDialog(HWND mainWnd);
	void ValidateInput();

	std::wstring GetUsername();
	std::wstring GetPassword();
	std::wstring GetMultifactorKey();

private:
	static INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lparam);
	void InitDialog(HWND hDlg);
	void HandleOk(HWND hDlg);
	void ReadFromIni();
	void UpdateUsernameFromEdit();
	void UpdateUsernameFromCombo();
	void UpdatePasswordFromEdit();
	void UpdateMultifactorKeyFromEdit();
	void GetPasswordFromTc();
private:
	HINSTANCE _pluginInstance;
	HWND _cbUsername;
	HWND _edPassword;
	HWND _edMultifactorKey;
	HWND _chkRemember;
	HWND _btnOk;
	int _pluginNr;
	tCryptProcW _cryptProc;
	int _cryptoNr;
	int _cryptoFlags;
	std::wstring _iniPath;
	std::wstring _username;
	std::wstring _password;
	std::wstring _multifactorKey;
	std::vector<std::wstring> _savedUsers;
	bool _remember;
};
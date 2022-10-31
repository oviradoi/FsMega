#include "pch.h"
#include "utils.h"
#include "resource.h"
#include "LoginDialog.h"

using namespace std;

const TCHAR AppName[] = _T("FsMega");

CLoginDialog::CLoginDialog(HINSTANCE pluginInstance, std::wstring iniPath, int pluginNr, tCryptProcW pCryptProc, int cryptoNr, int cryptoFlags)
{
	_pluginInstance = pluginInstance;
	_iniPath = iniPath;
	_pluginNr = pluginNr;
	_cryptProc = pCryptProc;
	_cryptoNr = cryptoNr;
	_cryptoFlags = cryptoFlags;
}

INT_PTR CLoginDialog::ShowLoginDialog(HWND mainWnd)
{
	LPARAM pThis = (LPARAM)this;
	return DialogBoxParam(_pluginInstance, MAKEINTRESOURCE(IDD_LOGINDIALOG), mainWnd, LoginDlgProc, pThis);
}

INT_PTR CALLBACK CLoginDialog::LoginDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		CLoginDialog* lpThis = (CLoginDialog*)lParam;		
		lpThis->InitDialog(hDlg);
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
	{
		CLoginDialog* lpThis = (CLoginDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT_PASSWORD)
		{
			lpThis->UpdatePasswordFromEdit();
			lpThis->ValidateInput();
		}
		else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT_MULTIFACTORKEY)
		{
			lpThis->UpdateMultifactorKeyFromEdit();
		}
		else if (HIWORD(wParam) == CBN_EDITCHANGE && LOWORD(wParam) == IDC_COMBO_USERNAME)
		{
			lpThis->UpdateUsernameFromEdit();
			lpThis->ValidateInput();
		}
		else if (HIWORD(wParam) == CBN_SELENDOK && LOWORD(wParam) == IDC_COMBO_USERNAME)
		{
			lpThis->UpdateUsernameFromCombo();
			lpThis->GetPasswordFromTc();
			lpThis->ValidateInput();
		}
		else if (HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
		{
			if (LOWORD(wParam) == IDOK)
			{
				lpThis->HandleOk(hDlg);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}

void CLoginDialog::InitDialog(HWND hDlg)
{
	// Load icon
	HICON hIcon = (HICON)LoadImage(_pluginInstance, MAKEINTRESOURCE(IDI_MEGA), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED);
	SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	_cbUsername = GetDlgItem(hDlg, IDC_COMBO_USERNAME);
	_edPassword = GetDlgItem(hDlg, IDC_EDIT_PASSWORD);
	_edMultifactorKey = GetDlgItem(hDlg, IDC_EDIT_MULTIFACTORKEY);
	_chkRemember = GetDlgItem(hDlg, IDC_REMEMBER);
	_btnOk = GetDlgItem(hDlg, IDOK);

	ReadFromIni();
	GetPasswordFromTc();

	Button_SetCheck(_chkRemember, _remember);
	if (_remember)
	{
		ComboBox_SetText(_cbUsername, _username.c_str());
		Edit_SetText(_edPassword, _password.c_str());
	}

	ValidateInput();
}

void CLoginDialog::ValidateInput()
{
	Button_Enable(_btnOk, !_username.empty() && !_password.empty());
}

std::wstring CLoginDialog::GetUsername()
{
	return _username;
}

std::wstring CLoginDialog::GetPassword()
{
	return _password;
}

std::wstring CLoginDialog::GetMultifactorKey()
{
	return _multifactorKey;
}

void CLoginDialog::HandleOk(HWND hDlg)
{
	// Save username and password
	_remember = Button_GetCheck(_chkRemember);
	if (_remember)
	{
		WritePrivateProfileString(AppName, _T("Remember"), TEXT("1"), _iniPath.c_str());
		WritePrivateProfileString(AppName, _T("Username"), _username.c_str(), _iniPath.c_str());
		if (!_username.empty() && !_password.empty())
		{
			// Save password to TC
			_cryptProc(_pluginNr, _cryptoNr, FS_CRYPT_SAVE_PASSWORD, const_cast<WCHAR*>(_username.c_str()), const_cast<WCHAR*>(_password.c_str()), MAX_PATH);
		}

		if (std::find(_savedUsers.begin(), _savedUsers.end(), _username) == _savedUsers.end())
		{
			_savedUsers.emplace_back(_username);
		}

		WritePrivateProfileString(_T("Usernames"), nullptr, nullptr, _iniPath.c_str());
		for (size_t i = 0; i < _savedUsers.size(); i++)
		{
			WritePrivateProfileString(_T("Usernames"), const_cast<WCHAR*>(std::to_wstring(i).c_str()), _savedUsers[i].c_str(), _iniPath.c_str());
		}
	}
	else
	{
		WritePrivateProfileString(AppName, _T("Remember"), _T("0"), _iniPath.c_str());
		WritePrivateProfileString(AppName, _T("Username"), NULL, _iniPath.c_str());
		if (!_username.empty())
		{
			// Delete password from TC
			_cryptProc(_pluginNr, _cryptoNr, FS_CRYPT_DELETE_PASSWORD, const_cast<WCHAR*>(_username.c_str()), nullptr, 0);
		}
	}
}

void CLoginDialog::ReadFromIni()
{
	_remember = GetPrivateProfileInt(AppName, _T("Remember"), 0, _iniPath.c_str());
	_savedUsers.clear();
	ComboBox_ResetContent(_cbUsername);

	TCHAR buffer[MAX_PATH];
	for (int i = 0; i < 255; i++)
	{
		if (GetPrivateProfileString(_T("Usernames"), const_cast<WCHAR*>(std::to_wstring(i).c_str()), nullptr, buffer, MAX_PATH, _iniPath.c_str()) == 0)
		{
			break;
		}

		_savedUsers.emplace_back(buffer);
		ComboBox_AddString(_cbUsername, buffer);
	}

	GetPrivateProfileString(AppName, _T("Username"), _T(""), buffer, MAX_PATH, _iniPath.c_str());
	_username = buffer;
}

void CLoginDialog::GetPasswordFromTc()
{
	// Try to get password from TC
	if (!_username.empty())
	{
		TCHAR buffer[MAX_PATH];
		auto result = _cryptProc(_pluginNr, _cryptoNr, FS_CRYPT_LOAD_PASSWORD, const_cast<WCHAR*>(_username.c_str()), buffer, MAX_PATH);
		if (result == FS_FILE_OK)
		{
			_password = buffer;
		}
		else
		{
			_password = _T("");
		}
	}
	else
	{
		_password = _T("");
	}

	Edit_SetText(_edPassword, _password.c_str());
}

void CLoginDialog::UpdateUsernameFromEdit() 
{
	TCHAR buffer[MAX_STRING_RESOURCE];
	ComboBox_GetText(_cbUsername, buffer, MAX_STRING_RESOURCE);
	_username = buffer;
}

void CLoginDialog::UpdateUsernameFromCombo() 
{
	int crtSel = ComboBox_GetCurSel(_cbUsername);
	int textLen = ComboBox_GetLBTextLen(_cbUsername, crtSel);
	if (textLen != CB_ERR)
	{
		TCHAR* buffer = new TCHAR[textLen + 1];
		ComboBox_GetLBText(_cbUsername, crtSel, buffer);
		_username = buffer;
		delete[] buffer;
	}
}

void CLoginDialog::UpdatePasswordFromEdit() 
{
	TCHAR buffer[MAX_STRING_RESOURCE];
	Edit_GetText(_edPassword, buffer, MAX_STRING_RESOURCE);
	_password = buffer;
}

void CLoginDialog::UpdateMultifactorKeyFromEdit()
{
	TCHAR buffer[MAX_STRING_RESOURCE];
	Edit_GetText(_edMultifactorKey, buffer, MAX_STRING_RESOURCE);
	_multifactorKey = buffer;
}
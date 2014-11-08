/*
 *	Copyright (C) 2003, 2004 Amit Schreiber <gnobal@yahoo.com>
 *
 *	This file is part of Caller ID plugin for Miranda IM.
 *
 *	Caller ID plugin for Miranda IM is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Caller ID plugin for Miranda IM is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Caller ID plugin for Miranda IM ; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Options.h"
#include "random\utils\m_utils.h"
#include "../Relay/m_relay.h"
#include <string>
#include <fstream>

using std::string;

LRESULT g_lInitiallySelectedDevice = 0;

LPUNKNOWN GetComboItemData(HWND hCombo, LONG lIndex)
{
	LRESULT lRet = SendMessage(hCombo, CB_GETITEMDATA, lIndex, 0);
	if (lRet == CB_ERR)
		return NULL;

	return reinterpret_cast<LPUNKNOWN>(lRet);
}

LPUNKNOWN GetComboCurItemData(HWND hCombo)
{
	const LRESULT lCurSel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
	if (lCurSel == CB_ERR)
		return NULL;
	
	return GetComboItemData(hCombo, lCurSel);
}

int FindComboItemDataIndex(HWND hCombo, HANDLE hContactToFind)
{
	if (hContactToFind == NULL)
		return 0;

	const LRESULT lCount = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	if (lCount == CB_ERR)
		return 0;
	
	for (LONG lCurr = 0; lCurr < lCount; ++lCurr)
	{
		if (hContactToFind == GetComboItemData(hCombo, lCurr))
			return (int) lCurr;
	}

	return 0;
}

void ReadOptions(HWND hDlg)
{
	USES_CONVERSION;

	DBVARIANT dbvSelectedDevice = {0};
	const int nRetSelectedDevice = DBGetContactSetting(NULL, MODULE_NAME, OPT_SELECTED_DEVICE, &dbvSelectedDevice);
	
	string strSelectedDevice;
	if (nRetSelectedDevice == 0)
		strSelectedDevice = dbvSelectedDevice.pszVal;

	DBFreeVariant(&dbvSelectedDevice);

	HWND hDevicesCombo = GetDlgItem(hDlg, IDC_DEVICES);

	if (nRetSelectedDevice == 0)
	{
		const LRESULT lItemIndex = SendMessage(hDevicesCombo, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(strSelectedDevice.c_str()));
		if (lItemIndex == CB_ERR)
		{
			g_lInitiallySelectedDevice = CB_ERR;
		}
		else
		{
			const LRESULT lSelectRet = SendMessage(hDevicesCombo, CB_SETCURSEL, lItemIndex, 0);
			if (lSelectRet == CB_ERR)
			{
				g_lInitiallySelectedDevice = CB_ERR;
			}
			else
			{
				g_lInitiallySelectedDevice = lItemIndex;
			}
		}
	}
	else
	{
		g_lInitiallySelectedDevice = CB_ERR;
	}

	if (ServiceExists(MS_RELAY_RELAY_MESSAGE))
	{
		CheckDlgButton(hDlg, IDC_USE_RELAY, 
			(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_USE_RELAY, FALSE));

		ShowWindow(GetDlgItem(hDlg, IDC_GET_RELAY), SW_HIDE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg, IDC_USE_RELAY), FALSE);
	}

	DBVARIANT dbvLogFilePath = {0};
	const int nRetLogFilePath = DBGetContactSetting(NULL, MODULE_NAME, OPT_LOG_FILE_PATH, &dbvLogFilePath);
	
	string strLogFilePath;
	if (nRetLogFilePath == 0)
	{
		strLogFilePath = dbvLogFilePath.pszVal;
	}
	else
	{
		strLogFilePath = "";
	}

	DBFreeVariant(&dbvLogFilePath);

	SetWindowText(GetDlgItem(hDlg, IDC_LOG_FILE_PATH), A2CT(strLogFilePath.c_str()));

	const BOOL bLogCalls = (BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_LOG_CALLS, FALSE);
	CheckDlgButton(hDlg, IDC_LOG_CALLS, bLogCalls);
	EnableWindow(GetDlgItem(hDlg, IDC_LOG_FILE_PATH), bLogCalls);

	SetDlgItemInt(hDlg, IDC_NOTIFY_TIME, DBGetContactSettingDword(NULL, MODULE_NAME, OPT_NOTIFY_TIME, 10), FALSE);

	const BOOL bShowNotification = DBGetContactSettingByte(NULL, MODULE_NAME, OPT_SHOW_NOTIFICATION, FALSE);
	CheckDlgButton(hDlg, IDC_SHOW_NOTIFICATION, bShowNotification);
	EnableWindow(GetDlgItem(hDlg, IDC_NOTIFY_TIME), bShowNotification);
}

void LogFileError(HWND hDlg)
{
	USES_CONVERSION;

	const char* szFilePathErrorMsg = "Invalid path specified for Caller ID log file. A log file will not be created.";
	const char* szFilePathErrorTitle = "Caller ID Settings Error";

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_LOG_CALLS, FALSE);
	DBWriteContactSettingString(NULL, MODULE_NAME, OPT_LOG_FILE_PATH, "");
	
	CheckDlgButton(hDlg, IDC_LOG_CALLS, FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_LOG_FILE_PATH), FALSE);

	MessageBox(hDlg, A2CT(szFilePathErrorMsg), A2CT(szFilePathErrorTitle), MB_OK | MB_ICONERROR);
}

void WriteOptions(HWND hDlg)
{
	USES_CONVERSION;

	const LRESULT lSelectRet = SendMessage(GetDlgItem(hDlg, IDC_DEVICES), CB_GETCURSEL, 0, 0);
	if (lSelectRet == CB_ERR)
		return;

	if (lSelectRet != g_lInitiallySelectedDevice)
	{
		LPUNKNOWN pUnkSelectedAddress = GetComboCurItemData(GetDlgItem(hDlg, IDC_DEVICES));
		if (pUnkSelectedAddress == NULL)
		{
			ATLASSERT(FALSE);
			return;
		}

		CComQIPtr<ITAddress> pSelectedAddress = pUnkSelectedAddress;

		HRESULT hr = S_OK;
		if (g_lRegisterPhone != (long) -1) // Release previous registration if exists
		{
			hr = g_pTapi->UnregisterNotifications(g_lRegisterPhone);
			ATLASSERT(SUCCEEDED(hr));
			if (FAILED(hr))
				return;
		}
		
		g_lRegisterPhone = (long) -1;

		CComBSTR bstrAddressName;
		hr = pSelectedAddress->get_AddressName(&bstrAddressName);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return;

		hr = g_pTapi->RegisterCallNotifications(
			pSelectedAddress,
			VARIANT_TRUE,   // monitor privileges
			VARIANT_TRUE,   // owner privileges
			MEDIA_TYPE, 
			g_dwCookie,		// As returned by Advise
			&g_lRegisterPhone
		);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return;

		int nWriteRet = DBWriteContactSettingString(NULL, MODULE_NAME, OPT_SELECTED_DEVICE, OLE2A(bstrAddressName));
		ATLASSERT(nWriteRet == 0);
		if (nWriteRet != 0)
			return;
	}

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_USE_RELAY, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_USE_RELAY));

	g_fOutFile.close();
	g_fOutFile.clear();

	if (IsDlgButtonChecked(hDlg, IDC_LOG_CALLS))
	{
		TCHAR szLogFilePath[_MAX_PATH + 1] = {0};
		GetDlgItemText(hDlg, IDC_LOG_FILE_PATH, szLogFilePath, _MAX_PATH+1);

		TCHAR szLogFileAbsPath[_MAX_PATH + 1] = {0};
		if (_fullpath(szLogFileAbsPath, szLogFilePath, _MAX_PATH+1) == NULL)
		{
			LogFileError(hDlg);
		}
		else
		{
			g_fOutFile.open(szLogFileAbsPath, std::ios_base::out | std::ios_base::app);
			if (!g_fOutFile)
			{
				LogFileError(hDlg);
			}
			else
			{
				DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_LOG_CALLS, TRUE);
				DBWriteContactSettingString(NULL, MODULE_NAME, OPT_LOG_FILE_PATH, szLogFileAbsPath);
			}
		}
	}
	else
	{
		DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_LOG_CALLS, FALSE);
		DBWriteContactSettingString(NULL, MODULE_NAME, OPT_LOG_FILE_PATH, "");
	}

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_SHOW_NOTIFICATION, IsDlgButtonChecked(hDlg, IDC_SHOW_NOTIFICATION));
	DBWriteContactSettingDword(NULL, MODULE_NAME, OPT_NOTIFY_TIME, GetDlgItemInt(hDlg, IDC_NOTIFY_TIME, NULL, FALSE));
}	

BOOL CALLBACK CallerIDOptionsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hWnd);
			
			AddressArray arrValidAddresses;
			HRESULT hr = FindAddresses(g_pTapi, LINEADDRESSTYPE_PHONENUMBER, MEDIA_TYPE, arrValidAddresses);
			ATLASSERT(SUCCEEDED(hr));
			if (FAILED(hr))
				return FALSE;

			HWND hWndDevicesCombo = GetDlgItem(hWnd, IDC_DEVICES);
			for (AddressArray::iterator it = arrValidAddresses.begin(); it != arrValidAddresses.end(); ++it)
			{
				USES_CONVERSION;

				CComBSTR bstrAddressName;
				HRESULT hr = (*it)->get_AddressName(&bstrAddressName);
				if (FAILED(hr))
				{
					ATLASSERT(FALSE);
					continue;
				}
				
				const TCHAR* szAddressName = OLE2CT(bstrAddressName.m_str);
				const LRESULT lItemPos = SendMessage(hWndDevicesCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szAddressName));
				if (lItemPos == CB_ERR)
					continue;

				LPUNKNOWN pUnkAddress = NULL;
				(*it)->QueryInterface(IID_IUnknown, (LPVOID*) &pUnkAddress);

				const LRESULT lRet = SendMessage(hWndDevicesCombo, CB_SETITEMDATA, lItemPos, reinterpret_cast<LPARAM>(pUnkAddress));
				if (lRet == CB_ERR)
					continue;

			}

			const LRESULT lCount = SendMessage(hWndDevicesCombo, CB_GETCOUNT, 0, 0);
			if (lCount == CB_ERR)
				return 0;

			SendMessage(GetDlgItem(hWnd, IDC_LOG_FILE_PATH), EM_SETLIMITTEXT, _MAX_PATH, 0);
			
			// EnableDialogItems(hWnd, (BOOL) lCount > 0);
			if (lCount != 0)
			{
				ReadOptions(hWnd);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_DEVICES), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_USE_RELAY), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_GET_RELAY), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_LOG_CALLS), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_LOG_FILE_PATH), FALSE);
				ShowWindow(GetDlgItem(hWnd, IDC_STATIC_NO_DEVICES), SW_SHOW);
			}
		}
		break;

	case WM_COMMAND:
		{
			const int nID = LOWORD(wParam);
			switch (nID)
			{
			case IDC_GET_RELAY:
				CallService(MS_UTILS_OPENURL, TRUE, reinterpret_cast<LPARAM>("http://www.miranda-im.org/download/details.php?action=viewfile&id=1255"));
				return FALSE; // No need to notify change

			case IDC_DEVICES:
				switch (HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					break;

				default:
					return FALSE;
				}
				break;

			case IDC_LOG_CALLS:
				{
					const BOOL bUseLogFile = IsDlgButtonChecked(hWnd, IDC_LOG_CALLS);
					EnableWindow(GetDlgItem(hWnd, IDC_LOG_FILE_PATH), bUseLogFile);
				}
				break;

			case IDC_LOG_FILE_PATH:
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					if (reinterpret_cast<HWND>(lParam) == GetFocus())
					{
						break;
					}
					else
					{
						return FALSE;
					}

				default:
					return FALSE;
				}
				break;

			case IDC_NOTIFY_TIME:
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					if (reinterpret_cast<HWND>(lParam) == GetFocus())
					{
						break;
					}
					else
					{
						return FALSE;
					}

				default:
					return FALSE;
				}
				break;

			case IDC_SHOW_NOTIFICATION:
				{
					EnableWindow(GetDlgItem(hWnd, IDC_NOTIFY_TIME), IsDlgButtonChecked(hWnd, IDC_SHOW_NOTIFICATION));
				}
				break;

			default:
				break;
			}

			SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
			case PSN_APPLY:
				WriteOptions(hWnd);
				return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			HWND hDevicesCombo = GetDlgItem(hWnd, IDC_DEVICES);
			const LRESULT lCount = SendMessage(hDevicesCombo, CB_GETCOUNT, 0, 0);
			if (lCount == CB_ERR)
				return 0;
			
			for (LONG lCurr = 0; lCurr < lCount; ++lCurr)
			{
				LPUNKNOWN pUnkAddress = GetComboItemData(hDevicesCombo, lCurr);
				if (pUnkAddress == NULL)
				{
					ATLASSERT(FALSE);
					continue;
				}

				pUnkAddress->Release();
			}
		}
		break;

	default:
		break;
	}

	return FALSE;
}


int EventOptionsInitalize(WPARAM addInfo, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp = {0};
    
	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_CALLER_ID_OPTIONS);
	odp.pszTitle = Translate("Caller ID");
	odp.pszGroup = Translate("Plugins");
	odp.pfnDlgProc = CallerIDOptionsWndProc;
	
	CallService(MS_OPT_ADDPAGE, addInfo, (LPARAM)&odp);

    return 0;
}

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

#include "Common.h"
#include "CallerIDEventHandler.h"
#include "TapiEventProcessor.h"
#include "m_callerid.h"
#include "Options.h"

#include "ui/contactlist/m_clist.h"

#include "../Relay/m_relay.h"

#include <fstream>
#include <ctime>
#include <string>

using std::string;

HINSTANCE hInst;
PLUGINLINK *pluginLink;
std::ofstream g_fOutFile;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"Caller ID",
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"Caller ID Plugin (beta)",
	"Amit Schreiber",
	"gnobal@sourceforge.net",
	"© 2004 Amit Schreiber",
	"http://miranda-icq.sourceforge.net/",
	0,		//not transient
	0		//doesn't replace anything built-in
};


CComModule _Module;
CComPtr<ITTAPI> g_pTapi;
CComPtr<IConnectionPoint> g_pCP;
DWORD g_dwCookie = 0;
long g_lRegisterPhone = (long) -1;

HANDLE g_hMessageThread = NULL;
DWORD g_dwMessageThreadId = 0;

HANDLE g_hEventIncomingCall = NULL;
HANDLE g_hIncomingCallHook = NULL;

HANDLE g_hEventOptions = NULL;


BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

int IncomingCallHandler(WPARAM wParam,LPARAM lParam)
{
	CALLERIDINCOMINGCALLINFO* pici = reinterpret_cast<CALLERIDINCOMINGCALLINFO*>(lParam);
	BOOL bHasNumber = (BOOL) strcmp(pici->szCallerIdNumber, "");
	const char* szUnknown = "Unknown/Private Caller";

	// Notify to relay
	if (DBGetContactSettingByte(NULL, MODULE_NAME, OPT_USE_RELAY, FALSE))
	{
		string strMsg = "Incoming call: ";
		strMsg += bHasNumber ? pici->szCallerIdNumber : szUnknown;

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage = strMsg.c_str();
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}

	// Notify with baloon
	if (DBGetContactSettingByte(NULL, MODULE_NAME, OPT_SHOW_NOTIFICATION, FALSE))
	{
		MIRANDASYSTRAYNOTIFY msn = {0};
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = NULL;
		msn.szInfoTitle = "Incoming Call";
		if (!bHasNumber)
		{
			msn.szInfo = const_cast<char*>(szUnknown);
		}
		else
		{
			msn.szInfo = const_cast<char*>(pici->szCallerIdNumber);
		}
		msn.uTimeout = 1000*DBGetContactSettingDword(NULL, MODULE_NAME, OPT_NOTIFY_TIME, 10);
		msn.dwInfoFlags = NIIF_INFO;

		CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, reinterpret_cast<LPARAM>(&msn));
	}

	// Write to log
	if (DBGetContactSettingByte(NULL, MODULE_NAME, OPT_LOG_CALLS, FALSE))
	{
		char szTimeString[200];
		const size_t nStringSize = sizeof(szTimeString) / sizeof(szTimeString[0]);
		time_t tTime;
		time(&tTime);
		struct tm* now = localtime(&tTime);

		strftime(szTimeString, nStringSize,"%Y-%m-%d\t%H:%M:%S\t", now);
		g_fOutFile << szTimeString;
		if (bHasNumber)
		{
			g_fOutFile << pici->szCallerIdNumber;
		}
		else
		{
			g_fOutFile << "0";
		}
		g_fOutFile << std::endl;

		g_fOutFile.flush();
	}
		
	return 0;
}


extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	USES_CONVERSION;

	pluginLink = link;

	g_hEventOptions = HookEvent(ME_OPT_INITIALISE, EventOptionsInitalize);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return 1;
	
	hr = _Module.Init(NULL, hInst);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return 1;


	hr = InitializeTAPI(g_pTapi, TE_CALLNOTIFICATION | TE_CALLINFOCHANGE | TE_ADDRESS | TE_CALLSTATE);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return 1;
	
	CComPtr<ITTAPIEventNotification> pEventHandler = new CComObject<CCallerIDEventHandler>;
	hr = AdviseTAPI(g_pTapi, pEventHandler, g_pCP, g_dwCookie);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return 1;
	
	// Actual code starts here
	
	AddressArray arrValidAddresses;
	hr = FindAddresses(g_pTapi, LINEADDRESSTYPE_PHONENUMBER, MEDIA_TYPE, arrValidAddresses);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return 1;

	DBVARIANT dbvSelectedDevice = {0};
	const int nRetSelectedDevice = DBGetContactSetting(NULL, MODULE_NAME, OPT_SELECTED_DEVICE, &dbvSelectedDevice);
	
	if (nRetSelectedDevice == 0) // Option exists in DB
	{
		const string strSelectedDevice = dbvSelectedDevice.pszVal;

		AddressArray::iterator it;
		for (it = arrValidAddresses.begin(); it != arrValidAddresses.end(); ++it)
		{
			CComBSTR bstrAddressName;
			hr = (*it)->get_AddressName(&bstrAddressName);
			if (FAILED(hr))
				continue;

			if (strSelectedDevice == string(OLE2A(bstrAddressName.m_str)))
				break;
		}

		if (it != arrValidAddresses.end())
		{
			hr = g_pTapi->RegisterCallNotifications(
				*it,
				// arrValidAddresses[0],
				VARIANT_TRUE,   // monitor privileges
				VARIANT_TRUE,   // owner privileges
				MEDIA_TYPE, 
				g_dwCookie,		// As returned by Advise
				&g_lRegisterPhone
			);
			ATLASSERT(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				DBFreeVariant(&dbvSelectedDevice);
				return 1;
			}
		}
	}
	DBFreeVariant(&dbvSelectedDevice);

	// <NewCode>

	if (DBGetContactSettingByte(NULL, MODULE_NAME, OPT_LOG_CALLS, FALSE))
	{
		DBVARIANT dbvLogFilePath = {0};
		const int nRetLogFilePath = DBGetContactSetting(NULL, MODULE_NAME, OPT_LOG_FILE_PATH, &dbvLogFilePath);
		if (nRetLogFilePath == 0)
		{
			g_fOutFile.open(dbvLogFilePath.pszVal, std::ios_base::out | std::ios_base::app);
		}
		DBFreeVariant(&dbvLogFilePath);
	}


	g_hEventIncomingCall = CreateHookableEvent(ME_CALLERID_INCOMING_CALL);

	g_hIncomingCallHook = HookEvent(ME_CALLERID_INCOMING_CALL, IncomingCallHandler);

	g_hMessageThread = CreateThread(NULL, 0, EventProcessingThreadProc, NULL, 0, &g_dwMessageThreadId);
	if (g_hMessageThread == NULL)
	{
		ATLASSERT(FALSE);
		return 1;
	}


	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	if (g_hEventOptions != NULL)
	{
		UnhookEvent(g_hEventOptions);
		g_hEventOptions = NULL;
	}

	if (g_dwMessageThreadId != 0)
	{
		PostThreadMessage(g_dwMessageThreadId, WM_QUIT, NULL, NULL);
		DWORD dwWaitRet = WaitForSingleObject(g_hMessageThread, INFINITE);
		ATLASSERT(dwWaitRet == WAIT_OBJECT_0);
		CloseHandle(g_hMessageThread);
		
		g_hMessageThread = NULL;
		g_dwMessageThreadId = 0;
	}

	if (g_lRegisterPhone != (long) -1)
	{
		g_pTapi->UnregisterNotifications(g_lRegisterPhone);
		g_lRegisterPhone = (long) -1;
	}

	if (g_dwCookie != 0)
	{
		HRESULT hr = UnadviseTAPI(g_pCP, g_dwCookie);
		ATLASSERT(SUCCEEDED(hr));

		g_dwCookie = 0;
	}

	if (g_pTapi != NULL)
	{
		ShutdownTAPI(g_pTapi);
	}

	_Module.Term();
    CoUninitialize();


	if (g_hIncomingCallHook != NULL)
	{
		UnhookEvent(g_hIncomingCallHook);
		g_hIncomingCallHook = NULL;
	}

	if (g_hEventIncomingCall != NULL)
	{
		DestroyHookableEvent(g_hEventIncomingCall);
		g_hEventIncomingCall = NULL;
	}

	if (g_fOutFile)
	{
		g_fOutFile.close();
		g_fOutFile.clear();
	}

	return 0;
}


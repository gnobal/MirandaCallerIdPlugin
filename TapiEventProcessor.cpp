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

#include "TapiEventProcessor.h"
#include "random/plugins/newpluginapi.h"
#include "m_callerid.h"
#include <string>
#include <memory>

using std::auto_ptr;
using std::string;

CComAutoCriticalSection csGlobals;
CComPtr<ITCallInfo> g_pCurrCallInfo;
int g_nNumRings = 0;
BOOL g_bCallWasNotified = FALSE;

HRESULT HandleEvent(TAPI_EVENT TapiEvent, IDispatch* pEvent)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;

	CComQIPtr<ITCallInfo> pCallInfo;
	if (TapiEvent == TE_CALLNOTIFICATION)
	{
		CComQIPtr<ITCallNotificationEvent> pNotification = pEvent;
		ATLASSERT(pNotification != NULL);
		if (pNotification == NULL)
			return E_POINTER;

		hr = pNotification->get_Call(&pCallInfo);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		csGlobals.Lock();
		// ATLASSERT(g_pCurrCallInfo == NULL);
		g_pCurrCallInfo = pCallInfo;
		g_bCallWasNotified = FALSE;
		g_nNumRings = 0;
		csGlobals.Unlock();

		// Return quickly here... usually no caller ID information will be available now
		// so we just save the CallInfo object for later in hope that it'll be quick enough 
		// to catch the TE_CALLINFOCHANGE event
		// If it's not quick enough, we'll get the information in the next ring
		// This is a known annoyance in the TAPI 3 events handling, which forces to save
		// the latest call in a global variable
		return S_OK;
	}
	else if (TapiEvent == TE_CALLINFOCHANGE)
	{
		CComQIPtr<ITCallInfoChangeEvent> pNotification = pEvent;
		ATLASSERT(pNotification != NULL);
		if (pNotification == NULL)
			return E_POINTER;

		CALLINFOCHANGE_CAUSE eCause = CIC_OTHER;
		hr = pNotification->get_Cause(&eCause);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if (eCause != CIC_CALLERID)
			return S_OK;

		hr = pNotification->get_Call(&pCallInfo);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
	} 
	else if (TapiEvent == TE_ADDRESS)
	{
		CComQIPtr<ITAddressEvent> pNotification = pEvent;
		ATLASSERT(pNotification != NULL);
		if (pNotification == NULL)
			return E_POINTER;
		
		ADDRESS_EVENT eEvent = AE_STATE;
		hr = pNotification->get_Event(&eEvent);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
		
		if (eEvent != AE_RINGING)
			return S_OK;
	
		csGlobals.Lock();
		pCallInfo = g_pCurrCallInfo;
		g_nNumRings++;
		csGlobals.Unlock();

	}
	else if (TapiEvent == TE_CALLSTATE)
	{
		CComQIPtr<ITCallStateEvent> pNotification = pEvent;
		ATLASSERT(pNotification != NULL);
		if (pNotification == NULL)
			return E_POINTER;

		hr = pNotification->get_Call(&pCallInfo);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		CALL_STATE eState;
		hr = pCallInfo->get_CallState(&eState);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if (eState != CS_DISCONNECTED)
			return S_OK;

		csGlobals.Lock();
		g_pCurrCallInfo = (ITCallInfo*) NULL;
		g_nNumRings = 0;
		csGlobals.Unlock();

		return S_OK;

	}
	
	// if (pCallInfo == NULL) // Happens after we've already notified the event
	//	return S_OK;

	CComBSTR bstrString;
	hr = pCallInfo->get_CallInfoString(CIS_CALLERIDNUMBER, &bstrString);
	
	csGlobals.Lock();
	// Do not notify the call:
	// 1. Again
	// 2. If there's no call info available and the phone rang less than twice (give
	//    caller ID info time to become available)
	if ((bstrString.Length() == 0 && g_nNumRings < 2) || g_bCallWasNotified == TRUE)
	{
		csGlobals.Unlock();
		return S_OK;
	}
	csGlobals.Unlock();

	CALLERIDINCOMINGCALLINFO ici = {0};
	ici.cbSize = sizeof(CALLERIDINCOMINGCALLINFO);
	if (bstrString.m_str != NULL)
	{
		ici.szCallerIdNumber = OLE2CA(bstrString.m_str);
	}
	else
	{
		ici.szCallerIdNumber = "";
	}
	NotifyEventHooks(g_hEventIncomingCall, 0, reinterpret_cast<LPARAM>(&ici));

	bstrString.Empty();

	csGlobals.Lock();
	// g_pCurrCallInfo = NULL;
	g_nNumRings = 0;
	g_bCallWasNotified = TRUE;
	csGlobals.Unlock();

	return S_OK;
}

DWORD WINAPI EventProcessingThreadProc(LPVOID lpParameter)
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_USERTAPIEVENT)
		{
            TAPI_EVENT TapiEvent = (TAPI_EVENT)msg.wParam;
            IDispatch *pEvent = (IDispatch*)msg.lParam;


            HandleEvent(TapiEvent, pEvent);

			pEvent->Release();
            pEvent = NULL;
		}
	}

	g_pCurrCallInfo = NULL; // Release (in case we're in the middle of handling an incoming call)

	return 0;
}

/*
	hr = pCallInfo->get_CallInfoString(CIS_CALLEDIDNAME, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_CALLEDIDNUMBER, &bstrString);
	bstrString.Empty();
	
	hr = pCallInfo->get_CallInfoString(CIS_CONNECTEDIDNAME, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_CONNECTEDIDNUMBER, &bstrString);
	bstrString.Empty();
	
	hr = pCallInfo->get_CallInfoString(CIS_REDIRECTIONIDNAME, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_REDIRECTIONIDNUMBER, &bstrString);
	bstrString.Empty();
	
	hr = pCallInfo->get_CallInfoString(CIS_REDIRECTINGIDNAME, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_REDIRECTINGIDNUMBER, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_CALLEDPARTYFRIENDLYNAME, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_COMMENT, &bstrString);
	bstrString.Empty();
	
	hr = pCallInfo->get_CallInfoString(CIS_DISPLAYABLEADDRESS, &bstrString);
	bstrString.Empty();

	hr = pCallInfo->get_CallInfoString(CIS_CALLINGPARTYID, &bstrString);
	bstrString.Empty();

 */
/* // Get the call info using the VB interface (TE_ADDRESS)
		CComPtr<ITAddress> pAddress;
		hr = pNotification->get_Address(&pAddress);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		CComVariant varCalls;
		hr = pAddress->get_Calls(&varCalls);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		CComQIPtr<ITCollection> pEnumCalls = varCalls.pdispVal;
		ATLASSERT(pEnumCalls != NULL);
		if (pEnumCalls == NULL)
			return E_POINTER;

		long lCallCount = 0;
		hr = pEnumCalls->get_Count(&lCallCount);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if (lCallCount == 0)
			return S_OK;

		CComVariant varCall;
		hr = pEnumCalls->get_Item(1, &varCall);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		pCallInfo = varCall.pdispVal;
		ATLASSERT(pCallInfo != NULL);
		if (pCallInfo == NULL)
			return E_POINTER;
*/
/* // Get the call info using the C++ interface TE_ADDRESS)
		CComPtr<ITAddress> pAddress;
		hr = pNotification->get_Address(&pAddress);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		CComPtr<IEnumCall> pEnumCalls;
		hr = pAddress->EnumerateCalls(&pEnumCalls);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		ULONG ulFetched = 0;
		hr = pEnumCalls->Next(1, &pCallInfo, &ulFetched);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if (hr == S_FALSE)
			return S_OK;
*/

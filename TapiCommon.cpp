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

#include "TapiCommon.h"

HRESULT InitializeTAPI(CComPtr<ITTAPI>& pTapi, long lEventFilterMask)
{
    
    HRESULT hr = E_FAIL;

    hr = pTapi.CoCreateInstance(
                          CLSID_TAPI,
                          NULL,
                          CLSCTX_INPROC_SERVER
                         );
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pTapi->Initialize();
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        pTapi = NULL;
        
        return hr;
    }

    hr = pTapi->put_EventFilter(lEventFilterMask);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        pTapi->Shutdown();
        pTapi = NULL;
        return hr;
    }

    return S_OK;
}


void ShutdownTAPI(CComPtr<ITTAPI>& pTapi)
{
    if (pTapi != NULL)
    {
        pTapi->Shutdown();
        pTapi = NULL;
    }
}


HRESULT FindAddresses(CComPtr<ITTAPI>& pTapi, long lAddressType, long lMediaType, /*CComPtr<ITAddress>& pValidAddress*/ AddressArray& arrValidAddresses)
{

    HRESULT hr = E_FAIL;

    CComPtr<IEnumAddress> pEnumAddress;
    hr = pTapi->EnumerateAddresses(&pEnumAddress);
	ATLASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        return hr;
    }

    while (TRUE)
    {
        CComPtr<ITAddress> pAddress;
        hr = pEnumAddress->Next( 1, &pAddress, NULL );
        if (S_OK != hr)
        {
            break;
        }
        
        CComQIPtr<ITAddressCapabilities> pAddressCaps = pAddress;
        if (pAddressCaps == NULL)
        {
            pAddress = NULL;
            continue;
        }

        long nType = 0;
        hr = pAddressCaps->get_AddressCapability(AC_ADDRESSTYPES, &nType);
        pAddressCaps = (ITAddressCapabilities*) NULL;
        if (FAILED(hr))
        {
            pAddress = NULL;
            continue;
        }

        if (nType & lAddressType)
        {
            
            CComQIPtr<ITMediaSupport> pMediaSupport = pAddress;
            if (pMediaSupport == NULL)
            {
                pAddress = NULL;
                continue;
            }

            
            CComVariant bvarMediaTypeSupported = VARIANT_FALSE;
            hr = pMediaSupport->QueryMediaType(lMediaType, &bvarMediaTypeSupported.boolVal);

            pMediaSupport = (ITMediaSupport*) NULL;
            
            if (SUCCEEDED(hr) && (VARIANT_TRUE == bvarMediaTypeSupported.boolVal))
            {
                CComBSTR bstrAddressName;
                
                hr = pAddress->get_AddressName(&bstrAddressName);

                if (FAILED(hr))
                {
					ATLTRACE("FindAddress: failed to get address name\n");
                }
                else
                {
                    ATLTRACE("%S\n", bstrAddressName);
                }

                // pValidAddress = pAddress;
				
				arrValidAddresses.push_back(pAddress);
                
				// break;
            }
        }

        pAddress = NULL;
    }

    pEnumAddress = NULL;

/*    if (pValidAddress == NULL)
    {
        return E_FAIL;
    }
*/
	if (arrValidAddresses.size() == 0)
		return E_FAIL;

    return S_OK;
}

HRESULT AdviseTAPI(
				   CComPtr<ITTAPI>& pTapi,
				   CComPtr<ITTAPIEventNotification> pEventHandler,
				   CComPtr<IConnectionPoint>& pCP,
				   DWORD& dwCookie
				   )
{
	dwCookie = 0;
	
	CComQIPtr<IConnectionPointContainer> pCPC = pTapi;
	HRESULT hr = pCPC->FindConnectionPoint(IID_ITTAPIEventNotification, &pCP);
	ATLASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;

	pCPC = (IConnectionPointContainer*) NULL;

	CComPtr<IUnknown> pUnkEventHandler = pEventHandler;
	ATLASSERT(pUnkEventHandler != NULL);
	if (pUnkEventHandler == NULL)
		return E_POINTER;

	hr = pCP->Advise(pUnkEventHandler, &dwCookie);
	ATLASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;

	pUnkEventHandler = NULL;

	return S_OK;
}

HRESULT UnadviseTAPI(
					 CComPtr<IConnectionPoint>& pCP,
					 const DWORD dwCookie
					 )
{
	HRESULT hr = pCP->Unadvise(dwCookie);
	pCP = NULL;

	ATLASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;

	return S_OK;
}

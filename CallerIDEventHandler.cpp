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
#include "CallerIDEventHandler.h"
#include "TapiEventProcessor.h"
#include "random/plugins/newpluginapi.h"
#include "..\Relay\m_relay.h"

HRESULT STDMETHODCALLTYPE CCallerIDEventHandler::Event(
													   TAPI_EVENT TapiEvent,
													   IDispatch* pEvent
													   )
{
	pEvent->AddRef();
	PostThreadMessage(g_dwMessageThreadId, WM_USERTAPIEVENT, TapiEvent, (LPARAM) pEvent);
	return S_OK;
}


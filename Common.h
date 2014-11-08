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

#ifndef __CALLER_ID_COMMON_H__
#define __CALLER_ID_COMMON_H__

#include <windows.h>
#include "resource.h"

#include "TapiCommon.h"

#include "random/plugins/newpluginapi.h"
#include "database/m_database.h"
#include "ui/options/m_options.h"
#include "random/langpack/m_langpack.h"

#include <iosfwd>

#define MEDIA_TYPE (TAPIMEDIATYPE_DATAMODEM) // Because it's actually a modem :-S

extern HINSTANCE hInst;

extern CComPtr<ITTAPI> g_pTapi;
extern DWORD g_dwCookie;
extern long g_lRegisterPhone;
extern std::ofstream g_fOutFile;

#endif

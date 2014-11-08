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

#ifndef __CALLER_ID_OPTIONS_H__
#define __CALLER_ID_OPTIONS_H__

#include "Common.h"

#define MODULE_NAME "CallerID"
/*
#define OPT_MIMIC "Mimic"
#define OPT_ALERT_ONLINE "AlertOnline"
#define OPT_ALERT_OFFLINE "AlertOffline"
#define OPT_ALLOWED_COMMANDS "AllowedCommands"
#define OPT_DISALLOWED_COMMANDS "DisallowedCommands"
*/

#define OPT_SELECTED_DEVICE "SelectedDevice"
#define OPT_USE_RELAY "UseRelay"
#define OPT_LOG_CALLS "LogCalls"
#define OPT_LOG_FILE_PATH "LogFilePath"
#define OPT_SHOW_NOTIFICATION "ShowNotification"
#define OPT_NOTIFY_TIME "NotifyTime"

int EventOptionsInitalize(WPARAM addInfo, LPARAM lParam);

#endif

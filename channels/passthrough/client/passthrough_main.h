/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Passthrough virtual channel
 *
 * Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FREERDP_CHANNEL_CLIPRDR_CLIENT_MAIN_H
#define FREERDP_CHANNEL_CLIPRDR_CLIENT_MAIN_H

#include <winpr/stream.h>

#include <freerdp/svc.h>
#include <freerdp/addin.h>
#include <freerdp/channels/log.h>

#define TAG CHANNELS_TAG("plex.client")

struct passthrough_plugin
{
	CHANNEL_DEF channelDef;
	CHANNEL_ENTRY_POINTS_FREERDP_EX channelEntryPoints;
	char* real_channel_name;
	
	PassthroughClientContext* context;

	wLog* log;
	HANDLE thread;
	void* InitHandle;
	DWORD OpenHandle;
};
typedef struct passthrough_plugin passthroughPlugin;

PassthroughClientContext* cliprdr_get_client_interface(passthroughPlugin* passthrough);

#ifdef WITH_DEBUG_CLIPRDR
#define DEBUG_CLIPRDR(...) WLog_DBG(TAG, __VA_ARGS__)
#else
#define DEBUG_CLIPRDR(...) do { } while (0)
#endif

#endif /* FREERDP_CHANNEL_CLIPRDR_CLIENT_MAIN_H */

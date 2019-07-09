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

#ifndef FREERDP_CHANNEL_PASSTHROUGH_SERVER_MAIN_H
#define FREERDP_CHANNEL_PASSTHROUGH_SERVER_MAIN_H

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/stream.h>
#include <winpr/thread.h>

#include <freerdp/channels/log.h>

#define TAG CHANNELS_TAG("plex.server")

typedef struct _passthrough_server_private PassthroughServerPrivate;

struct _passthrough_server_private
{
	HANDLE vcm;
	HANDLE Thread;
	HANDLE StopEvent;
	void* ChannelHandle;
	HANDLE ChannelEvent;

	wStream* s;
};

#endif /* FREERDP_CHANNEL_PASSTHROUGH_SERVER_MAIN_H */

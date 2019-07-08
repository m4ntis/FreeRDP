/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Display Update Virtual Channel Extension
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2015 Thincast Technologies GmbH
 * Copyright 2015 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
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

#ifndef FREERDP_CHANNEL_PASSTHROUGH_SERVER_PASSTHROUGH_H
#define FREERDP_CHANNEL_PASSTHROUGH_SERVER_PASSTHROUGH_H

#include <freerdp/channels/wtsvc.h>
#include <freerdp/freerdp.h>
#include <winpr/wtypes.h>

typedef struct _passthrough_server_context PassthroughServerContext;

typedef UINT (*cbPassthroughDataReceived)(PassthroughServerContext* context, const BYTE* data, UINT32 len);
typedef UINT (*cbPassthroughSendData)(PassthroughServerContext* context, BYTE* data, UINT32 len);
typedef UINT (*cbPassthroughOpen)(PassthroughServerContext* context);
typedef UINT (*cbPassthroughClose)(PassthroughServerContext* context);
typedef UINT (*cbPassthroughStart)(PassthroughServerContext* context);
typedef UINT (*cbPassthroughStop)(PassthroughServerContext* context);

struct _passthrough_server_context
{
	cbPassthroughOpen Open;
	cbPassthroughClose Close;
	cbPassthroughStart Start;
	cbPassthroughStop Stop;

	cbPassthroughDataReceived DataReceived;
	cbPassthroughSendData SendData;
	
	rdpContext* rdpcontext;

	void* handle;
	void* custom;
};


#ifdef __cplusplus
extern "C" {
#endif

FREERDP_API PassthroughServerContext* passthrough_server_context_new(HANDLE vcm);
FREERDP_API void passthrough_server_context_free(PassthroughServerContext* context);

#ifdef __cplusplus
}
#endif

#endif /* FREERDP_CHANNEL_PASSTHROUGH_SERVER_PASSTHROUGH_H */

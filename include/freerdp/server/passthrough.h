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

#include <winpr/wtypes.h>

typedef UINT (*cbDataReceived)(PassthroughServerContext* context, const BYTE* data);

typedef struct _passthrough_server_context PassthroughServerContext;

struct _passthrough_server_context
{
	cbDataReceived DataReceived;
	
	void* handle;
	void* custom;
};

#endif /* FREERDP_CHANNEL_PASSTHROUGH_SERVER_PASSTHROUGH_H */

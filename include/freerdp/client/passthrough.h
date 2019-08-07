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

#ifndef FREERDP_CHANNEL_PASSTHROUGH_CLIENT_PASSTHROUGH_H
#define FREERDP_CHANNEL_PASSTHROUGH_CLIENT_PASSTHROUGH_H

#include <winpr/wtypes.h>
#include <freerdp/freerdp.h>

typedef struct _passthrough_client_context PassthroughClientContext;

typedef UINT (*cbDataReceived)(PassthroughClientContext* context, const BYTE* data, UINT32 len);
typedef UINT (*cbSendData)(PassthroughClientContext* context, const BYTE* data, UINT32 len);

struct _passthrough_client_context
{
    cbDataReceived DataReceived;
    cbSendData SendData;

	rdpContext* rdpcontext;

	void* handle;
	void* custom;
	HANDLE write_complete;
};

#endif /* FREERDP_CHANNEL_PASSTHROUGH_CLIENT_PASSTHROUGH_H */

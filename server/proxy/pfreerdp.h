/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 *
 * Copyright 2019 Mati Shabtay <matishabtay@gmail.com>
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

#ifndef FREERDP_SERVER_PROXY_PFREERDP_H
#define FREERDP_SERVER_PROXY_PFREERDP_H

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>
#include <freerdp/channels/wtsvc.h>
#include <freerdp/server/audin.h>
#include <freerdp/server/rdpsnd.h>
#include <freerdp/server/encomsp.h>

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/wlog.h>

// Define log tags
#define PROXY_TAG(tag) "proxy." tag

struct proxy_context
{
	rdpContext _p;
	rdpContext* clientContext;

	RFX_CONTEXT* rfx_context;
	NSC_CONTEXT* nsc_context;
	wStream* s;
	BYTE* icon_data;
	BYTE* bg_data;
	int icon_width;
	int icon_height;
	int icon_x;
	int icon_y;
	BOOL activated;
	HANDLE event;
	HANDLE stopEvent;
	HANDLE vcm;
	void* debug_channel;
	HANDLE debug_channel_thread;
	audin_server_context* audin;
	BOOL audin_open;
	UINT32 frame_id;
	RdpsndServerContext* rdpsnd;
	EncomspServerContext* encomsp;
};
typedef struct proxy_context proxyContext;

#endif /* FREERDP_SERVER_PROXY_PFREERDP_H */
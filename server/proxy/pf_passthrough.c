/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 *
 * Copyright 2019 Mati Shabtay <matishabtay@gmail.com>
 * Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
 * Copyright 2019 Idan Freiberg <speidy@gmail.com>
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

#include <freerdp/client/passthrough.h>
#include <freerdp/server/passthrough.h>

#include <winpr/synch.h>

#include "pf_passthrough.h"
#include "pf_context.h"
#include "pf_log.h"

#define TAG PROXY_TAG("passthrough")

BOOL pf_server_passthrough_init(pServerContext* ps)
{
	PassthroughServerContext* pass;
	pass = ps->pass = passthrough_server_context_new(ps->vcm);

	if (!pass)
	{
		return FALSE;
	}

	pass->rdpcontext = (rdpContext*)ps;
	return TRUE;
}

static UINT pf_passthrough_data_received_from_client(PassthroughServerContext* context, const BYTE* data, UINT32 len)
{
	proxyData* pdata = (proxyData*) context->custom;
	PassthroughClientContext* client = (PassthroughClientContext*) pdata->pc->pass;
	client->SendData(client, data, len);
	return 0;
}

static UINT pf_passthrough_data_received_from_server(PassthroughClientContext* context, const BYTE* data, UINT32 len)
{
	proxyData* pdata = (proxyData*) context->custom;
	PassthroughServerContext* server = (PassthroughServerContext*) pdata->ps->pass;
	server->SendData(server, data, len);
	return 0;
}

void pf_passthrough_pipeline_init(PassthroughClientContext* client, PassthroughServerContext* server,
                             proxyData* pdata)
{
	/* Set server and client side references to proxy data */
	client->custom = (void*) pdata;
	server->custom = (void*) pdata;

	/* Set server callbacks */
	server->DataReceived = pf_passthrough_data_received_from_client;
	client->DataReceived = pf_passthrough_data_received_from_server;
}

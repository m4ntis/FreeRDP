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

#ifndef FREERDP_SERVER_PROXY_PFCONTEXT_H
#define FREERDP_SERVER_PROXY_PFCONTEXT_H

#include <freerdp/freerdp.h>
#include <freerdp/channels/wtsvc.h>
#include <freerdp/client/encomsp.h>
#include <freerdp/client/rdpei.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/server/rdpgfx.h>

typedef struct client_to_proxy_context clientToProxyContext;
typedef struct proxy_to_server_context proxyToServerContext;

/**
 * Context used for the client's connection to the proxy.
 */
struct client_to_proxy_context
{
	/* Underlying context of the client connection */
	rdpContext c;

	/**
	 * Context of the proxy's connection to the target server.
	 * Events from the client's context are forwarded to this one.
	 */
	proxyToServerContext* peer;
	HANDLE connectionClosed;

	/* Client to proxy related context */
	HANDLE vcm;
	HANDLE thread;

	RdpgfxServerContext* gfx;
};

BOOL init_client_to_proxy_context(freerdp_peer* client);

/**
 * Context used for the proxy's connection to the target server.
 */
struct proxy_to_server_context
{
	/* Underlying context of the server connection */
	rdpContext c;

	/**
	 * Context of the proxy's connection to the client.
	 * Events from the server's context are forwarded to this one.
	 */
	clientToProxyContext* peer;
	HANDLE connectionClosed;

	RdpeiClientContext* rdpei;
	RdpgfxClientContext* gfx;
	EncomspClientContext* encomsp;
};

proxyToServerContext* proxy_to_server_context_create(rdpSettings* clientSettings, char* host,
        DWORD port);

#endif /* FREERDP_SERVER_PROXY_PFCONTEXT_H */

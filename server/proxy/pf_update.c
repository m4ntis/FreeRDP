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

#include "pf_update.h"
#include "pf_context.h"

BOOL pf_server_refresh_rect(rdpContext* context, BYTE count,
                            const RECTANGLE_16* areas)
{
	proxyContext* pContext = (proxyContext*) context;
	return pContext->peerContext->update->RefreshRect(pContext->peerContext,
	        count, areas);
}

BOOL pf_server_suppress_output(rdpContext* context, BYTE allow,
                               const RECTANGLE_16* area)
{
	proxyContext* pContext = (proxyContext*) context;
	return pContext->peerContext->update->SuppressOutput(pContext->peerContext,
	        allow, area);
}

void register_update_callbacks(rdpUpdate* update)
{
	update->RefreshRect = pf_server_refresh_rect;
	update->SuppressOutput = pf_server_suppress_output;
}
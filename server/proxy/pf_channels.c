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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <freerdp/gdi/gfx.h>

#include <freerdp/client/rdpei.h>
#include <freerdp/client/tsmf.h>
#include <freerdp/client/rail.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/client/disp.h>

#include "pf_channels.h"
#include "pf_client.h"
#include "pf_rdpgfx.h"
#include "pf_cliprdr.h"
#include "pf_log.h"
#include "pf_disp.h"

#define TAG PROXY_TAG("channels")

void pf_OnChannelConnectedEventHandler(void* context,
                                       ChannelConnectedEventArgs* e)
{
	pClientContext* pc = (pClientContext*) context;
	pServerContext* ps = pc->pdata->ps;

	WLog_DBG(TAG, "Channel connected: %s", e->name);

	if (strcmp(e->name, RDPEI_DVC_CHANNEL_NAME) == 0)
	{
		pc->rdpei = (RdpeiClientContext*) e->pInterface;
	}
	else if (strcmp(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0)
	{
		RdpgfxClientContext* gfx;
		RdpgfxServerContext* server;

		gfx = (RdpgfxClientContext*) e->pInterface;
		pc->gfx = gfx;
		server = ps->gfx;
		pf_rdpgfx_pipeline_init(gfx, server, pc->pdata);
	}
	else if (strcmp(e->name, DISP_DVC_CHANNEL_NAME) == 0)
	{
		UINT error;
		pc->disp = (DispClientContext*) e->pInterface;
		ps->dispOpened = FALSE;

		if ((error = ps->disp->Open(ps->disp)) != CHANNEL_RC_OK)
		{
			if (error == ERROR_NOT_FOUND)
			{
				/* disp is not opened by client, ignore */
				return;
			}

			WLog_WARN(TAG, "Failed to open disp channel");
			return;
		}

		ps->dispOpened = TRUE;
		pf_disp_register_callbacks(pc->disp, ps->disp, pc->pdata);
	}
	else if (strcmp(e->name, CLIPRDR_SVC_CHANNEL_NAME) == 0)
	{
		WLog_INFO(TAG, "cliprdr, connect");
		CliprdrClientContext *cliprdr_client = e->pInterface;
		CliprdrServerContext *cliprdr_server = ps->cliprdr;
		pc->cliprdr = cliprdr_client;
		pf_cliprdr_channel_register(cliprdr_client, cliprdr_server, pc->pdata);
	}
}

void pf_OnChannelDisconnectedEventHandler(void* context,
        ChannelDisconnectedEventArgs* e)
{
	pClientContext* pc = (pClientContext*) context;
	rdpSettings* settings;
	settings = ((rdpContext*)pc)->settings;

	if (strcmp(e->name, RDPEI_DVC_CHANNEL_NAME) == 0)
	{
		pc->rdpei = NULL;
	}
	else if (strcmp(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0)
	{
		gdi_graphics_pipeline_uninit(((rdpContext*)context)->gdi, (RdpgfxClientContext*) e->pInterface);
	}
	else if (strcmp(e->name, DISP_DVC_CHANNEL_NAME) == 0)
	{
		pc->disp = NULL;
	}
	else if (strcmp(e->name, CLIPRDR_SVC_CHANNEL_NAME) == 0)
	{
		WLog_INFO(TAG, "cliprdr, disconnect");
	}
}


UINT pf_channels_init(pServerContext* ps)
{
	WLog_INFO(TAG, "pf_channels_init called");

	if (((rdpContext*) ps)->settings->SupportGraphicsPipeline)
	{
		pf_rdpgfx_init(ps);
	}

	if (WTSVirtualChannelManagerIsChannelJoined(ps->vcm, CLIPRDR_SVC_CHANNEL_NAME))
	{
		pf_cliprdr_init(ps);
	}

	return CHANNEL_RC_OK;
}

void pf_channels_free(pServerContext* ps)
{

	if (((rdpContext*) ps)->settings->SupportGraphicsPipeline)
	{
		pf_rdpgfx_free(ps);
	}

	pf_cliprdr_free(ps);
}
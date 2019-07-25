#include <winpr/crt.h>
#include <freerdp/log.h>
#include <freerdp/codec/dsp.h>
#include <freerdp/server/rdpsnd.h>
#include <freerdp/server/server-common.h>

#include "pf_disp.h"
#include "pf_log.h"

#define TAG PROXY_TAG("disp")

static void rdpsnd_activated(RdpsndServerContext* context)
{
	const AUDIO_FORMAT* agreed_format = NULL;
	UINT16 i = 0, j = 0;

	for (i = 0; i < context->num_client_formats; i++)
	{
		for (j = 0; j < context->num_server_formats; j++)
		{
			if (audio_format_compatible(&context->server_formats[j], &context->client_formats[i]))
			{
				agreed_format = &context->server_formats[j];
				break;
			}
		}

		if (agreed_format != NULL)
			break;
	}

	if (agreed_format == NULL)
	{
		WLog_ERR(TAG, "Could not agree on a audio format with the server\n");
		return;
	}

	printf("format %"PRIu16"\n", context->client_formats[i].wFormatTag);
	context->SelectFormat(context, i);
}

BOOL pf_server_rdpsnd_init(pServerContext* ps)
{
	RdpsndServerContext* rdpsnd;
	rdpsnd = ps->rdpsnd = rdpsnd_server_context_new(ps->vcm);

	if (!rdpsnd)
	{
		return FALSE;
	}

	rdpsnd->rdpcontext = (rdpContext*)ps;
	rdpsnd->data = (rdpContext*)ps;

	rdpsnd->num_server_formats = server_rdpsnd_get_formats(&rdpsnd->server_formats);

	if (rdpsnd->num_server_formats > 0)
		rdpsnd->src_format = &rdpsnd->server_formats[0];

	rdpsnd->Activated = rdpsnd_activated;
	rdpsnd->Initialize(rdpsnd, TRUE);
	return TRUE;
}




// void pf_server_rdpsnd_init(rdpShadowClient* client)
// {
// 	if (client->rdpsnd)
// 	{
// 		client->rdpsnd->Stop(client->rdpsnd);
// 		rdpsnd_server_context_free(client->rdpsnd);
// 		client->rdpsnd = NULL;
// 	}
// }


// void pf_rdpsnd_register_callbacks(DispClientContext* client, DispServerContext* server,
//                            proxyData* pdata)
// {
// 	client->custom = (void*) pdata;
// 	server->custom = (void*) pdata;

// 	 client receives from server, forward using disp server to original client 
// 	client->DisplayControlCaps = pf_disp_on_caps_control;
// 	/* server receives from client, forward to target server using disp client */
// 	server->DispMonitorLayout = pf_disp_monitor_layout;
// }

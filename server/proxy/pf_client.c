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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/utils/signal.h>

#include <freerdp/client/file.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/channels.h>
#include <freerdp/channels/channels.h>

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <freerdp/log.h>

#include "pf_channels.h"
#include "pf_gdi.h"
#include "pf_graphics.h"
#include "pf_common.h"
#include "pf_client.h"
#include "pf_context.h"
#include "pf_log.h"

#define TAG PROXY_TAG("client")

/* This function is called whenever a new frame starts.
 * It can be used to reset invalidated areas. */
static BOOL pf_begin_paint(rdpContext* context)
{
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return sContext->update->BeginPaint(sContext);
}

/* This function is called when the library completed composing a new
 * frame. Read out the changed areas and blit them to your output device.
 * The image buffer will have the format specified by gdi_init
 */
static BOOL pf_end_paint(rdpContext* context)
{
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return sContext->update->EndPaint(sContext);
}

/**
 * Called before a connection is established.
 *
 * TODO: Take client to proxy settings and use channel whitelist to filter out
 * unwanted channels.
 */
static BOOL pf_pre_connect(freerdp* instance)
{
	rdpSettings* settings;
	settings = instance->settings;
	proxyContext* pContext = (proxyContext*)instance->context;
	rdpContext* cContext = (rdpContext*)pContext->peerContext;
	/* set color depth after client to proxy negotiation */
	settings->ColorDepth = cContext->settings->ColorDepth;
	/* TODO: Consider forwarding this from client. */

	settings->OsMajorType = OSMAJORTYPE_UNIX;
	settings->OsMinorType = OSMINORTYPE_NATIVE_XSERVER;
	/**
	 * settings->OrderSupport is initialized at this point.
	 * Only override it if you plan to implement custom order
	 * callbacks or deactiveate certain features.
	 */
	/**
	 * Register the channel listeners.
	 * They are required to set up / tear down channels if they are loaded.
	 */
	PubSub_SubscribeChannelConnected(instance->context->pubSub,
	                                 pf_OnChannelConnectedEventHandler);
	PubSub_SubscribeChannelDisconnected(instance->context->pubSub,
	                                    pf_OnChannelDisconnectedEventHandler);

	/**
	 * Load all required plugins / channels / libraries specified by current
	 * settings.
	 */

	WLog_INFO(TAG, "Loading addins");
	if (!freerdp_client_load_addins(instance->context->channels,
	                                instance->settings))
	{
		WLog_ERR(TAG, "Failed to load addins");
		return FALSE;
	}

	return TRUE;
}


BOOL pf_client_bitmap_update(rdpContext* context, const BITMAP_UPDATE* bitmap)
{
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return sContext->update->BitmapUpdate(sContext, bitmap);
}

/**
 * Called after a RDP connection was successfully established.
 * Settings might have changed during negociation of client / server feature
 * support.
 *
 * Set up local framebuffers and painting callbacks.
 * If required, register pointer callbacks to change the local mouse cursor
 * when hovering over the RDP window
 */
static BOOL pf_post_connect(freerdp* instance)
{
	if (!gdi_init(instance, PIXEL_FORMAT_XRGB32))
		return FALSE;

	rdpContext* context = instance->context;
	rdpSettings* settings = instance->settings;
	rdpUpdate* update = instance->update;

	if (!pf_register_pointer(context->graphics))
		return FALSE;

	if (!settings->SoftwareGdi)
	{
		if (!pf_register_graphics(context->graphics))
		{
			WLog_ERR(TAG, "failed to register graphics");
			return FALSE;
		}

		pf_gdi_register_update_callbacks(update);
		brush_cache_register_callbacks(instance->update);
		glyph_cache_register_callbacks(instance->update);
		bitmap_cache_register_callbacks(instance->update);
		offscreen_cache_register_callbacks(instance->update);
		palette_cache_register_callbacks(instance->update);
	}

	update->BeginPaint = pf_begin_paint;
	update->EndPaint = pf_end_paint;
	update->BitmapUpdate = pf_client_bitmap_update;
	return TRUE;
}

/* This function is called whether a session ends by failure or success.
 * Clean up everything allocated by pre_connect and post_connect.
 */
static void pf_post_disconnect(freerdp* instance)
{
	proxyToServerContext* context;

	if (!instance)
		return;

	if (!instance->context)
		return;

	context = (proxyToServerContext*) instance->context;
	proxyContext* pContext = (proxyContext*)context;
	PubSub_UnsubscribeChannelConnected(instance->context->pubSub,
	                                   pf_OnChannelConnectedEventHandler);
	PubSub_UnsubscribeChannelDisconnected(instance->context->pubSub,
	                                      pf_OnChannelDisconnectedEventHandler);
	gdi_free(instance);
	rdpContext* cContext = pContext->peerContext;

	if (!pf_common_connection_aborted_by_peer(pContext))
	{
		SetEvent(pContext->connectionClosed);
		WLog_INFO(TAG, "connectionClosed event is not set; closing connection with client");
		freerdp_peer* peer = cContext->peer;
		peer->Disconnect(peer);
	}

	/*
	* It's important to avoid calling `freerdp_peer_context_free` and `freerdp_peer_free` here,
	* in order to avoid double-free. Those objects will be freed by the server when needed.
	*/
}

/**
 * RDP main loop.
 * Connects RDP, loops while running and handles event and dispatch, cleans up
 * after the connection ends.
 */
static DWORD WINAPI pf_client_thread_proc(LPVOID arg)
{
	freerdp* instance = (freerdp*)arg;
	DWORD nCount;
	DWORD status;
	HANDLE handles[64];

	char* argv[] = {"./pfreerdp", "/gfx", NULL};
	freerdp_client_settings_parse_command_line(instance->settings, 2, argv, FALSE);

	if (!freerdp_connect(instance))
	{
		WLog_ERR(TAG, "connection failure: %s", freerdp_get_last_error_string(instance->context));
		return 0;
	}

	while (!freerdp_shall_disconnect(instance))
	{
		nCount = freerdp_get_event_handles(instance->context, &handles[0], 64);

		if (nCount == 0)
		{
			WLog_ERR(TAG, "%s: freerdp_get_event_handles failed", __FUNCTION__);
			break;
		}

		status = WaitForMultipleObjects(nCount, handles, FALSE, 100);

		if (status == WAIT_FAILED)
		{
			WLog_ERR(TAG, "%s: WaitForMultipleObjects failed with %"PRIu32"", __FUNCTION__,
			         status);
			break;
		}

		if (freerdp_shall_disconnect(instance))
			break;

		if (!freerdp_check_event_handles(instance->context))
		{
			if (freerdp_get_last_error(instance->context) == FREERDP_ERROR_SUCCESS)
				WLog_ERR(TAG, "Failed to check FreeRDP event handles");

			break;
		}
	}

	freerdp_disconnect(instance);
	return 0;
}

/* Optional global initializer.
 * Here we just register a signal handler to print out stack traces
 * if available. */
static BOOL pf_client_global_init(void)
{
	if (freerdp_handle_signals() != 0)
		return FALSE;

	return TRUE;
}

/* Optional global tear down */
static void pf_client_global_uninit(void)
{
}

static int pf_logon_error_info(freerdp* instance, UINT32 data, UINT32 type)
{
	proxyToServerContext* tf;
	const char* str_data = freerdp_get_logon_error_info_data(data);
	const char* str_type = freerdp_get_logon_error_info_type(type);

	if (!instance || !instance->context)
		return -1;

	tf = (proxyToServerContext*) instance->context;
	WLog_INFO(TAG, "Logon Error Info %s [%s]", str_data, str_type);
	return 1;
}

static BOOL pf_client_new(freerdp* instance, rdpContext* context)
{
	if (!instance || !context)
		return FALSE;

	instance->PreConnect = pf_pre_connect;
	instance->PostConnect = pf_post_connect;
	instance->PostDisconnect = pf_post_disconnect;
	instance->Authenticate = client_cli_authenticate;
	/* TODO: Use a different auth methods, these are interactive with the client */
	instance->GatewayAuthenticate = client_cli_gw_authenticate;
	instance->VerifyCertificateEx = client_cli_verify_certificate_ex;
	instance->VerifyChangedCertificateEx = client_cli_verify_changed_certificate_ex;
	instance->LogonErrorInfo = pf_logon_error_info;
	return TRUE;
}


static void pf_client_free(freerdp* instance, rdpContext* context) {}

static int pf_client_start(rdpContext* context)
{
	return 0;
}

static int pf_client_stop(rdpContext* context)
{
	return 0;
}

int RdpClientEntry(RDP_CLIENT_ENTRY_POINTS* pEntryPoints)
{
	ZeroMemory(pEntryPoints, sizeof(RDP_CLIENT_ENTRY_POINTS));
	pEntryPoints->Version = RDP_CLIENT_INTERFACE_VERSION;
	pEntryPoints->Size = sizeof(RDP_CLIENT_ENTRY_POINTS_V1);
	pEntryPoints->GlobalInit = pf_client_global_init;
	pEntryPoints->GlobalUninit = pf_client_global_uninit;
	pEntryPoints->ContextSize = sizeof(proxyToServerContext);
	/* Client init and finish */
	pEntryPoints->ClientNew = pf_client_new;
	pEntryPoints->ClientFree = pf_client_free;
	pEntryPoints->ClientStart = pf_client_start;
	pEntryPoints->ClientStop = pf_client_stop;
	return 0;
}

/**
 * Starts running a client connection towards target server.
 */
DWORD WINAPI proxy_client_start(LPVOID arg)
{
	rdpContext* context = (rdpContext*)arg;

	if (freerdp_client_start(context) != 0)
		return 1;

	return pf_client_thread_proc(context->instance);
}
/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Audio Output Virtual Channel
 *
 * Copyright 2019 Armin Novak <armin.novak@thincast.com>
 * Copyright 2019 Thincast Technologies GmbH
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winpr/crt.h>
#include <winpr/stream.h>
#include <winpr/cmdline.h>
#include <winpr/sysinfo.h>
#include <freerdp/types.h>

#include "rdpsnd_main.h"
#include "../../server/proxy/pf_context.h"

typedef struct rdpsnd_proxy_plugin rdpsndProxyPlugin;

struct rdpsnd_proxy_plugin
{
	rdpsndDevicePlugin device;
	pServerContext* rdpsnd_server;
};

static BOOL rdpsnd_proxy_open(rdpsndDevicePlugin* device, const AUDIO_FORMAT* format, UINT32 latency)
{
	rdpsndProxyPlugin* proxy = (rdpsndProxyPlugin*)device;
	proxy->rdpsnd_server->rdpsnd->latency = latency;

	printf("rdpsnd_proxy_open\n");
	return TRUE;
}

static void rdpsnd_proxy_close(rdpsndDevicePlugin* device)
{
	printf("rdpsnd_proxy_close\n");

}

static BOOL rdpsnd_proxy_set_volume(rdpsndDevicePlugin* device, UINT32 value)
{
	printf("rdpsnd_proxy_set_volume\n");
	rdpsndProxyPlugin* proxy = (rdpsndProxyPlugin*)device;

	proxy->rdpsnd_server->rdpsnd->SetVolume(proxy->rdpsnd_server->rdpsnd, value, value);

	return TRUE;
}

static void rdpsnd_proxy_free(rdpsndDevicePlugin* device)
{
	rdpsndProxyPlugin* proxy = (rdpsndProxyPlugin*) device;

	if (!proxy)
		return;

	free(proxy);
}

static BOOL rdpsnd_proxy_format_supported(rdpsndDevicePlugin* device, const AUDIO_FORMAT* format)
{
	printf("rdpsnd_proxy_format_supported\n");
	if (WAVE_FORMAT_PCM == format->wFormatTag)
		return TRUE;

	printf("shit\n");
	return FALSE;
}

static BOOL rdpsnd_proxy_set_format(rdpsndDevicePlugin* device, const AUDIO_FORMAT* format,
                                   int latency)
{
	printf("rdpsnd_proxy_set_format\n");

	rdpsndProxyPlugin* proxy = (rdpsndProxyPlugin*)device;

	proxy->rdpsnd_server->rdpsnd->SelectFormat(proxy->rdpsnd_server->rdpsnd, format->wFormatTag);
	return TRUE;
}

static UINT rdpsnd_proxy_play(rdpsndDevicePlugin* device, const BYTE* data, size_t size)
{
	printf("rdpsnd_proxy_play\n");
	rdpsndProxyPlugin* proxy = (rdpsndProxyPlugin*)device;

	UINT64 start = GetTickCount();
	proxy->rdpsnd_server->rdpsnd->SendSamples(proxy->rdpsnd_server->rdpsnd, data, size / 4, start);
	return GetTickCount() - start;
}

static void rdpsnd_proxy_start(rdpsndDevicePlugin* device)
{
	printf("rdpsnd_proxy_start\n");

}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */


#ifdef BUILTIN_CHANNELS
#define freerdp_rdpsnd_client_subsystem_entry	proxy_freerdp_rdpsnd_client_subsystem_entry
#else
#define freerdp_rdpsnd_client_subsystem_entry	FREERDP_API freerdp_rdpsnd_client_subsystem_entry
#endif

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT freerdp_rdpsnd_client_subsystem_entry(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints)
{
	ADDIN_ARGV* args;
	rdpsndProxyPlugin* proxy;
	UINT ret = CHANNEL_RC_OK;
	proxy = (rdpsndProxyPlugin*) calloc(1, sizeof(rdpsndProxyPlugin));

	if (!proxy)
		return CHANNEL_RC_NO_MEMORY;

	proxy->device.Open = rdpsnd_proxy_open;
	proxy->device.FormatSupported = rdpsnd_proxy_format_supported;
	proxy->device.SetVolume = rdpsnd_proxy_set_volume;
	proxy->device.Play = rdpsnd_proxy_play;
	proxy->device.Start = rdpsnd_proxy_start;
	proxy->device.Close = rdpsnd_proxy_close;
	proxy->device.Free = rdpsnd_proxy_free;
	args = pEntryPoints->args;

	pEntryPoints->pRegisterRdpsndDevice(pEntryPoints->rdpsnd, &proxy->device);
	pClientContext* pc = proxy->device.rdpcontext;
	proxy->rdpsnd_server = pc->pdata->ps;
	printf("updated rdpsnd_server\n");
	return ret;
error:
	rdpsnd_proxy_free(&proxy->device);
	return ret;
}

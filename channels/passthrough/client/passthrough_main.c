/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Clipboard Virtual Channel
 *
 * Copyright 2009-2011 Jay Sorg
 * Copyright 2010-2011 Vic Lee
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>
#include <winpr/print.h>

#include <freerdp/types.h>
#include <freerdp/constants.h>
#include <freerdp/client/passthrough.h>

#include "passthrough_main.h"

PassthroughClientContext* passthrough_get_client_interface(passthroughPlugin* passthrough)
{
	PassthroughClientContext* pInterface;

	if (!passthrough)
		return NULL;

	pInterface = (PassthroughClientContext*) passthrough->channelEntryPoints.pInterface;
	return pInterface;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_send_data(PassthroughClientContext* context, BYTE* data, UINT32 len)
{
	passthroughPlugin* passthrough = (passthroughPlugin*) context->handle;
	UINT status;

	if (!passthrough)
	{
		status = CHANNEL_RC_BAD_INIT_HANDLE;
	}
	else
	{
		status = passthrough->channelEntryPoints.pVirtualChannelWriteEx(passthrough->InitHandle,
		         passthrough->OpenHandle,
		         data, len, data);
	}

	if (status != CHANNEL_RC_OK)
		WLog_ERR(TAG, "VirtualChannelWrite failed with %s [%08"PRIX32"]",
		         WTSErrorToString(status), status);

	return status;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_virtual_channel_event_data_received(passthroughPlugin* passthrough,
        void* pData, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags)
{
	PassthroughClientContext* context = passthrough_get_client_interface(passthrough);
	wStream* data_in;

	if ((dataFlags & CHANNEL_FLAG_SUSPEND) || (dataFlags & CHANNEL_FLAG_RESUME))
	{
		return CHANNEL_RC_OK;
	}

	if (dataFlags & CHANNEL_FLAG_FIRST)
	{
		if (passthrough->data_in)
			Stream_Free(passthrough->data_in, TRUE);

		passthrough->data_in = Stream_New(NULL, totalLength);
	}

	if (!(data_in = passthrough->data_in))
	{
		WLog_ERR(TAG, "Stream_New failed!");
		return CHANNEL_RC_NO_MEMORY;
	}

	if (!Stream_EnsureRemainingCapacity(data_in, dataLength))
	{
		Stream_Free(passthrough->data_in, TRUE);
		passthrough->data_in = NULL;
		return CHANNEL_RC_NO_MEMORY;
	}

	Stream_Write(data_in, pData, dataLength);

	if (dataFlags & CHANNEL_FLAG_LAST)
	{
		if (Stream_Capacity(data_in) != Stream_GetPosition(data_in))
		{
			WLog_ERR(TAG, "cliprdr_plugin_process_received: read error");
			return ERROR_INTERNAL_ERROR;
		}

		passthrough->data_in = NULL;
		Stream_SealLength(data_in);
		Stream_SetPosition(data_in, 0);

		/* send all received data */
		context->DataReceived(context, (BYTE*) Stream_Buffer(data_in), dataLength);
	}

	return CHANNEL_RC_OK;
}

static VOID VCAPITYPE passthrough_virtual_channel_open_event_ex(LPVOID lpUserParam, DWORD openHandle,
        UINT event,
        LPVOID pData, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags)
{
	UINT error = CHANNEL_RC_OK;
	passthroughPlugin* passthrough = (passthroughPlugin*) lpUserParam;

	if (!passthrough || (passthrough->OpenHandle != openHandle))
	{
		WLog_ERR(TAG, "error no match");
		return;
	}

	switch (event)
	{
		case CHANNEL_EVENT_DATA_RECEIVED:
			if ((error = passthrough_virtual_channel_event_data_received(passthrough, pData, dataLength,
			             totalLength, dataFlags)))
				WLog_ERR(TAG, "failed with error %"PRIu32"", error);

			break;

		case CHANNEL_EVENT_WRITE_COMPLETE:
			break;

		case CHANNEL_EVENT_USER:
			break;
	}

	if (error && passthrough->context->rdpcontext)
		setChannelError(passthrough->context->rdpcontext, error,
		                "passthrough_virtual_channel_client_thread reported an error");
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_virtual_channel_event_connected(passthroughPlugin* passthrough,
        LPVOID pData, UINT32 dataLength)
{
	UINT32 status;
	status = passthrough->channelEntryPoints.pVirtualChannelOpenEx(passthrough->InitHandle,
	         &passthrough->OpenHandle, passthrough->channelDef.name,
	         passthrough_virtual_channel_open_event_ex);

	if (status != CHANNEL_RC_OK)
	{
		WLog_ERR(TAG, "pVirtualChannelOpen failed with %s [%08"PRIX32"]",
		         WTSErrorToString(status), status);
		return status;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_virtual_channel_event_disconnected(passthroughPlugin* passthrough)
{
	UINT rc;

	if (passthrough->OpenHandle == 0)
		return CHANNEL_RC_OK;

	rc = passthrough->channelEntryPoints.pVirtualChannelCloseEx(passthrough->InitHandle, passthrough->OpenHandle);

	if (CHANNEL_RC_OK != rc)
	{
		WLog_ERR(TAG, "pVirtualChannelClose failed with %s [%08"PRIX32"]",
		         WTSErrorToString(rc), rc);
		return rc;
	}

	passthrough->OpenHandle = 0;

	if (passthrough->data_in)
	{
		Stream_Free(passthrough->data_in, TRUE);
		passthrough->data_in = NULL;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_virtual_channel_event_terminated(passthroughPlugin* passthrough)
{
	passthrough->InitHandle = 0;
	free(passthrough->context);
	free(passthrough);
	return CHANNEL_RC_OK;
}

static VOID VCAPITYPE passthrough_virtual_channel_init_event_ex(LPVOID lpUserParam, LPVOID pInitHandle,
        UINT event, LPVOID pData, UINT dataLength)
{
	UINT error = CHANNEL_RC_OK;
	passthroughPlugin* passthrough = (passthroughPlugin*) lpUserParam;

	if (!passthrough || (passthrough->InitHandle != pInitHandle))
	{
		WLog_ERR(TAG, "error no match");
		return;
	}

	switch (event)
	{
		case CHANNEL_EVENT_CONNECTED:
			if ((error = passthrough_virtual_channel_event_connected(passthrough, pData, dataLength)))
				WLog_ERR(TAG, "cliprdr_virtual_channel_event_connected failed with error %"PRIu32"!",
				         error);

			break;

		case CHANNEL_EVENT_DISCONNECTED:
			if ((error = passthrough_virtual_channel_event_disconnected(passthrough)))
				WLog_ERR(TAG,
				         "cliprdr_virtual_channel_event_disconnected failed with error %"PRIu32"!", error);

			break;

		case CHANNEL_EVENT_TERMINATED:
			if ((error = passthrough_virtual_channel_event_terminated(passthrough)))
				WLog_ERR(TAG, "cliprdr_virtual_channel_event_terminated failed with error %"PRIu32"!",
				         error);

			break;
	}

	if (error && passthrough->context->rdpcontext)
		setChannelError(passthrough->context->rdpcontext, error,
		                "passthrough_virtual_channel_init_event reported an error");
}

#define VirtualChannelEntryEx plex_VirtualChannelEntryEx

BOOL VCAPITYPE VirtualChannelEntryEx(PCHANNEL_ENTRY_POINTS pEntryPoints, PVOID pInitHandle)
{
	UINT rc;
	passthroughPlugin* passthrough;
	PassthroughClientContext* context = NULL;
	CHANNEL_ENTRY_POINTS_FREERDP_EX* pEntryPointsEx;
	passthrough = (passthroughPlugin*) calloc(1, sizeof(passthroughPlugin));


	if (!passthrough)
	{
		WLog_ERR(TAG, "calloc failed!");
		return FALSE;
	}

	passthrough->channelDef.options =
	    CHANNEL_OPTION_INITIALIZED |
	    CHANNEL_OPTION_ENCRYPT_RDP |
	    CHANNEL_OPTION_COMPRESS_RDP |
	    CHANNEL_OPTION_SHOW_PROTOCOL;
	sprintf_s(passthrough->channelDef.name, ARRAYSIZE(passthrough->channelDef.name), "rdpsnd");
	pEntryPointsEx = (CHANNEL_ENTRY_POINTS_FREERDP_EX*) pEntryPoints;

	if ((pEntryPointsEx->cbSize >= sizeof(CHANNEL_ENTRY_POINTS_FREERDP_EX)) &&
	    (pEntryPointsEx->MagicNumber == FREERDP_CHANNEL_MAGIC_NUMBER))
	{
		context = (PassthroughClientContext*) calloc(1, sizeof(PassthroughClientContext));

		if (!context)
		{
			free(passthrough);
			WLog_ERR(TAG, "calloc failed!");
			return FALSE;
		}

		context->handle = (void*) passthrough;
		context->custom = NULL;
		context->DataReceived = NULL;
		context->SendData = passthrough_send_data;

		passthrough->context = context;
		context->rdpcontext = pEntryPointsEx->context;
	}

	passthrough->log = WLog_Get("com.freerdp.channels.passthrough.client");
	WLog_Print(passthrough->log, WLOG_DEBUG, "VirtualChannelEntryEx");
	CopyMemory(&(passthrough->channelEntryPoints), pEntryPoints,
	           sizeof(CHANNEL_ENTRY_POINTS_FREERDP_EX));
	passthrough->InitHandle = pInitHandle;
	rc = passthrough->channelEntryPoints.pVirtualChannelInitEx(passthrough, context, pInitHandle,
	        &passthrough->channelDef, 1, VIRTUAL_CHANNEL_VERSION_WIN2000,
	        passthrough_virtual_channel_init_event_ex);

	if (CHANNEL_RC_OK != rc)
	{
		WLog_ERR(TAG, "pVirtualChannelInit failed with %s [%08"PRIX32"]",
		         WTSErrorToString(rc), rc);
		free(passthrough->context);
		free(passthrough);
		return FALSE;
	}

	passthrough->channelEntryPoints.pInterface = context;
	return TRUE;
}

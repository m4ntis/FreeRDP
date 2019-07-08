/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Clipboard Virtual Channel Extension
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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
#include <winpr/stream.h>

#include <freerdp/channels/log.h>
#include <freerdp/server/passthrough.h>

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT cliprdr_server_init(CliprdrServerContext* context)
{
	UINT32 generalFlags;
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_MONITOR_READY monitorReady;
	CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;
	UINT error;
	ZeroMemory(&capabilities, sizeof(capabilities));
	ZeroMemory(&monitorReady, sizeof(monitorReady));
	generalFlags = 0;

	if (context->useLongFormatNames)
		generalFlags |= CB_USE_LONG_FORMAT_NAMES;

	if (context->streamFileClipEnabled)
		generalFlags |= CB_STREAM_FILECLIP_ENABLED;

	if (context->fileClipNoFilePaths)
		generalFlags |= CB_FILECLIP_NO_FILE_PATHS;

	if (context->canLockClipData)
		generalFlags |= CB_CAN_LOCK_CLIPDATA;

	capabilities.msgType = CB_CLIP_CAPS;
	capabilities.msgFlags = 0;
	capabilities.dataLen = 4 + CB_CAPSTYPE_GENERAL_LEN;
	capabilities.cCapabilitiesSets = 1;
	capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*) &generalCapabilitySet;
	generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
	generalCapabilitySet.capabilitySetLength = CB_CAPSTYPE_GENERAL_LEN;
	generalCapabilitySet.version = CB_CAPS_VERSION_2;
	generalCapabilitySet.generalFlags = generalFlags;

	if ((error = context->ServerCapabilities(context, &capabilities)))
	{
		WLog_ERR(TAG, "ServerCapabilities failed with error %"PRIu32"!", error);
		return error;
	}

	if ((error = context->MonitorReady(context, &monitorReady)))
	{
		WLog_ERR(TAG, "MonitorReady failed with error %"PRIu32"!", error);
		return error;
	}

	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT cliprdr_server_read(CliprdrServerContext* context)
{
	wStream* s;
	size_t position;
	DWORD BytesToRead;
	DWORD BytesReturned;
	CLIPRDR_HEADER header;
	PassthroughServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;
	UINT error;
	DWORD status;
	s = cliprdr->s;

	if (Stream_GetPosition(s) < CLIPRDR_HEADER_LENGTH)
	{
		BytesReturned = 0;
		BytesToRead = CLIPRDR_HEADER_LENGTH - Stream_GetPosition(s);
		status = WaitForSingleObject(cliprdr->ChannelEvent, 0);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
			return error;
		}

		if (status == WAIT_TIMEOUT)
			return CHANNEL_RC_OK;

		if (!WTSVirtualChannelRead(cliprdr->ChannelHandle, 0,
		                           (PCHAR) Stream_Pointer(s), BytesToRead, &BytesReturned))
		{
			WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
			return ERROR_INTERNAL_ERROR;
		}

		Stream_Seek(s, BytesReturned);
	}

	if (Stream_GetPosition(s) >= CLIPRDR_HEADER_LENGTH)
	{
		position = Stream_GetPosition(s);
		Stream_SetPosition(s, 0);
		Stream_Read_UINT16(s, header.msgType); /* msgType (2 bytes) */
		Stream_Read_UINT16(s, header.msgFlags); /* msgFlags (2 bytes) */
		Stream_Read_UINT32(s, header.dataLen); /* dataLen (4 bytes) */

		if (!Stream_EnsureCapacity(s, (header.dataLen + CLIPRDR_HEADER_LENGTH)))
		{
			WLog_ERR(TAG, "Stream_EnsureCapacity failed!");
			return CHANNEL_RC_NO_MEMORY;
		}

		Stream_SetPosition(s, position);

		if (Stream_GetPosition(s) < (header.dataLen + CLIPRDR_HEADER_LENGTH))
		{
			BytesReturned = 0;
			BytesToRead = (header.dataLen + CLIPRDR_HEADER_LENGTH) - Stream_GetPosition(s);
			status = WaitForSingleObject(cliprdr->ChannelEvent, 0);

			if (status == WAIT_FAILED)
			{
				error = GetLastError();
				WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
				return error;
			}

			if (status == WAIT_TIMEOUT)
				return CHANNEL_RC_OK;

			if (!WTSVirtualChannelRead(cliprdr->ChannelHandle, 0,
			                           (PCHAR) Stream_Pointer(s), BytesToRead, &BytesReturned))
			{
				WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
				return ERROR_INTERNAL_ERROR;
			}

			Stream_Seek(s, BytesReturned);
		}

		if (Stream_GetPosition(s) >= (header.dataLen + CLIPRDR_HEADER_LENGTH))
		{
			Stream_SetPosition(s, (header.dataLen + CLIPRDR_HEADER_LENGTH));
			Stream_SealLength(s);
			Stream_SetPosition(s, CLIPRDR_HEADER_LENGTH);

			if ((error = cliprdr_server_receive_pdu(context, s, &header)))
			{
				WLog_ERR(TAG, "cliprdr_server_receive_pdu failed with error code %"PRIu32"!", error);
				return error;
			}

			Stream_SetPosition(s, 0);
			/* check for trailing zero bytes */
			status = WaitForSingleObject(cliprdr->ChannelEvent, 0);

			if (status == WAIT_FAILED)
			{
				error = GetLastError();
				WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
				return error;
			}

			if (status == WAIT_TIMEOUT)
				return CHANNEL_RC_OK;

			BytesReturned = 0;
			BytesToRead = 4;

			if (!WTSVirtualChannelRead(cliprdr->ChannelHandle, 0,
			                           (PCHAR) Stream_Pointer(s), BytesToRead, &BytesReturned))
			{
				WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
				return ERROR_INTERNAL_ERROR;
			}

			if (BytesReturned == 4)
			{
				Stream_Read_UINT16(s, header.msgType); /* msgType (2 bytes) */
				Stream_Read_UINT16(s, header.msgFlags); /* msgFlags (2 bytes) */

				if (!header.msgType)
				{
					/* ignore trailing bytes */
					Stream_SetPosition(s, 0);
				}
			}
			else
			{
				Stream_Seek(s, BytesReturned);
			}
		}
	}

	return CHANNEL_RC_OK;
}

static DWORD WINAPI cliprdr_server_thread(LPVOID arg)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE ChannelEvent;
	CliprdrServerContext* context = (CliprdrServerContext*) arg;
	CliprdrServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;
	UINT error;

	ChannelEvent = context->GetEventHandle(context);
	nCount = 0;
	events[nCount++] = cliprdr->StopEvent;
	events[nCount++] = ChannelEvent;

	if ((error = cliprdr_server_init(context)))
	{
		WLog_ERR(TAG, "cliprdr_server_init failed with error %"PRIu32"!", error);
		goto out;
	}

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForMultipleObjects failed with error %"PRIu32"", error);
			goto out;
		}

		status = WaitForSingleObject(cliprdr->StopEvent, 0);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
			goto out;
		}

		if (status == WAIT_OBJECT_0)
			break;

		status = WaitForSingleObject(ChannelEvent, 0);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
			goto out;
		}

		if (status == WAIT_OBJECT_0)
		{
			if ((error = context->CheckEventHandle(context)))
			{
				WLog_ERR(TAG, "CheckEventHandle failed with error %"PRIu32"!", error);
				break;
			}
		}
	}

out:

	if (error && context->rdpcontext)
		setChannelError(context->rdpcontext, error,
		                "cliprdr_server_thread reported an error");

	ExitThread(error);
	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT cliprdr_server_open(CliprdrServerContext* context)
{
	void* buffer = NULL;
	DWORD BytesReturned = 0;
	CliprdrServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;
	cliprdr->ChannelHandle = WTSVirtualChannelOpen(cliprdr->vcm,
	                         WTS_CURRENT_SESSION, "cliprdr");

	if (!cliprdr->ChannelHandle)
	{
		WLog_ERR(TAG, "WTSVirtualChannelOpen for cliprdr failed!");
		return ERROR_INTERNAL_ERROR;
	}

	cliprdr->ChannelEvent = NULL;

	if (WTSVirtualChannelQuery(cliprdr->ChannelHandle, WTSVirtualEventHandle,
	                           &buffer, &BytesReturned))
	{
		if (BytesReturned != sizeof(HANDLE))
		{
			WLog_ERR(TAG, "BytesReturned has not size of HANDLE!");
			return ERROR_INTERNAL_ERROR;
		}

		CopyMemory(&(cliprdr->ChannelEvent), buffer, sizeof(HANDLE));
		WTSFreeMemory(buffer);
	}

	if (!cliprdr->ChannelEvent)
	{
		WLog_ERR(TAG, "WTSVirtualChannelQuery for cliprdr failed!");
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT cliprdr_server_close(CliprdrServerContext* context)
{
	CliprdrServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;

	if (cliprdr->ChannelHandle)
	{
		WTSVirtualChannelClose(cliprdr->ChannelHandle);
		cliprdr->ChannelHandle = NULL;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT cliprdr_server_start(CliprdrServerContext* context)
{
	CliprdrServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;
	UINT error;

	if (!cliprdr->ChannelHandle)
	{
		if ((error = context->Open(context)))
		{
			WLog_ERR(TAG, "Open failed!");
			return error;
		}
	}

	if (!(cliprdr->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		WLog_ERR(TAG, "CreateEvent failed!");
		return ERROR_INTERNAL_ERROR;
	}

	if (!(cliprdr->Thread = CreateThread(NULL, 0, cliprdr_server_thread, (void*) context, 0, NULL)))
	{
		WLog_ERR(TAG, "CreateThread failed!");
		CloseHandle(cliprdr->StopEvent);
		cliprdr->StopEvent = NULL;
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT cliprdr_server_stop(CliprdrServerContext* context)
{
	UINT error = CHANNEL_RC_OK;
	CliprdrServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;

	if (cliprdr->StopEvent)
	{
		SetEvent(cliprdr->StopEvent);

		if (WaitForSingleObject(cliprdr->Thread, INFINITE) == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
			return error;
		}

		CloseHandle(cliprdr->Thread);
		CloseHandle(cliprdr->StopEvent);
	}

	if (cliprdr->ChannelHandle)
		return context->Close(context);

	return error;
}

static HANDLE cliprdr_server_get_event_handle(CliprdrServerContext* context)
{
	CliprdrServerPrivate* cliprdr = (CliprdrServerPrivate*) context->handle;
	return cliprdr->ChannelEvent;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT cliprdr_server_check_event_handle(CliprdrServerContext* context)
{
	return cliprdr_server_read(context);
}

CliprdrServerContext* cliprdr_server_context_new(HANDLE vcm)
{
	CliprdrServerContext* context;
	CliprdrServerPrivate* cliprdr;
	context = (CliprdrServerContext*) calloc(1, sizeof(CliprdrServerContext));

	if (context)
	{
		context->Open = cliprdr_server_open;
		context->Close = cliprdr_server_close;
		context->Start = cliprdr_server_start;
		context->Stop = cliprdr_server_stop;
		context->GetEventHandle = cliprdr_server_get_event_handle;
		context->CheckEventHandle = cliprdr_server_check_event_handle;
		context->ServerCapabilities = cliprdr_server_capabilities;
		context->MonitorReady = cliprdr_server_monitor_ready;
		context->ServerFormatList = cliprdr_server_format_list;
		context->ServerFormatListResponse = cliprdr_server_format_list_response;
		context->ServerLockClipboardData = cliprdr_server_lock_clipboard_data;
		context->ServerUnlockClipboardData = cliprdr_server_unlock_clipboard_data;
		context->ServerFormatDataRequest = cliprdr_server_format_data_request;
		context->ServerFormatDataResponse = cliprdr_server_format_data_response;
		context->ServerFileContentsRequest = cliprdr_server_file_contents_request;
		context->ServerFileContentsResponse = cliprdr_server_file_contents_response;
		cliprdr = context->handle = (CliprdrServerPrivate*) calloc(1,
		                            sizeof(CliprdrServerPrivate));

		if (cliprdr)
		{
			cliprdr->vcm = vcm;
			cliprdr->s = Stream_New(NULL, 4096);

			if (!cliprdr->s)
			{
				WLog_ERR(TAG, "Stream_New failed!");
				free(context->handle);
				free(context);
				return NULL;
			}
		}
		else
		{
			WLog_ERR(TAG, "calloc failed!");
			free(context);
			return NULL;
		}
	}

	return context;
}

void cliprdr_server_context_free(CliprdrServerContext* context)
{
	CliprdrServerPrivate* cliprdr;

	if (!context)
		return;

	cliprdr = (CliprdrServerPrivate*) context->handle;

	if (cliprdr)
	{
		Stream_Free(cliprdr->s, TRUE);
		free(cliprdr->temporaryDirectory);
	}

	free(context->handle);
	free(context);
}

/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Passthrough virtual channel
 *
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

#include <winpr/crt.h>
#include <winpr/print.h>
#include <winpr/stream.h>

#include <freerdp/channels/log.h>
#include <freerdp/server/passthrough.h>

#include "passthrough_main.h"

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT passthrough_server_read(PassthroughServerContext* context)
{
	UINT error = CHANNEL_RC_OK;

	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;
	wStream* s = passthrough->s;
	DWORD BytesReturned;

	Stream_SetPosition(s, 0);

	if (!WTSVirtualChannelRead(passthrough->ChannelHandle, 0,
	                           (PCHAR) Stream_Pointer(s), Stream_Capacity(s), &BytesReturned))
	{
		WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
		return GetLastError();
	}

	IFCALLRET(context->DataReceived, error, context, Stream_Buffer(s), BytesReturned);
	return error;
}

static DWORD WINAPI passthrough_server_thread(LPVOID arg)
{
	UINT error;
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE ChannelEvent;
	PassthroughServerContext* context = (PassthroughServerContext*) arg;
	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;
	ChannelEvent = passthrough->ChannelEvent;
	nCount = 0;
	events[nCount++] = passthrough->StopEvent;
	events[nCount++] = ChannelEvent;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForMultipleObjects failed with error %"PRIu32"", error);
			goto out;
		}

		status = WaitForSingleObject(passthrough->StopEvent, 0);

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
			if ((error = passthrough_server_read(context)))
			{
				WLog_ERR(TAG, "CheckEventHandle failed with error %"PRIu32"!", error);
				break;
			}
		}
	}

out:

	if (error && context->rdpcontext)
		setChannelError(context->rdpcontext, error,
		                "passthrough_server_thread reported an error");

	ExitThread(error);
	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_server_open(PassthroughServerContext* context)
{
	void* buffer = NULL;
	DWORD BytesReturned = 0;
	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;
	passthrough->ChannelHandle = WTSVirtualChannelOpen(passthrough->vcm,
	                         WTS_CURRENT_SESSION, "bkey66");

	if (!passthrough->ChannelHandle)
	{
		WLog_ERR(TAG, "WTSVirtualChannelOpen for passthrough failed!");
		return ERROR_INTERNAL_ERROR;
	}

	passthrough->ChannelEvent = NULL;

	if (WTSVirtualChannelQuery(passthrough->ChannelHandle, WTSVirtualEventHandle,
	                           &buffer, &BytesReturned))
	{
		if (BytesReturned != sizeof(HANDLE))
		{
			WLog_ERR(TAG, "BytesReturned has not size of HANDLE!");
			return ERROR_INTERNAL_ERROR;
		}

		CopyMemory(&(passthrough->ChannelEvent), buffer, sizeof(HANDLE));
		WTSFreeMemory(buffer);
	}

	if (!passthrough->ChannelEvent)
	{
		WLog_ERR(TAG, "WTSVirtualChannelQuery for passthrough failed!");
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_server_close(PassthroughServerContext* context)
{
	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;

	if (passthrough->ChannelHandle)
	{
		WTSVirtualChannelClose(passthrough->ChannelHandle);
		passthrough->ChannelHandle = NULL;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_server_start(PassthroughServerContext* context)
{
	UINT error;
	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;

	if (!passthrough->ChannelHandle)
	{
		if ((error = context->Open(context)))
		{
			WLog_ERR(TAG, "Open failed!");
			return error;
		}
	}

	if (!(passthrough->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		WLog_ERR(TAG, "CreateEvent failed!");
		return ERROR_INTERNAL_ERROR;
	}

	if (!(passthrough->Thread = CreateThread(NULL, 0, passthrough_server_thread, (void*) context, 0, NULL)))
	{
		WLog_ERR(TAG, "CreateThread failed!");
		CloseHandle(passthrough->StopEvent);
		passthrough->StopEvent = NULL;
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}

static UINT passthrough_server_send_data(PassthroughServerContext* context, const BYTE* data, UINT32 len)
{
	DWORD written;
	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;

	if (!WTSVirtualChannelWrite(passthrough->ChannelHandle, (PCHAR) data, len, &written))
	{
		WLog_ERR(TAG, "WTSVirtualChannelWrite failed!");
		return ERROR_INTERNAL_ERROR;
	}

	if (len != written)
	{
		WLog_WARN(TAG, "passthrough_server_send_data, len (%"PRIu32") != written (%"PRIu32")", len, written);
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}
/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT passthrough_server_stop(PassthroughServerContext* context)
{
	UINT error = CHANNEL_RC_OK;
	PassthroughServerPrivate* passthrough = (PassthroughServerPrivate*) context->handle;

	if (passthrough->StopEvent)
	{
		SetEvent(passthrough->StopEvent);

		if (WaitForSingleObject(passthrough->Thread, INFINITE) == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %"PRIu32"", error);
			return error;
		}

		CloseHandle(passthrough->Thread);
		CloseHandle(passthrough->StopEvent);
	}

	if (passthrough->ChannelHandle)
		return context->Close(context);

	return error;
}

PassthroughServerContext* passthrough_server_context_new(HANDLE vcm)
{
	PassthroughServerContext* context;
	PassthroughServerPrivate* passthrough;
	context = (PassthroughServerContext*) calloc(1, sizeof(PassthroughServerContext));

	if (context)
	{
		context->Open = passthrough_server_open;
		context->Close = passthrough_server_close;
		context->Start = passthrough_server_start;
		context->Stop = passthrough_server_stop;
		context->SendData = passthrough_server_send_data;
		
		passthrough = context->handle = (PassthroughServerPrivate*) calloc(1,
		                            sizeof(PassthroughServerPrivate));

		if (passthrough)
		{
			passthrough->vcm = vcm;
			passthrough->s = Stream_New(NULL, 4096);

			if (!passthrough->s)
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

void passthrough_server_context_free(PassthroughServerContext* context)
{
	PassthroughServerPrivate* passthrough;

	if (!context)
		return;

	passthrough = (PassthroughServerPrivate*) context->handle;

	if (passthrough)
	{
		Stream_Free(passthrough->s, TRUE);
	}

	free(context->handle);
	free(context);
}

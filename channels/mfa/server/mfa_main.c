/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * MFA Virtual Channel Extension
 *
 * Copyright 2019 Idan Freiberg <speidy@gmail.com>
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
#include "mfa_main.h"

// TODO: put protocol flow here

wStream* mfa_server_packet_new(UINT16 msgType, UINT16 msgFlags, UINT32 dataLen)
{
	wStream* s;
	s = Stream_New(NULL, dataLen + 8);

	if (!s)
	{
		WLog_ERR(TAG, "Stream_New failed!");
		return NULL;
	}

	Stream_Write_UINT16(s, msgType);
	Stream_Write_UINT16(s, msgFlags);
	/* Write actual length after the entire packet has been constructed. */
	Stream_Seek(s, 4);
	return s;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT mfa_server_packet_send(MfaServerPrivate* mfa, wStream* s)
{
	size_t pos;
	BOOL status;
	UINT32 dataLen;
	UINT32 written;
	pos = Stream_GetPosition(s);
	dataLen = pos - 8;
	Stream_SetPosition(s, 4);
	Stream_Write_UINT32(s, dataLen);
	Stream_SetPosition(s, pos);
	status = WTSVirtualChannelWrite(mfa->ChannelHandle, (PCHAR)Stream_Buffer(s), Stream_Length(s),
	                                &written);
	Stream_Free(s, TRUE);
	return status ? CHANNEL_RC_OK : ERROR_INTERNAL_ERROR;
}

static UINT mfa_server_receive_client_cancelled(MfaServerContext* context, wStream* s,
                                                MFA_HEADER* header)
{
	UINT error = CHANNEL_RC_OK;

	IFCALLRET(context->ClientCancelled, error, context);
	if (error)
	{
		WLog_ERR(TAG, "ClientCancelled failed with error %" PRIu32 "!", error);
	}

	return error;
}

static UINT mfa_server_receive_client_token(MfaServerContext* context, wStream* s,
                                            MFA_HEADER* header)
{
	UINT error = CHANNEL_RC_OK;
	MFA_CLIENT_TOKEN ct;

	Stream_Read_UINT32(s, ct.cbTokenLen);
	ct.TokenData = malloc(ct.cbTokenLen);
	if (ct.TokenData == NULL)
	{
		return CHANNEL_RC_NO_MEMORY;
	}
	Stream_Read(s, ct.TokenData, ct.cbTokenLen);

	IFCALLRET(context->ClientToken, error, context, &ct);
	if (error)
	{
		WLog_ERR(TAG, "ClientToken failed with error %" PRIu32 "!", error);
	}

	free(ct.TokenData);
	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_receive_pdu(MfaServerContext* context, wStream* s, MFA_HEADER* header)
{
	UINT error;
	WLog_INFO(TAG,
	          "MfaServerReceivePdu: msgType: %" PRIu16 " msgFlags: 0x%04" PRIX16
	          " dataLen: %" PRIu32 "",
	          header->msgType, header->msgFlags, header->dataLen);

	switch (header->msgType)
	{
	case CB_CLIENT_TOKEN:
		if ((error = mfa_server_receive_client_token(context, s, header)))
			WLog_ERR(TAG, "mfa_server_receive_client_token failed with error %" PRIu32 "!", error);
		break;

	case CB_CLIENT_CANCELLED:
		if ((error = mfa_server_receive_client_cancelled(context, s, header)))
			WLog_ERR(TAG, "mfa_server_receive_client_cancelled failed with error %" PRIu32 "!", error);
		break;

	default:
		error = ERROR_INVALID_DATA;
		WLog_DBG(TAG, "Unexpected clipboard PDU type: %" PRIu16 "", header->msgType);
		break;
	}

	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_send_server_ready(MfaServerContext* context, const MFA_SERVER_READY* server_ready)
{
	wStream* s;
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	s = mfa_server_packet_new(CB_SERVER_READY, 0, 2);
	if (!s)
	{
		WLog_ERR(TAG, "mfa_server_packet_new failed!");
		return ERROR_INTERNAL_ERROR;
	}

	Stream_Write_UINT16(s, server_ready->version); /* version (2 bytes) */

	WLog_DBG(TAG, "mfa_send_server_ready");
	return mfa_server_packet_send(mfa, s);
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_send_server_token_response(MfaServerContext* context, const enum MFA_FLAGS flags)
{
	wStream* s;
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	s = mfa_server_packet_new(CB_SERVER_TOKEN_RESPONSE, flags, 0);
	if (!s)
	{
		WLog_ERR(TAG, "mfa_server_packet_new failed!");
		return ERROR_INTERNAL_ERROR;
	}

	// no data

	WLog_DBG(TAG, "mfa_send_server_token_response");
	return mfa_server_packet_send(mfa, s);
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_init(MfaServerContext* context)
{
	UINT error = CHANNEL_RC_OK;

	/* Send server ready PDU */
	MFA_SERVER_READY sr;
	sr.version = MFA_VERSION_1;

	if ((error = context->ServerReady(context, &sr)))
	{
		WLog_ERR(TAG, "ServerReady failed with error %" PRIu32 "!", error);
		return error;
	}

	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT mfa_server_read(MfaServerContext* context)
{
	wStream* s;
	size_t position;
	DWORD BytesToRead;
	DWORD BytesReturned;
	MFA_HEADER header;
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	UINT error;
	DWORD status;
	s = mfa->s;

	if (Stream_GetPosition(s) < MFA_HEADER_LENGTH)
	{
		BytesReturned = 0;
		BytesToRead = MFA_HEADER_LENGTH - Stream_GetPosition(s);
		status = WaitForSingleObject(mfa->ChannelEvent, 0);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %" PRIu32 "", error);
			return error;
		}

		if (status == WAIT_TIMEOUT)
			return CHANNEL_RC_OK;

		if (!WTSVirtualChannelRead(mfa->ChannelHandle, 0, (PCHAR)Stream_Pointer(s), BytesToRead,
		                           &BytesReturned))
		{
			WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
			return ERROR_INTERNAL_ERROR;
		}

		Stream_Seek(s, BytesReturned);
	}

	if (Stream_GetPosition(s) >= MFA_HEADER_LENGTH)
	{
		position = Stream_GetPosition(s);
		Stream_SetPosition(s, 0);
		Stream_Read_UINT16(s, header.msgType);  /* msgType (2 bytes) */
		Stream_Read_UINT16(s, header.msgFlags); /* msgFlags (2 bytes) */
		Stream_Read_UINT32(s, header.dataLen);  /* dataLen (4 bytes) */

		if (!Stream_EnsureCapacity(s, (header.dataLen + MFA_HEADER_LENGTH)))
		{
			WLog_ERR(TAG, "Stream_EnsureCapacity failed!");
			return CHANNEL_RC_NO_MEMORY;
		}

		Stream_SetPosition(s, position);

		if (Stream_GetPosition(s) < (header.dataLen + MFA_HEADER_LENGTH))
		{
			BytesReturned = 0;
			BytesToRead = (header.dataLen + MFA_HEADER_LENGTH) - Stream_GetPosition(s);
			status = WaitForSingleObject(mfa->ChannelEvent, 0);

			if (status == WAIT_FAILED)
			{
				error = GetLastError();
				WLog_ERR(TAG, "WaitForSingleObject failed with error %" PRIu32 "", error);
				return error;
			}

			if (status == WAIT_TIMEOUT)
				return CHANNEL_RC_OK;

			if (!WTSVirtualChannelRead(mfa->ChannelHandle, 0, (PCHAR)Stream_Pointer(s), BytesToRead,
			                           &BytesReturned))
			{
				WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
				return ERROR_INTERNAL_ERROR;
			}

			Stream_Seek(s, BytesReturned);
		}

		if (Stream_GetPosition(s) >= (header.dataLen + MFA_HEADER_LENGTH))
		{
			Stream_SetPosition(s, (header.dataLen + MFA_HEADER_LENGTH));
			Stream_SealLength(s);
			Stream_SetPosition(s, MFA_HEADER_LENGTH);

			if ((error = mfa_server_receive_pdu(context, s, &header)))
			{
				WLog_ERR(TAG, "mfa_server_receive_pdu failed with error code %" PRIu32 "!", error);
				return error;
			}

			Stream_SetPosition(s, 0);
			/* check for trailing zero bytes */
			status = WaitForSingleObject(mfa->ChannelEvent, 0);

			if (status == WAIT_FAILED)
			{
				error = GetLastError();
				WLog_ERR(TAG, "WaitForSingleObject failed with error %" PRIu32 "", error);
				return error;
			}

			if (status == WAIT_TIMEOUT)
				return CHANNEL_RC_OK;

			BytesReturned = 0;
			BytesToRead = 4;

			if (!WTSVirtualChannelRead(mfa->ChannelHandle, 0, (PCHAR)Stream_Pointer(s), BytesToRead,
			                           &BytesReturned))
			{
				WLog_ERR(TAG, "WTSVirtualChannelRead failed!");
				return ERROR_INTERNAL_ERROR;
			}

			if (BytesReturned == 4)
			{
				Stream_Read_UINT16(s, header.msgType);  /* msgType (2 bytes) */
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

static DWORD WINAPI mfa_server_thread(LPVOID arg)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE ChannelEvent;
	MfaServerContext* context = (MfaServerContext*)arg;
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	UINT error;

	ChannelEvent = context->GetEventHandle(context);
	nCount = 0;
	events[nCount++] = mfa->StopEvent;
	events[nCount++] = ChannelEvent;

	if ((error = mfa_server_init(context)))
	{
		WLog_ERR(TAG, "mfa_server_init failed with error %" PRIu32 "!", error);
		goto out;
	}

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForMultipleObjects failed with error %" PRIu32 "", error);
			goto out;
		}

		status = WaitForSingleObject(mfa->StopEvent, 0);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %" PRIu32 "", error);
			goto out;
		}

		if (status == WAIT_OBJECT_0)
			break;

		status = WaitForSingleObject(ChannelEvent, 0);

		if (status == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %" PRIu32 "", error);
			goto out;
		}

		if (status == WAIT_OBJECT_0)
		{
			if ((error = context->CheckEventHandle(context)))
			{
				WLog_ERR(TAG, "CheckEventHandle failed with error %" PRIu32 "!", error);
				break;
			}
		}
	}

out:

	if (error && context->rdpcontext)
		setChannelError(context->rdpcontext, error, "mfa_server_thread reported an error");

	ExitThread(error);
	return error;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_open(MfaServerContext* context)
{
	void* buffer = NULL;
	DWORD BytesReturned = 0;
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	mfa->ChannelHandle = WTSVirtualChannelOpen(mfa->vcm, WTS_CURRENT_SESSION, MFA_SVC_CHANNEL_NAME);

	if (!mfa->ChannelHandle)
	{
		WLog_ERR(TAG, "WTSVirtualChannelOpen for mfa failed!");
		return ERROR_INTERNAL_ERROR;
	}

	mfa->ChannelEvent = NULL;

	if (WTSVirtualChannelQuery(mfa->ChannelHandle, WTSVirtualEventHandle, &buffer, &BytesReturned))
	{
		if (BytesReturned != sizeof(HANDLE))
		{
			WLog_ERR(TAG, "BytesReturned has not size of HANDLE!");
			return ERROR_INTERNAL_ERROR;
		}

		CopyMemory(&(mfa->ChannelEvent), buffer, sizeof(HANDLE));
		WTSFreeMemory(buffer);
	}

	if (!mfa->ChannelEvent)
	{
		WLog_ERR(TAG, "WTSVirtualChannelQuery for mfa failed!");
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_close(MfaServerContext* context)
{
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;

	if (mfa->ChannelHandle)
	{
		WTSVirtualChannelClose(mfa->ChannelHandle);
		mfa->ChannelHandle = NULL;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_start(MfaServerContext* context)
{
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	UINT error;

	if (!mfa->ChannelHandle)
	{
		if ((error = context->Open(context)))
		{
			WLog_ERR(TAG, "Open failed!");
			return error;
		}
	}

	if (!(mfa->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		WLog_ERR(TAG, "CreateEvent failed!");
		return ERROR_INTERNAL_ERROR;
	}

	if (!(mfa->Thread = CreateThread(NULL, 0, mfa_server_thread, (void*)context, 0, NULL)))
	{
		WLog_ERR(TAG, "CreateThread failed!");
		CloseHandle(mfa->StopEvent);
		mfa->StopEvent = NULL;
		return ERROR_INTERNAL_ERROR;
	}

	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_stop(MfaServerContext* context)
{
	UINT error = CHANNEL_RC_OK;
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;

	if (mfa->StopEvent)
	{
		SetEvent(mfa->StopEvent);

		if (WaitForSingleObject(mfa->Thread, INFINITE) == WAIT_FAILED)
		{
			error = GetLastError();
			WLog_ERR(TAG, "WaitForSingleObject failed with error %" PRIu32 "", error);
			return error;
		}

		CloseHandle(mfa->Thread);
		CloseHandle(mfa->StopEvent);
	}

	if (mfa->ChannelHandle)
		return context->Close(context);

	return error;
}

static HANDLE mfa_server_get_event_handle(MfaServerContext* context)
{
	MfaServerPrivate* mfa = (MfaServerPrivate*)context->handle;
	return mfa->ChannelEvent;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT mfa_server_check_event_handle(MfaServerContext* context)
{
	return mfa_server_read(context);
}

MfaServerContext* mfa_server_context_new(HANDLE vcm)
{
	MfaServerContext* context;
	MfaServerPrivate* mfa;
	context = (MfaServerContext*)calloc(1, sizeof(MfaServerContext));

	if (context)
	{
		context->Open = mfa_server_open;
		context->Close = mfa_server_close;
		context->Start = mfa_server_start;
		context->Stop = mfa_server_stop;
		context->GetEventHandle = mfa_server_get_event_handle;
		context->CheckEventHandle = mfa_server_check_event_handle;

		context->ServerReady = mfa_send_server_ready;
		context->ServerTokenResponse = mfa_send_server_token_response;

		/* message callbacks */
		context->ClientToken = NULL;
		context->ClientCancelled = NULL;

		mfa = context->handle = (MfaServerPrivate*)calloc(1, sizeof(MfaServerPrivate));

		if (mfa)
		{
			mfa->vcm = vcm;
			mfa->s = Stream_New(NULL, 4096);

			if (!mfa->s)
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

void mfa_server_context_free(MfaServerContext* context)
{
	MfaServerPrivate* mfa;

	if (!context)
		return;

	mfa = (MfaServerPrivate*)context->handle;

	if (mfa)
	{
		Stream_Free(mfa->s, TRUE);
	}

	free(context->handle);
	free(context);
}

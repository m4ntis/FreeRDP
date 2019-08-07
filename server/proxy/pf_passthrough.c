/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server Bkey66 channel
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

#include <freerdp/client/passthrough.h>
#include <freerdp/server/passthrough.h>

#include <winpr/synch.h>
#include <winpr/print.h>
#include <winpr/error.h>

#include "pf_passthrough.h"
#include "pf_context.h"
#include "pf_log.h"

#define TAG PROXY_TAG("passthrough")

#define NR_STATE 2

enum bkey_state {
	STATE_FIRST_SERVER_PDU,
	STATE_FIRST_CLIENT_PDU,
};

#define STATE_OK STATE_FIRST_CLIENT_PDU + 1

/* static variable holding the current verification state */
static enum bkey_state g_current_state = STATE_FIRST_SERVER_PDU;

BOOL pf_server_passthrough_init(pServerContext* ps)
{
	PassthroughServerContext* pass;
	pass = ps->pass = passthrough_server_context_new(ps->vcm, "bkey66");

	if (!pass)
	{
		return FALSE;
	}

	pass->rdpcontext = (rdpContext*)ps;
	return TRUE;
}

typedef BOOL (*state_handler)(enum bkey_state, wStream *);

/* state handlers */
static BOOL bkey_verify_first_server_pdu(enum bkey_state, wStream*);
static BOOL bkey_verify_first_client_pdu(enum bkey_state, wStream*);

static state_handler state_handlers[NR_STATE] = {
	bkey_verify_first_server_pdu,
	bkey_verify_first_client_pdu
};

static BOOL pf_bkey_check_build_info(wStream* input)
{
	UINT32 major;
	UINT32 minor;

	Stream_Read_UINT32_BE(input, major);
	Stream_Read_UINT32_BE(input, minor);

	printf("major %x minor %x\n", major, minor);
	if (major != 0x6 || minor != 0x6)
		return FALSE;

	return TRUE;
}

static BOOL bkey_verify_first_server_pdu(enum bkey_state current, wStream* input)
{
	UINT32 message_type;

	if (!Stream_EnsureRemainingCapacity(input, 148))
		return FALSE;

	if (!pf_bkey_check_build_info(input))
		return FALSE;

	Stream_SetPosition(input, 128); /* seek to message type offest (4 bytes) */
	Stream_Read_UINT32_BE(input, message_type);

	if (message_type != 0x1)
	{
		WLog_WARN(TAG, "pf_bkey_validate_first_server_pdu: msg type invalid, expected 0x1, got 0x%x", message_type);
		return FALSE;
	}

	return TRUE;
}

static BOOL bkey_verify_first_client_pdu(enum bkey_state current, wStream* input)
{
	UINT32 data_len;
	UINT32 message_type;

	if (!Stream_EnsureRemainingCapacity(input, 4))
		return FALSE;

	Stream_Read_UINT32_BE(input, data_len);

	if (!Stream_EnsureRemainingCapacity(input, data_len))
	{
		WLog_ERR(TAG, "bkey_verify_first_client_pdu: data len is invalid!");
		return FALSE;
	}

	if (!pf_bkey_check_build_info(input))
		return FALSE;

	Stream_SetPosition(input, 132); /* seek to message type offest (4 bytes) */
	Stream_Read_UINT32_BE(input, message_type);

	if (message_type != 0x2)
	{
		WLog_WARN(TAG, "pf_bkey_validate_first_server_pdu: msg type invalid, expected 0x2, got 0x%x", message_type);
		return FALSE;
	}

	return TRUE;
}

static BOOL pf_bkey_check_current_state(wStream* input)
{
	WLog_INFO(TAG, "pf_bkey_check_current_state: current state: %d\n", g_current_state);
	if (g_current_state == STATE_OK)
		return TRUE;
	
	state_handler handler = state_handlers[g_current_state];
	if (!handler(g_current_state, input))
		return FALSE;

	/* proceed to next state */
	g_current_state++;
	return TRUE;
}

static UINT pf_passthrough_data_received_from_client(PassthroughServerContext* context, const BYTE* data, UINT32 len)
{
	wStream s;
	proxyData* pdata = (proxyData*) context->custom;
	PassthroughClientContext* client = (PassthroughClientContext*) pdata->pc->pass;

	Stream_StaticInit(&s, (BYTE*) data, len);

	if (!pf_bkey_check_current_state(&s))
	{
		WLog_WARN(TAG, "bkey_recv_from_client: pf_bkey_check_current_state failed!");
		return ERROR_INTERNAL_ERROR;
	}

	WLog_INFO(TAG, "received data from client, len: %d", len);
	winpr_HexDump(TAG, WLOG_INFO, data, len);
	
	client->SendData(client, data, len);

	/*
	 * client write is async, wait for write finish before exit,
	 * otherwise BYTE* data will be freed/overriden!
	 */
	WaitForSingleObject(client->write_complete, INFINITE);
	return CHANNEL_RC_OK;
}

static UINT pf_passthrough_data_received_from_server(PassthroughClientContext* context, const BYTE* data, UINT32 len)
{
	wStream s;
	proxyData* pdata = (proxyData*) context->custom;
	PassthroughServerContext* server = (PassthroughServerContext*) pdata->ps->pass;

	Stream_StaticInit(&s, (BYTE*) data, len);

	if (!pf_bkey_check_current_state(&s))
	{
		WLog_WARN(TAG, "bkey_recv_from_server: pf_bkey_check_current_state failed!");
		return ERROR_INTERNAL_ERROR;
	}

	WLog_INFO(TAG, "received data from server, len: %d", len);
	winpr_HexDump(TAG, WLOG_INFO, data, len);
	server->SendData(server, data, len);
	return CHANNEL_RC_OK;

}

void pf_passthrough_pipeline_init(PassthroughClientContext* client, PassthroughServerContext* server,
                             proxyData* pdata)
{
	/* Set server and client side references to proxy data */
	client->custom = (void*) pdata;
	server->custom = (void*) pdata;

	/* Set server callbacks */
	server->DataReceived = pf_passthrough_data_received_from_client;
	client->DataReceived = pf_passthrough_data_received_from_server;
}
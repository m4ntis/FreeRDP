/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 * MFA implementation
 *
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

#include <freerdp/server/mfa.h>

#include <winpr/synch.h>

#include "pf_mfa.h"
#include "pf_log.h"
#include "token_validator.h"

#define TAG PROXY_TAG("mfa")

/* forward declerations */
static void pf_mfa_register_callbacks(MfaServerContext* context, proxyData* pdata);

struct mfa_context* pf_mfa_context_new()
{
	struct mfa_context* ctx;

	ctx = calloc(1, sizeof(struct mfa_context));
	if (!ctx)
		return NULL;

	ctx->status = MFA_STATUS_UNINITIALIZED;

	if (!InitializeCriticalSectionEx(&(ctx->lock), 0, 0))
		goto error;

	if (!(ctx->auth_status = CreateEvent(NULL, TRUE, FALSE, NULL)))
		goto error;

	if (!(ctx->token_expired = CreateEvent(NULL, TRUE, FALSE, NULL)))
		goto error;

	return ctx;
error:
	if (&ctx->lock)
		DeleteCriticalSection(&ctx->lock);
	if (ctx->auth_status)
		CloseHandle(ctx->auth_status);
	if (ctx->token_expired)
		CloseHandle(ctx->token_expired);
	free(ctx);
	return NULL;
}

void pf_mfa_context_free(struct mfa_context* ctx)
{
	if (ctx->auth_status)
	{
		CloseHandle(ctx->auth_status);
		ctx->auth_status = NULL;
	}

	if (ctx->token_expired)
	{
		CloseHandle(ctx->token_expired);
		ctx->token_expired = NULL;
	}

	if (ctx->exp_thread)
	{
		CloseHandle(ctx->exp_thread);
		ctx->exp_thread = NULL;
	}

	DeleteCriticalSection(&ctx->lock);
}

void pf_mfa_mark_token_as_expired(struct mfa_context* ctx)
{
	SetEvent(ctx->token_expired);
}

void pf_mfa_wait_for_token_expired_thread(struct mfa_context* ctx)
{
	MFA_STATUS status;
	pf_mfa_get_status(ctx, &status);

	if (ctx == NULL || status != MFA_STATUS_AUTH_SUCCESS)
	{
		/* exp_thread is not set, client probably didn't provide a token */
		WLog_DBG(TAG, "pf_mfa_wait_for_token_expired_thread(): mfa not initialized, no need to wait");
		return;
	}

	/* wait for expired thread to finish */
	WLog_DBG(TAG, "waiting for exp_thread to exit");
	if (WaitForSingleObject(ctx->exp_thread, INFINITE) != WAIT_OBJECT_0)
	{
		WLog_ERR(TAG, "pf_server_wait_for_mfa_to_finish(): WaitForSingleObject failed!");
		return;
	}

	WLog_DBG(TAG, "exp_thread exited");
}

void pf_mfa_set_status(struct mfa_context* ctx, MFA_STATUS status)
{
	EnterCriticalSection(&ctx->lock);
	ctx->status = status;
	LeaveCriticalSection(&ctx->lock);

	/* signal other threads that mfa status has changed */
	SetEvent(ctx->auth_status);
}

void pf_mfa_get_status(struct mfa_context* ctx, MFA_STATUS* status)
{
	EnterCriticalSection(&ctx->lock);
	*status = ctx->status;
	LeaveCriticalSection(&ctx->lock);
}

BOOL pf_mfa_init(rdpContext* context)
{
	pServerContext* ps = (pServerContext*)context;
	if (ps->mfa != NULL)
	{
		WLog_WARN(TAG, "pf_mfa_init: ps->mfa already initialized");
		return TRUE;
	}

	/* client did not connect with MFA channel */
	if (!WTSVirtualChannelManagerIsChannelJoined(ps->vcm, MFA_SVC_CHANNEL_NAME))
	{
		WLog_WARN(TAG, "pf_mfa_init: client did not connect with MFA channel");
		return FALSE;
	}

	MfaServerContext* mfa;
	mfa = ps->mfa = mfa_server_context_new(ps->vcm);

	if (!mfa)
	{
		return FALSE;
	}

	mfa->rdpcontext = (rdpContext*)ps;
	pf_mfa_register_callbacks(ps->mfa, ps->pdata);

	return mfa->Start(ps->mfa) == CHANNEL_RC_OK;
}

static UINT pf_mfa_client_token(MfaServerContext* context, const MFA_CLIENT_TOKEN* ct)
{
	WLog_INFO(TAG, "pf_mfa_client_token!");
	proxyData* pdata = context->custom;
	proxyConfig* config = pdata->config;

	if (token_validator_validate_token(config->tv, pdata, (const char*)ct->TokenData,
	                                   ct->cbTokenLen))
	{
		WLog_INFO(TAG, "pf_mfa_client_token: got valid token");
		pf_mfa_set_status(pdata->mfa, MFA_STATUS_AUTH_SUCCESS);
		return context->ServerTokenResponse(context, MFA_FLAG_OK);
	}
	else
	{
		WLog_INFO(TAG, "pf_mfa_client_token: got invalid token");
		pf_mfa_set_status(pdata->mfa, MFA_STATUS_AUTH_FAIL);
		return context->ServerTokenResponse(context, MFA_FLAG_FAIL);
	}
}

static UINT pf_mfa_client_cancelled(MfaServerContext* context)
{
	WLog_INFO(TAG, "pf_mfa_client_cancelled!");
	proxyData* pdata = context->custom;
	pf_mfa_set_status(pdata->mfa, MFA_STATUS_AUTH_FAIL);

	return CHANNEL_RC_OK;
}

static void pf_mfa_register_callbacks(MfaServerContext* context, proxyData* pdata)
{
	context->custom = pdata;

	context->ClientToken = pf_mfa_client_token;
	context->ClientCancelled = pf_mfa_client_cancelled;
}

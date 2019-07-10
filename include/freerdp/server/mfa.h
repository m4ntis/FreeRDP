/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * MFA Virtual Channel Server Interface
 *
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

#ifndef FREERDP_CHANNEL_MFA_SERVER_MFA_H
#define FREERDP_CHANNEL_MFA_SERVER_MFA_H

#include <freerdp/api.h>
#include <freerdp/types.h>
#include <freerdp/channels/wtsvc.h>

#include <freerdp/channels/mfa.h>

/**
 * Server Interface
 */

typedef struct _mfa_server_context MfaServerContext;

typedef UINT (*psMfaOpen)(MfaServerContext* context);
typedef UINT (*psMfaClose)(MfaServerContext* context);
typedef UINT (*psMfaStart)(MfaServerContext* context);
typedef UINT (*psMfaStop)(MfaServerContext* context);
typedef HANDLE (*psMfaGetEventHandle)(MfaServerContext* context);
typedef UINT (*psMfaCheckEventHandle)(MfaServerContext* context);

typedef UINT (*psMfaServerReady)(MfaServerContext* context, const MFA_SERVER_READY* sr);
typedef UINT (*psMfaClientToken)(MfaServerContext* context, const MFA_CLIENT_TOKEN* ct);
typedef UINT (*psMfaClientCancelled)(MfaServerContext* context);
typedef UINT (*psMfaServerTokenResponse)(MfaServerContext* context, const enum MFA_FLAGS flags);

struct _mfa_server_context
{
	void* handle;
	void* custom;

	psMfaOpen Open;
	psMfaClose Close;
	psMfaStart Start;
	psMfaStop Stop;
	psMfaGetEventHandle GetEventHandle;
	psMfaCheckEventHandle CheckEventHandle;

	psMfaServerReady ServerReady;
	psMfaClientToken ClientToken;
	psMfaClientCancelled ClientCancelled;
	psMfaServerTokenResponse ServerTokenResponse;

	rdpContext* rdpcontext;
};

#ifdef __cplusplus
extern "C"
{
#endif

	FREERDP_API MfaServerContext* mfa_server_context_new(HANDLE vcm);
	FREERDP_API void mfa_server_context_free(MfaServerContext* context);

#ifdef __cplusplus
}
#endif

#endif /* FREERDP_CHANNEL_MFA_SERVER_MFA_H */

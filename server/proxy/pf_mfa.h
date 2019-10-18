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

#ifndef FREERDP_SERVER_PROXY_PFMFA_H
#define FREERDP_SERVER_PROXY_PFMFA_H

#include <freerdp/server/mfa.h>

typedef enum mfa_status MFA_STATUS;

enum mfa_status
{
	MFA_STATUS_UNINITIALIZED = 0,
	MFA_STATUS_AUTH_SUCCESS,
	MFA_STATUS_AUTH_FAIL,
};

/* used to store information about the MFA status of a session */
struct mfa_context
{
	CRITICAL_SECTION lock; /* used to protect `status` */
	MFA_STATUS status;     /* current status of MFA authentication */
	HANDLE auth_status;    /* used by proxy's client to wait for MFA auth status */
	HANDLE token_expired;  /* use by server to close the connection when token expires */
	HANDLE exp_thread;     /* handle to a thread that manages token expiration */
};

BOOL pf_mfa_init(rdpContext* context);

struct mfa_context* pf_mfa_context_new();
void pf_mfa_context_free(struct mfa_context* ctx);

void pf_mfa_set_status(struct mfa_context* ctx, MFA_STATUS status);
void pf_mfa_get_status(struct mfa_context* ctx, MFA_STATUS* status);
void pf_mfa_mark_token_as_expired(struct mfa_context* ctx);
void pf_mfa_wait_for_token_expired_thread(struct mfa_context* ctx);

#endif /* FREERDP_SERVER_PROXY_PFMFA_H */

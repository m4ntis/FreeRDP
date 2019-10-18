/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * JWT Token validation
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

#ifndef FREERDP_SERVER_PROXY_TOKEN_VALIDATOR_H
#define FREERDP_SERVER_PROXY_TOKEN_VALIDATOR_H

#include <cjose/cjose.h>
#include <winpr/collections.h>

#include "pf_context.h"

typedef struct token_validator TokenValidator;

#define MAX_JWK_ARRAY_SZ 5
struct token_validator
{
	cjose_jwk_t* jwk_array[MAX_JWK_ARRAY_SZ];
	char* issuer;
	char* jwks_uri;
	char* audience;
	BOOL ssl_verify_peer;
	INT64 token_skew_minutes;
	wHashTable* used_tokens;
};

TokenValidator* token_validator_init(const char* adfs_base_url, const char* app_audience,
									 INT64 token_skew_minutes, BOOL insecure_ssl);
void token_validator_free(TokenValidator* tv);
BOOL token_validator_validate_token(TokenValidator* tv, proxyData* pdata, const char* compact_token,
                                    const size_t compact_token_len);

#endif /* FREERDP_SERVER_PROXY_TOKEN_VALIDATOR_H */

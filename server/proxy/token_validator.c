/* FreeRDP: A Remote Desktop Protocol Implementation
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

#include <jansson.h>
#include <curl/curl.h>

#include <winpr/string.h>
#include <winpr/sysinfo.h>
#include <winpr/synch.h>
#include <winpr/thread.h>

#include "pf_log.h"
#include "pf_mfa.h"
#include "token_validator.h"

#define TAG PROXY_TAG("token_validator")

#define TOKEN_CLAIM_ISSUER "iss"
#define TOKEN_CLAIM_AUDIENCE "aud"
#define TOKEN_CLAIM_TOKEN_CREATION "iat"
#define TOKEN_CLAIM_EXP "exp"
#define TOKEN_CLAIM_NONCE "nonce"

#define OPENID_CONFIG_ISSUER "issuer"
#define OPENID_CONFIG_JWKS_URI "jwks_uri"

typedef struct token_context TokenContext;

struct token_context
{
	HANDLE timer;            /* timer handle, used for token expiration handling */

	wHashTable* used_tokens; /* used to check if the token is already used */
	const char* nonce;       /* used to check if the token is already used */
	proxyData* pdata;  /* pointer to the RDP session */
};

static void token_validator_token_context_free(TokenContext* tc)
{
	if (tc == NULL)
		return;

	if (tc->timer)
	{
		CloseHandle(tc->timer);
		tc->timer = NULL;
	}

	free(tc);
}

static BOOL curl_do_http_get_request(const char* url, void* response_callback, void* callback_param,
                                     BOOL ssl_verify_peer)
{
	CURL* curl;
	CURLcode res;

	curl = curl_easy_init();
	if (curl == NULL)
	{
		WLog_ERR(TAG, "curl_do_http_get_request(): curl_easy_init failed!");
		return FALSE;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);

	if (!ssl_verify_peer)
	{
		WLog_WARN(TAG, "curl_do_http_get_request: SSL certificate validation disabled!");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	}

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	if (response_callback)
	{
		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_callback);
	}

	if (callback_param)
	{
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, callback_param);
	}

	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "MFA-channel/1.0");

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* Check for errors */
	if (res != CURLE_OK)
	{
		WLog_ERR(TAG, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		return FALSE;
	}

	/* always cleanup */
	curl_easy_cleanup(curl);
	return TRUE;
}

static size_t jwk_response(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	cjose_jwk_t** jwk_array = userp;

	WLog_DBG(TAG, "jwk_response: got JWK response.");

	json_error_t json_err;
	json_t* json = json_loadb(contents, realsize, 0, &json_err);
	if (json == NULL)
	{
		WLog_ERR(TAG, "jwk_response(): json_loadb failed!");
		return 0;
	}

	json_t* keys_array = json_object_get(json, "keys");
	if (keys_array == NULL)
	{
		WLog_ERR(TAG, "keys_array array not found");
		return 0;
	}

	if (!json_is_array(keys_array))
	{
		WLog_ERR(TAG, "keys_array is not an array object");
		return 0;
	}

	size_t array_size = json_array_size(keys_array);
	if (array_size > MAX_JWK_ARRAY_SZ)
	{
		WLog_ERR(TAG, "array_size > %d", MAX_JWK_ARRAY_SZ);
		return 0;
	}

	if (jwk_array == NULL)
	{
		WLog_ERR(TAG, "jwk_array is null");
		return 0;
	}

	size_t index;
	json_t* value;
	json_array_foreach(keys_array, index, value)
	{
		json_t* use_obj = json_object_get(value, "use");
		if (!json_is_string(use_obj))
		{
			WLog_ERR(TAG, "jwk_response: claim 'use' not found in jwk (index %d). skipping.",
			         index);
			continue;
		}

		/* check that the JWK usage is for signature validation */
		if (0 != strcmp(json_string_value(use_obj), "sig"))
		{
			WLog_WARN(TAG, "jwk_response: jwk's use != 'sig' (index %d). skipping.", index);
			continue;
		}

		char* key = json_dumps(value, 0);

		/* parse jwk */
		cjose_err err;
		jwk_array[index] = cjose_jwk_import(key, strlen(key), &err);
		if (NULL == jwk_array[index])
		{
			WLog_ERR(TAG, "cjose_jwk_import failed: code %d : %s", err.code, err.message);
			return -1;
		}

		free(key);
	}

	/* decrementing the refcount to zero should free the object */
	json_decref(json);

	return realsize;
}

static BOOL fetch_jwks(TokenValidator* tv)
{
	return curl_do_http_get_request(tv->jwks_uri, jwk_response, tv->jwk_array, tv->ssl_verify_peer);
}

static size_t parse_openid_configuration(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	TokenValidator* tv = userp;

	WLog_DBG(TAG, "parse_openid_configuration: got response...");

	json_error_t json_err;
	json_t* json = json_loadb(contents, realsize, 0, &json_err);
	if (NULL == json)
	{
		WLog_ERR(TAG, "parse_openid_configuration(): json_loadb failed!");
		return 0;
	}

	/* get issuer */
	json_t* issuer_obj = json_object_get(json, OPENID_CONFIG_ISSUER);
	if (!json_is_string(issuer_obj))
	{
		WLog_ERR(TAG, "issuer_obj is not string");
		return 0;
	}
	tv->issuer = _strdup(json_string_value(issuer_obj));
	WLog_INFO(TAG, "parse_openid_configuration: issuer %s", tv->issuer);

	/* get jwks_uri */
	json_t* jwks_uri_obj = json_object_get(json, OPENID_CONFIG_JWKS_URI);
	if (!json_is_string(jwks_uri_obj))
	{
		WLog_ERR(TAG, "jwks_uri_obj is not string");
		return 0;
	}
	tv->jwks_uri = _strdup(json_string_value(jwks_uri_obj));
	WLog_INFO(TAG, "parse_openid_configuration: jwks_uri %s", tv->jwks_uri);

	/* free json */
	json_decref(json);
	return realsize;
}

static BOOL fetch_openid_configuration(TokenValidator* tv, const char* adfs_base_url)
{

	const char* openid_configuration_suffix = "/.well-known/openid-configuration";
	char* url = malloc(strlen(openid_configuration_suffix) + strlen(adfs_base_url) + 1);
	if (url == NULL)
	{
		WLog_ERR(TAG, "get_openid_configuration: malloc failed");
		return FALSE;
	}
	strcpy(url, adfs_base_url);
	strcat(url, openid_configuration_suffix);

	if (!curl_do_http_get_request(url, parse_openid_configuration, tv, tv->ssl_verify_peer))
	{
		WLog_ERR(TAG, "curl_do_http_get_request failed.");
		free(url);
		return FALSE;
	}

	free(url);
	return TRUE;
}

TokenValidator* token_validator_init(const char* adfs_base_url, const char* app_audience,
                                     INT64 token_skew_minutes, BOOL insecure_ssl)
{
	if (NULL == adfs_base_url || NULL == app_audience)
	{
		WLog_ERR(TAG, "token_validator_init: bad parameters");
		return NULL;
	}

	TokenValidator* tv = calloc(1, sizeof(TokenValidator));
	if (NULL == tv)
	{
		WLog_ERR(TAG, "token_validator_init: calloc failed!");
		return NULL;
	}

	tv->ssl_verify_peer = !insecure_ssl;

	if (!fetch_openid_configuration(tv, adfs_base_url))
	{
		WLog_ERR(TAG, "fetch_openid_configuration failed");
		free(tv);
		return NULL;
	}

	if (!fetch_jwks(tv))
	{
		WLog_ERR(TAG, "fetch_jwks failed");
		free(tv);
		return NULL;
	}

	/* create thread-safe hash table */
	tv->used_tokens = HashTable_New(TRUE);
	if (!tv->used_tokens)
	{
		WLog_ERR(TAG, "HashTable_New failed!");
		free(tv);
		return NULL;
	}

	/* used_tokens is a hash table whose keys are strings.
	 * therefore, those callbacks must be set
	 */
	tv->used_tokens->hash = HashTable_StringHash;
	tv->used_tokens->keyCompare = HashTable_StringCompare;
	tv->used_tokens->keyClone = HashTable_StringClone;
	tv->used_tokens->keyFree = HashTable_StringFree;

	tv->audience = _strdup(app_audience);
	tv->token_skew_minutes = token_skew_minutes;
	return tv;
}

void token_validator_free(TokenValidator* tv)
{
	if (NULL == tv)
		return;

	/* hash table (used tokens) */
	HashTable_Free(tv->used_tokens);

	/* free jwks_uri */
	free(tv->jwks_uri);

	/* free issuer */
	free(tv->issuer);

	/* free jwk tokens */
	for (int i = 0; i < MAX_JWK_ARRAY_SZ; i++)
	{
		cjose_jwk_release(tv->jwk_array[i]);
		tv->jwk_array[i] = NULL;
	}

	/* free audience */
	free(tv->audience);

	free(tv);
}

static BOOL validate_token_claim(json_t* token, const char* claim_name,
                                 const char* expected_claim_value)
{
	json_t* claim_obj = json_object_get(token, claim_name);
	if (NULL == claim_obj)
	{
		WLog_ERR(TAG, "claim %s not found in token", claim_name);
		return FALSE;
	}

	if (0 != strcmp(json_string_value(claim_obj), expected_claim_value))
	{
		WLog_ERR(TAG, "invalid claim value for: %s. got: %s, expected: %s", claim_name,
		         json_string_value(claim_obj), expected_claim_value);
		return FALSE;
	}

	/* valid claim! */
	return TRUE;
}

// borrowed from `https://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/`
static INT64 FileTime_to_POSIX(LPFILETIME ft)
{
	// takes the last modified date
	LARGE_INTEGER date, adjust;
	date.HighPart = ft->dwHighDateTime;
	date.LowPart = ft->dwLowDateTime;

	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;

	// removes the diff between 1970 and 1601
	date.QuadPart -= adjust.QuadPart;

	// converts back from 100-nanoseconds to seconds
	return date.QuadPart / 10000000;
}

/* Read on msdn about FILETIME and LARGE_INTEGER structs. */
HANDLE token_validator_create_timer(INT64 token_exp)
{
	HANDLE timer;
	INT64 now, diff;
	LARGE_INTEGER due;
	FILETIME fileTime;

	GetSystemTimeAsFileTime(&fileTime);
	now = FileTime_to_POSIX(&fileTime);
	diff = token_exp - now;
	WLog_INFO(TAG, "token_validator_create_timer(): now: %ld, exp: %ld, diff: %ld sec", now,
	          token_exp, diff);

	if (now >= token_exp)
	{
		WLog_ERR(TAG, "token_validator_create_timer(): now >= token_exp");
		return NULL;
	}

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == timer)
	{
		WLog_ERR(TAG, "token_validator_create_timer(): CreateWaitableTimer failed");
		return NULL;
	}

	due.QuadPart = -10000000LL * diff; /* WinApi is the best! */
	if (!SetWaitableTimer(timer, &due, 0, NULL, NULL, 0))
	{
		CloseHandle(timer);
		return NULL;
	}

	return timer;
}

DWORD WINAPI token_validator_exp_thread(LPVOID arg)
{
	DWORD status;
	TokenContext* ctx = arg;
	proxyData* pdata = ctx->pdata;
	HANDLE timer = ctx->timer;
	HANDLE handles[2] = {timer, pdata->abort_event};

	status = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
	WLog_DBG(TAG, "token_validator_exp_thread: WaitForSingleObject returned %d\n", status);

	if (status == WAIT_FAILED)
	{
		// TODO: how to handle this error?
		// if we just ignore it, the token will never expire
		// TODO: check when WAIT_FAILED occures
	}

	if (WaitForSingleObject(pdata->abort_event, 0) == WAIT_OBJECT_0)
	{
		WLog_INFO(TAG, "connection closed, exiting token expired thread");
		goto out;
	}

	else if (status == WAIT_OBJECT_0)
	{
		WLog_INFO(TAG, "token expired");
		pf_mfa_mark_token_as_expired(pdata->mfa);
	}

out:
	/* remove nonce from hash table */
	HashTable_Remove(ctx->used_tokens, (void*)ctx->nonce);

	/* free token context */
	token_validator_token_context_free(ctx);
	ExitThread(0);
	return 0;
}

static BOOL token_validator_verify_signature(TokenValidator* tv, cjose_jws_t* token)
{
	cjose_err err;

	for (int i = 0; i < MAX_JWK_ARRAY_SZ; i++)
	{
		WLog_DBG(TAG, "token_validator_verify_signature(): using key at index %d", i);
		if (tv->jwk_array[i] == NULL)
		{
			/* no more keys */
			return FALSE;
		}

		if (cjose_jws_verify(token, tv->jwk_array[i], &err))
		{
			/* signature verified */
			return TRUE;
		}
		else
		{
			WLog_ERR(TAG,
			         "token_validator_verify_signature(): cjose_jws_verify failed: code %d: %s",
			         err.code, err.message);
			return FALSE;
		}
	}

	return FALSE;
}

static BOOL validate_token_skew(INT64 token_creation_time, INT64 token_skew_minutes)
{
	INT64 now;
	FILETIME fileTime;

	GetSystemTimeAsFileTime(&fileTime);
	now = FileTime_to_POSIX(&fileTime);

	if ((now - token_creation_time) > (1000 * 60 * token_skew_minutes))
	{
		WLog_WARN(TAG, "token_validator_validate_token_skew(): token is too old!");
		return FALSE;
	}

	return TRUE;
}

static BOOL validate_token_nonce(TokenValidator* tv, const char* nonce)
{
	if (tv == NULL)
	{
		WLog_ERR(TAG, "validate_token_nonce: tv == NULL");
		return FALSE;
	}

	if (HashTable_ContainsKey(tv->used_tokens, (void*)nonce))
	{
		/* token already used! */
		WLog_WARN(TAG, "validate_token_nonce(): token already used! nonce: %s", nonce);
		return FALSE;
	}

	/* set the value to 1. do not use NULL or 0, 
	 * it won't work because the implementation of HashTable_ContainsKey
	 */
	if (HashTable_Add(tv->used_tokens, (void*)nonce, (void*)1) < 0)
	{
		WLog_ERR(TAG, "validate_token_nonce(): failed to add nonce %s to tokens_used!", nonce);
		return FALSE;
	}

	return TRUE;
}

BOOL token_validator_validate_token(TokenValidator* tv, proxyData* pdata,
                                    const char* compact_token, const size_t compact_token_len)
{
	bool ret = FALSE;
	cjose_jws_t* token = NULL;
	json_t* plain_token;
	json_t* temp;
	cjose_err err;
	char* plain;
	size_t plain_len;

	/* used for token validation */
	TokenContext* ctx;
	HANDLE timer;
	INT64 token_exp;
	INT64 token_created;
	const char* nonce;

	if (tv == NULL)
	{
		WLog_ERR(TAG, "token_validator_validate_token: tv == NULL");
		return ret;
	}

	WLog_DBG(TAG, "loading the jws token");
	token = cjose_jws_import(compact_token, compact_token_len, &err);
	if (NULL == token)
	{
		WLog_ERR(TAG, "cjose_jws_import failed: code %d : %s", err.code, err.message);
		goto error;
	}

	/* signature check */
	WLog_DBG(TAG, "verifing signature");
	if (!token_validator_verify_signature(tv, token))
	{
		WLog_WARN(TAG, "invalid token signature.");
		goto error;
	}

	/* validate specefic token claims */
	if (!cjose_jws_get_plaintext(token, (void*)&plain, &plain_len, &err))
	{
		WLog_ERR(TAG, "cjose_jws_get_plaintext failed: code %d : %s", err.code, err.message);
		goto error;
	}

	WLog_INFO(TAG, "token plain: %s", plain);
	plain_token = json_loadb(plain, plain_len, 0, NULL);
	if (NULL == plain_token)
	{
		WLog_ERR(TAG, "json_loadb failed!");
		goto error;
	}

	/* nonce */
	temp = json_object_get(plain_token, TOKEN_CLAIM_NONCE);
	nonce = json_string_value(temp);
	if (!validate_token_nonce(tv, nonce))
		goto error;

	/* clock skew */
	temp = json_object_get(plain_token, TOKEN_CLAIM_TOKEN_CREATION);
	token_created = json_number_value(temp);
	if (!validate_token_skew(token_created, tv->token_skew_minutes))
		goto error;

	/* audience */
	if (!validate_token_claim(plain_token, TOKEN_CLAIM_AUDIENCE, tv->audience))
		goto error;

	/* issuer */
	if (!validate_token_claim(plain_token, TOKEN_CLAIM_ISSUER, tv->issuer))
		goto error;

	/* future expiration check */
	temp = json_object_get(plain_token, TOKEN_CLAIM_EXP);
	token_exp = json_number_value(temp);

	timer = token_validator_create_timer(token_exp);
	if (timer == NULL)
	{
		WLog_ERR(TAG, "token_validator_create_timer failed!");
		goto error;
	}

	/* ctx must be free()'ed by `token_validator_exp_thread`. */
	ctx = malloc(sizeof(TokenContext));
	if (ctx == NULL)
	{
		WLog_ERR(TAG, "token_validator_validate_token(): malloc TokenContext failed!");
		goto error;
	}

	ctx->pdata = pdata;
	ctx->timer = timer;
	ctx->nonce = nonce;
	ctx->used_tokens = tv->used_tokens;

	/* Start a proxy's client in it's own thread */
	if (!(pdata->mfa->exp_thread = CreateThread(NULL, 0, token_validator_exp_thread, ctx, 0, NULL)))
	{
		WLog_ERR(TAG, "CreateThread failed!");
		token_validator_token_context_free(ctx);
		goto error;
	}

	json_decref(plain_token);
	ret = TRUE;
error:
	cjose_jws_release(token);
	return ret;
}
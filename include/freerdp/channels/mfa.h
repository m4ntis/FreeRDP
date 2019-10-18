/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * MFA Virtual Channel Extension
 *
 * Copyright 2019 Idan Freiberg <speidy@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FREERDP_CHANNEL_MFA_H
#define FREERDP_CHANNEL_MFA_H

#include <freerdp/api.h>
#include <freerdp/types.h>

#include <winpr/shell.h>

#define MFA_SVC_CHANNEL_NAME "mfa"

/* MFA_HEADER.msgType */
#define CB_SERVER_READY 0x0001
#define CB_CLIENT_TOKEN 0x0002
#define CB_CLIENT_CANCELLED 0x0003
#define CB_SERVER_TOKEN_RESPONSE 0x0004

enum MFA_FLAGS {
	MFA_FLAG_UNUSED = 0,
	MFA_FLAG_TIMEOUT,
	MFA_FLAG_FAIL,
	MFA_FLAG_TOKEN_EXPIRED,
	MFA_FLAG_OK,
};

/* MFA Messages */

#define DEFINE_MFA_HEADER_COMMON() \
	UINT16 msgType;                \
	UINT16 msgFlags;               \
	UINT32 dataLen;

struct _MFA_HEADER
{
	DEFINE_MFA_HEADER_COMMON();
};
typedef struct _MFA_HEADER MFA_HEADER;

#define MFA_VERSION_1 (0x00000001)
struct _MFA_SERVER_READY
{
	DEFINE_MFA_HEADER_COMMON();

	UINT32 version;
};
typedef struct _MFA_SERVER_READY MFA_SERVER_READY;

struct _MFA_CLIENT_TOKEN
{
	DEFINE_MFA_HEADER_COMMON();

	UINT32 cbTokenLen;
	BYTE* TokenData;
};
typedef struct _MFA_CLIENT_TOKEN MFA_CLIENT_TOKEN;


struct _MFA_SERVER_TOKEN_RESPONSE
{
	DEFINE_MFA_HEADER_COMMON();
};
typedef struct _MFA_SERVER_TOKEN_RESPONSE MFA_SERVER_TOKEN_RESPONSE;

struct _MFA_CLIENT_CANCELLED
{
	DEFINE_MFA_HEADER_COMMON();
};
typedef struct _MFA_CLIENT_CANCELLED MFA_CLIENT_CANCELLED;

#endif /* FREERDP_CHANNEL_MFA_H */

/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Graphics Pipeline Extension
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
#include <winpr/stream.h>
#include <freerdp/channels/log.h>

#define TAG CHANNELS_TAG("cliprdr.common")

#include "cliprdr_common.h"

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT cliprdr_read_header(wStream* s, CLIPRDR_HEADER* header)
{
	if (Stream_GetRemainingLength(s) < 8)
	{
		WLog_ERR(TAG, "calloc failed!");
		return CHANNEL_RC_NO_MEMORY;
	}

	Stream_Read_UINT16(s, header->msgType); /* msgType (2 bytes) */
	Stream_Read_UINT16(s, header->msgFlags); /* msgFlags (2 bytes) */
	Stream_Read_UINT32(s, header->dataLen); /* dataLen (4 bytes) */
	return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
UINT cliprdr_write_header(wStream* s, const CLIPRDR_HEADER* header)
{
	Stream_Write_UINT16(s, header->msgType);
	Stream_Write_UINT16(s, header->msgFlags);
	/* Write actual length after the entire packet has been constructed. */
	Stream_Seek(s, 4);
	return CHANNEL_RC_OK;
}

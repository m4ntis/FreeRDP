/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 *
 * Copyright 2019 Mati Shabtay <matishabtay@gmail.com>
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

#include <stdio.h>
#include <string.h>
#include <winpr/crt.h>
#include "pf_config.h"

#define CHANNELS_SEPERATOR ","

char** parse_channels_from_str(const char* str, UINT32* length)
{
	char* s = strdup(str);
	int tokens_alloc = 1;
	int tokens_count = 0;
	char** tokens = calloc(tokens_alloc, sizeof(char*));
	char* token;

	while ((token = strsep(&s, CHANNELS_SEPERATOR)) != NULL)
	{
		if (tokens_count == tokens_alloc)
		{
			tokens_alloc *= 2;
			tokens = realloc(tokens, tokens_alloc * sizeof(char*));
		}

		tokens[tokens_count++] = strdup(token);
	}

	if (tokens_count == 0)
	{
		free(tokens);
		tokens = NULL;
	}
	else
	{
		tokens = realloc(tokens, tokens_count * sizeof(char*));
	}

	*length = tokens_count;
	free(s);
	return tokens;
}

BOOL pf_server_load_config(char* path, proxyConfig* config)
{
	const char* input;
	BOOL result = TRUE;
	wIniFile* ini;
	ini = IniFile_New();

	if (IniFile_ReadFile(ini, path) < 0)
	{
		result = FALSE;
		goto out;
	}

	/* Server */
	config->Host = _strdup(IniFile_GetKeyValueString(ini, "Server", "Host"));
	config->LocalOnly = IniFile_GetKeyValueInt(ini, "Server", "LocalOnly");
	config->Port = IniFile_GetKeyValueInt(ini, "Server", "Port");
	/* graphics */
	config->GFX = IniFile_GetKeyValueInt(ini, "Graphics", "GFX");
	config->BitmapUpdate = IniFile_GetKeyValueInt(ini, "Graphics", "BitmapUpdate");
	/* input */
	config->Keyboard = IniFile_GetKeyValueInt(ini, "Input", "Keyboard");
	config->Mouse = IniFile_GetKeyValueInt(ini, "Input", "Mouse");
	/* security */
	config->TlsSupport = IniFile_GetKeyValueInt(ini, "Security", "TlsSupport");
	config->NlaSupport = IniFile_GetKeyValueInt(ini, "Security", "NlaSupport");
	config->RdpSupport = IniFile_GetKeyValueInt(ini, "Security", "RdpSupport");
	/* channels filtering */
	input = IniFile_GetKeyValueString(ini, "Security", "AllowedChannels");

	if (input)
	{
		config->AllowedChannels = parse_channels_from_str(input, &config->AllowedChannelsCount);

		if (config->AllowedChannels == NULL)
		{
			result = FALSE;
			goto out;
		}
	}

	input = IniFile_GetKeyValueString(ini, "Security", "DeniedChannels");

	if (input)
	{
		config->DeniedChannels = parse_channels_from_str(input, &config->DeniedChannelsCount);

		if (config->DeniedChannels == NULL)
		{
			result = FALSE;
			goto out;
		}
	}

out:
	IniFile_Free(ini);
	return result;
}

void pf_server_config_free(proxyConfig* config)
{
	for (int i = 0; i < config->AllowedChannelsCount; i++)
		free(config->AllowedChannels[i]);

	for (int i = 0; i < config->DeniedChannelsCount; i++)
		free(config->DeniedChannels[i]);

	free(config->AllowedChannels);
	free(config->DeniedChannels);
	free(config->Host);
	free(config);
}
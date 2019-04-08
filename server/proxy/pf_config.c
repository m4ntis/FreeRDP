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

BOOL pf_server_load_config(char* path, proxyConfig* config)
{
	wIniFile* ini = IniFile_New();
	if (IniFile_ReadFile(ini, path) < 0)
	{
		IniFile_Free(ini);
		return FALSE;
	}

	/* general */
	config->Host = _strdup(IniFile_GetKeyValueString(ini, "General", "Host"));
	config->LocalOnly = IniFile_GetKeyValueInt(ini, "General", "LocalOnly");
	config->Port = IniFile_GetKeyValueInt(ini, "General", "Port");
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

	IniFile_Free(ini);
	return TRUE;
}

void pf_server_config_free(proxyConfig* config)
{
	free(config->Host);
	free(config);
}
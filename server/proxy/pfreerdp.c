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

#include "pf_server.h"
#include "proxy.h"
#include "pf_config.h"
#include "pf_log.h"

int main(int argc, char* argv[])
{
	int status = 0;
	rdpProxyServer* server;
	proxyConfig* config;
	server = proxy_server_new();

	if (server == NULL)
	{
		WLog_ERR(TAG, "Server instance allocation failed");
		return -1;
	}

	config = server->config;

	if (!pf_server_load_config("config.ini", config))
	{
		WLog_ERR(TAG, "An error occured while parsing configuration file");
		status = -1;
		goto fail;
	}

	if (config->Mode == PROXY_CHANNELS_MODE_BLACKLIST)
	{
		WLog_INFO(TAG, "Channels mode: BLACKLIST");

		for (int i = 0; i < config->BlockedChannelsCount; i++)
			WLog_INFO(TAG, "Blocking %s", config->BlockedChannels[i]);
	}
	else
	{
		WLog_INFO(TAG, "Channels mode: WHITELIST");

		for (int i = 0; i < config->AllowedChannelsCount; i++)
			WLog_INFO(TAG, "Allowing %s", config->AllowedChannels[i]);
	}

	status = pf_server_start(server);
fail:
	proxy_server_free(server);
	return status;
}

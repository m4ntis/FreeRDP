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
	server = proxy_server_new();

	if (server == NULL)
	{
		status = -1;
		goto fail;
	}

	if (!pf_server_load_config("config.ini", server->config))
	{
		WLog_ERR(TAG, "An error occured while parsing configuration file");
		status = -1;
		goto fail;
	}

	WLog_INFO(TAG, "Loaded server, allowing channels:");

	for (int i = 0; i < server->config->AllowedChannelsCount; i++)
		printf("  - %s\n", server->config->AllowedChannels[i]);

	WLog_INFO(TAG, "Loaded server, blocking channels:");

	for (int i = 0; i < server->config->DeniedChannelsCount; i++)
		printf("  - %s\n", server->config->DeniedChannels[i]);

	status = pf_server_start(server);
fail:
	proxy_server_free(server);
	return status;
}

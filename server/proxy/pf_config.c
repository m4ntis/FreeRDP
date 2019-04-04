#include "pf_config.h"

BOOL pf_server_load_config(char* path, proxyConfig* config)
{
	wIniFile* ini = IniFile_New();
	IniFile_ReadFile(ini, path);
	/* general */
	config->host = IniFile_GetKeyValueString(ini, "general", "host");
	config->localOnly = IniFile_GetKeyValueInt(ini, "general", "localOnly");
	config->port = IniFile_GetKeyValueInt(ini, "general", "port");
	/* graphics */
	config->gfx = IniFile_GetKeyValueInt(ini, "graphics", "gfx");
	config->bitmapUpdate = IniFile_GetKeyValueInt(ini, "graphics", "bitmapUpdate");
	/* input */
	config->keyboard = IniFile_GetKeyValueInt(ini, "input", "keyboard");
	config->mouse = IniFile_GetKeyValueInt(ini, "input", "mouse");
	/* security */
	config->tlsSupported = IniFile_GetKeyValueInt(ini, "security", "tlsSupported");
	config->nlaSupported = IniFile_GetKeyValueInt(ini, "security", "nlaSupported");
	config->rdpSupported = IniFile_GetKeyValueInt(ini, "security", "rdpSupported");
	return TRUE;
}
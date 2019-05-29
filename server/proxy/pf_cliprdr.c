#include "pf_cliprdr.h"
#include "pf_log.h"

#define TAG PROXY_TAG("cliprdr")

BOOL pf_cliprdr_init(pServerContext* ps)
{
	CliprdrServerContext* cliprdr;
	cliprdr = ps->cliprdr = cliprdr_server_context_new(ps->vcm);

	if (!cliprdr)
	{
		return FALSE;
	}

	cliprdr->rdpcontext = (rdpContext*)ps;

	// enable all capabilities
	cliprdr->useLongFormatNames = TRUE;
	cliprdr->streamFileClipEnabled = TRUE;
	cliprdr->fileClipNoFilePaths = TRUE;
	cliprdr->canLockClipData = TRUE;

	cliprdr->autoInitializationSequence = FALSE;
	return cliprdr->Start(cliprdr);
}

void pf_cliprdr_free(pServerContext* ps)
{
	if (ps->cliprdr)
	{
		ps->cliprdr->Stop(ps->cliprdr);
		// cliprdr_server_context_free(ps->cliprdr);
	}
}


/* server callbacks */
static UINT pf_cliprdr_ClientCapabilities(CliprdrServerContext* context, const CLIPRDR_CAPABILITIES* capabilities)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientCapabilities(client, capabilities);
}

static UINT pf_cliprdr_TempDirectory(CliprdrServerContext* context, const CLIPRDR_TEMP_DIRECTORY* tempDirectory)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->TempDirectory(client, tempDirectory);
}

static UINT pf_cliprdr_ClientFormatList(CliprdrServerContext* context, const CLIPRDR_FORMAT_LIST* formatList)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFormatList(client, formatList);
}

static UINT pf_cliprdr_ClientFormatListResponse(CliprdrServerContext* context, const CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFormatListResponse(client, formatListResponse);
}

static UINT pf_cliprdr_ClientLockClipboardData(CliprdrServerContext* context, const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientLockClipboardData(client, lockClipboardData);
}

static UINT pf_cliprdr_ClientUnlockClipboardData(CliprdrServerContext* context, const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientUnlockClipboardData(client, unlockClipboardData);
}

static UINT pf_cliprdr_ClientFormatDataRequest(CliprdrServerContext* context, const CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFormatDataRequest(client, formatDataRequest);
}

static UINT pf_cliprdr_ClientFormatDataResponse(CliprdrServerContext* context, const CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFormatDataResponse(client, formatDataResponse);
}

static UINT pf_cliprdr_ClientFileContentsRequest(CliprdrServerContext* context, const CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFileContentsRequest(client, fileContentsRequest);
}

static UINT pf_cliprdr_ClientFileContentsResponse(CliprdrServerContext* context, const CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFileContentsResponse(client, fileContentsResponse);
}

/* client callbacks */

static UINT pf_cliprdr_ServerCapabilities(CliprdrClientContext* context, const CLIPRDR_CAPABILITIES* capabilities)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerCapabilities(server, capabilities);
}

static UINT pf_cliprdr_MonitorReady(CliprdrClientContext* context, const CLIPRDR_MONITOR_READY* monitorReady)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->MonitorReady(server, monitorReady);
}

static UINT pf_cliprdr_ServerFormatList(CliprdrClientContext* context, const CLIPRDR_FORMAT_LIST* formatList)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFormatList(server, formatList);
}


static UINT pf_cliprdr_ServerFormatListResponse(CliprdrClientContext* context, const CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFormatListResponse(server, formatListResponse);
}


static UINT pf_cliprdr_ServerLockClipboardData(CliprdrClientContext* context, const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerLockClipboardData(server, lockClipboardData);
}


static UINT pf_cliprdr_ServerUnlockClipboardData(CliprdrClientContext* context, const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerUnlockClipboardData(server, unlockClipboardData);
}


static UINT pf_cliprdr_ServerFormatDataRequest(CliprdrClientContext* context, const CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFormatDataRequest(server, formatDataRequest);
}


static UINT pf_cliprdr_ServerFormatDataResponse(CliprdrClientContext* context, const CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFormatDataResponse(server, formatDataResponse);
}


static UINT pf_cliprdr_ServerFileContentsRequest(CliprdrClientContext* context, const CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFileContentsRequest(server, fileContentsRequest);
}


static UINT pf_cliprdr_ServerFileContentsResponse(CliprdrClientContext* context, const CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFileContentsResponse(server, fileContentsResponse);
}

void pf_cliprdr_channel_register(CliprdrClientContext* cliprdr_client, CliprdrServerContext* cliprdr_server,
	proxyData* pdata)
{
	/* Set server and client side references to proxy data */
	cliprdr_server->custom = (void*) pdata;
	cliprdr_client->custom = (void*) pdata;

	/* Set server callbacks */
	cliprdr_server->ClientCapabilities = pf_cliprdr_ClientCapabilities;
	cliprdr_server->TempDirectory = pf_cliprdr_TempDirectory;
	cliprdr_server->ClientFormatList = pf_cliprdr_ClientFormatList;
	cliprdr_server->ClientFormatListResponse = pf_cliprdr_ClientFormatListResponse;
	cliprdr_server->ClientLockClipboardData = pf_cliprdr_ClientLockClipboardData;
	cliprdr_server->ClientUnlockClipboardData = pf_cliprdr_ClientUnlockClipboardData;
	cliprdr_server->ClientFormatDataRequest = pf_cliprdr_ClientFormatDataRequest;
	cliprdr_server->ClientFormatDataResponse = pf_cliprdr_ClientFormatDataResponse;
	cliprdr_server->ClientFileContentsRequest = pf_cliprdr_ClientFileContentsRequest;
	cliprdr_server->ClientFileContentsResponse = pf_cliprdr_ClientFileContentsResponse;

	/* Set client callbacks */
	cliprdr_client->ServerCapabilities = pf_cliprdr_ServerCapabilities;
	cliprdr_client->MonitorReady = pf_cliprdr_MonitorReady;
	cliprdr_client->ServerFormatList = pf_cliprdr_ServerFormatList;
	cliprdr_client->ServerFormatListResponse = pf_cliprdr_ServerFormatListResponse;
	cliprdr_client->ServerLockClipboardData = pf_cliprdr_ServerLockClipboardData;
	cliprdr_client->ServerUnlockClipboardData = pf_cliprdr_ServerUnlockClipboardData;
	cliprdr_client->ServerFormatDataRequest = pf_cliprdr_ServerFormatDataRequest;
	cliprdr_client->ServerFormatDataResponse = pf_cliprdr_ServerFormatDataResponse;
	cliprdr_client->ServerFileContentsRequest = pf_cliprdr_ServerFileContentsRequest;
	cliprdr_client->ServerFileContentsResponse = pf_cliprdr_ServerFileContentsResponse;
}
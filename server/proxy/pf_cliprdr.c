#include "pf_cliprdr.h"
#include "pf_log.h"

#define TAG PROXY_TAG("cliprdr")

BOOL pf_cliprdr_init(pServerContext* ps)
{
	CliprdrServerContext* cliprdr;
	cliprdr = ps->cliprdr = cliprdr_server_context_new(ps->vcm);

	if (!cliprdr)
	{
		WLog_ERR(TAG, "cliprdr_server_context_new failed.");
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
		ps->cliprdr->Stop(ps->cliprdr);

	cliprdr_server_context_free(ps->cliprdr);
	ps->cliprdr = NULL;
}

/* format data response PDU returns the copied text as a unicode buffer.
 * pf_cliprdr_is_copy_paste_valid returns TRUE if the length of the copied
 * text is valid according to the configuration value of `MaxTextLength`.
 */
static BOOL pf_cliprdr_is_copy_paste_valid(proxyConfig* config,
        const CLIPRDR_FORMAT_DATA_RESPONSE* pdu)
{
	size_t copy_len = (pdu->dataLen / 2) - 1;

	if (copy_len > config->MaxTextLength)
	{
		WLog_WARN(TAG, "Text size is too large: %"PRIu32" (max %"PRIu32")", copy_len,
		          config->MaxTextLength);
		return FALSE;
	}

	return TRUE;
}

/* if the requested text size is too long, we need a way to return a
 * message to the other side of the connection, indicating that the copy/paste
 * operation failed, instead of just not forwarding the response
 * (because that destroys the state of the RDPECLIP channel). It is done by sending a
 * format_data_response PDU with msgFlags` = CB_RESPONSE_FAIL.
 */
static void pf_cliprdr_create_failed_format_data_response(const CLIPRDR_FORMAT_DATA_RESPONSE* src,
        CLIPRDR_FORMAT_DATA_RESPONSE* dst)
{
	dst->requestedFormatData = src->requestedFormatData;
	dst->dataLen = src->dataLen;
	dst->msgType = src->msgType;
	dst->msgFlags = CB_RESPONSE_FAIL;
}

static void pf_cliprdr_create_failed_file_contents_response(const CLIPRDR_FILE_CONTENTS_RESPONSE*
        src, CLIPRDR_FILE_CONTENTS_RESPONSE* dst)
{
	dst->cbRequested = src->cbRequested;
	dst->streamId = src->streamId;
	dst->dwFlags = src->dwFlags;
	dst->requestedData = src->requestedData;
	dst->dataLen = src->dataLen;
	dst->msgType = src->msgType;
	dst->msgFlags = CB_RESPONSE_FAIL;
}

/* server callbacks */
static UINT pf_cliprdr_ClientCapabilities(CliprdrServerContext* context,
        const CLIPRDR_CAPABILITIES* capabilities)
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

static UINT pf_cliprdr_ClientFormatList(CliprdrServerContext* context,
                                        const CLIPRDR_FORMAT_LIST* formatList)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;

	if (pdata->config->AllowFileCopy)
		return client->ClientFormatList(client, formatList);

	/* send a format list that allows only text */
	/* TODO: send this using an array holding the allowed formats for text:
	 * CF_UNICODETEXT
	 * CF_OEMTEXT
	 * CF_TEXT
	 */
	CLIPRDR_FORMAT_LIST list;
	list.msgFlags = CB_RESPONSE_OK;
	list.msgType = CB_FORMAT_LIST;
	list.dataLen = 4 * 1 + 1;
	CLIPRDR_FORMAT formats[1] = { 0 };
	formats[0].formatId = CF_UNICODETEXT;
	formats[0].formatName = '\0';
	list.numFormats = 1;
	list.formats = formats;
	return client->ClientFormatList(client, &list);
}

static UINT pf_cliprdr_ClientFormatListResponse(CliprdrServerContext* context,
        const CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFormatListResponse(client, formatListResponse);
}

static UINT pf_cliprdr_ClientLockClipboardData(CliprdrServerContext* context,
        const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientLockClipboardData(client, lockClipboardData);
}

static UINT pf_cliprdr_ClientUnlockClipboardData(CliprdrServerContext* context,
        const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientUnlockClipboardData(client, unlockClipboardData);
}

static UINT pf_cliprdr_ClientFormatDataRequest(CliprdrServerContext* context,
        const CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;

	if (!pdata->config->AllowFileCopy && formatDataRequest->requestedFormatId != CF_UNICODETEXT)
	{
		CLIPRDR_FORMAT_DATA_RESPONSE resp;
		resp.msgFlags = CB_RESPONSE_FAIL;
		resp.msgType = CB_FORMAT_DATA_RESPONSE;
		resp.requestedFormatData = NULL;
		resp.dataLen = 0;
		return client->ClientFormatDataResponse(client, &resp);
	}

	return client->ClientFormatDataRequest(client, formatDataRequest);
}

static UINT pf_cliprdr_ClientFormatDataResponse(CliprdrServerContext* context,
        const CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;

	if (client->lastRequestedFormatId == CF_UNICODETEXT &&
	    !pf_cliprdr_is_copy_paste_valid(pdata->config, formatDataResponse))
	{
		CLIPRDR_FORMAT_DATA_RESPONSE resp;
		pf_cliprdr_create_failed_format_data_response(formatDataResponse, &resp);
		return client->ClientFormatDataResponse(client, &resp);
	}

	return client->ClientFormatDataResponse(client, formatDataResponse);
}

static UINT pf_cliprdr_ClientFileContentsRequest(CliprdrServerContext* context,
        const CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;

	if (!pdata->config->AllowFileCopy)
		return CHANNEL_RC_OK;

	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFileContentsRequest(client, fileContentsRequest);
}

static UINT pf_cliprdr_ClientFileContentsResponse(CliprdrServerContext* context,
        const CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrClientContext* client = pdata->pc->cliprdr;
	return client->ClientFileContentsResponse(client, fileContentsResponse);
}

/* client callbacks */

static UINT pf_cliprdr_ServerCapabilities(CliprdrClientContext* context,
        const CLIPRDR_CAPABILITIES* capabilities)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerCapabilities(server, capabilities);
}

static UINT pf_cliprdr_MonitorReady(CliprdrClientContext* context,
                                    const CLIPRDR_MONITOR_READY* monitorReady)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->MonitorReady(server, monitorReady);
}

static UINT pf_cliprdr_ServerFormatList(CliprdrClientContext* context,
                                        const CLIPRDR_FORMAT_LIST* formatList)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;

	if (pdata->config->AllowFileCopy)
		return server->ServerFormatList(server, formatList);

	/* send a format list that allows only text */
	CLIPRDR_FORMAT_LIST list;
	list.msgFlags = CB_RESPONSE_OK;
	list.msgType = CB_FORMAT_LIST;
	list.dataLen = 4 * 1 + 1;
	CLIPRDR_FORMAT formats[1] = { 0 };
	formats[0].formatId = CF_UNICODETEXT;
	formats[0].formatName = '\0';
	list.numFormats = 1;
	list.formats = formats;
	return server->ServerFormatList(server, &list);
}


static UINT pf_cliprdr_ServerFormatListResponse(CliprdrClientContext* context,
        const CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFormatListResponse(server, formatListResponse);
}


static UINT pf_cliprdr_ServerLockClipboardData(CliprdrClientContext* context,
        const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerLockClipboardData(server, lockClipboardData);
}


static UINT pf_cliprdr_ServerUnlockClipboardData(CliprdrClientContext* context,
        const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerUnlockClipboardData(server, unlockClipboardData);
}


static UINT pf_cliprdr_ServerFormatDataRequest(CliprdrClientContext* context,
        const CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;

	if (!pdata->config->AllowFileCopy && formatDataRequest->requestedFormatId != CF_UNICODETEXT)
	{
		CLIPRDR_FORMAT_DATA_RESPONSE resp;
		resp.msgFlags = CB_RESPONSE_FAIL;
		resp.msgType = CB_FORMAT_DATA_RESPONSE;
		resp.requestedFormatData = NULL;
		resp.dataLen = 0;
		return server->ServerFormatDataResponse(server, &resp);
	}

	return server->ServerFormatDataRequest(server, formatDataRequest);
}

static UINT pf_cliprdr_ServerFormatDataResponse(CliprdrClientContext* context,
        const CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;

	if (server->lastRequestedFormatId == CF_UNICODETEXT &&
	    !pf_cliprdr_is_copy_paste_valid(pdata->config, formatDataResponse))
	{
		CLIPRDR_FORMAT_DATA_RESPONSE resp;
		pf_cliprdr_create_failed_format_data_response(formatDataResponse, &resp);
		return server->ServerFormatDataResponse(server, &resp);
	}

	return server->ServerFormatDataResponse(server, formatDataResponse);
}


static UINT pf_cliprdr_ServerFileContentsRequest(CliprdrClientContext* context,
        const CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;

	if (!pdata->config->AllowFileCopy)
		return CHANNEL_RC_OK;

	CliprdrServerContext* server = pdata->ps->cliprdr;
	return server->ServerFileContentsRequest(server, fileContentsRequest);
}


static UINT pf_cliprdr_ServerFileContentsResponse(CliprdrClientContext* context,
        const CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyData* pdata = (proxyData*) context->custom;
	CliprdrServerContext* server = pdata->ps->cliprdr;

	if (!pdata->config->AllowFileCopy)
	{
		CLIPRDR_FILE_CONTENTS_RESPONSE resp;
		pf_cliprdr_create_failed_file_contents_response(fileContentsResponse, &resp);
		return server->ServerFileContentsResponse(server, &resp);
	}

	return server->ServerFileContentsResponse(server, fileContentsResponse);
}

void pf_cliprdr_channel_register(CliprdrClientContext* cliprdr_client,
                                 CliprdrServerContext* cliprdr_server,
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
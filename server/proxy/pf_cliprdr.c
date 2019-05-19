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
	return TRUE;
}

void pf_cliprdr_free(pServerContext* ps)
{
	if (ps->cliprdr)
		cliprdr_server_context_free(ps->cliprdr);
}

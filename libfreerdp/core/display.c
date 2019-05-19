#include "display.h"

BOOL freerdp_display_send_monitor_layout(rdpContext* context)
{
	rdpRdp* rdp = context->rdp;
	rdpSettings* settings = context->settings;

	wStream* st = rdp_data_pdu_init(rdp);

	if (!st)
		return FALSE;

	if (!rdp_write_monitor_layout_pdu(st, settings->MonitorCount, settings->MonitorDefArray))
	{
		Stream_Release(st);
		return FALSE;
	}

	return rdp_send_data_pdu(rdp, st, DATA_PDU_TYPE_MONITOR_LAYOUT, 0);
}
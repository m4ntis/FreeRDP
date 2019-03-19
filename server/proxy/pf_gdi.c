/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * X11 GDI
 *
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2014 Thincast Technologies GmbH
 * Copyright 2014 Norbert Federa <norbert.federa@thincast.com>
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

#include <freerdp/gdi/gdi.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>
#include <freerdp/constants.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>
#include <freerdp/freerdp.h>
#include "pf_gdi.h"
#include "pf_context.h"
#include "pf_log.h"

#include <freerdp/log.h>
#define TAG PROXY_TAG("gdi")

static BOOL pf_gdi_set_bounds(rdpContext* context,
                              const rdpBounds* bounds)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SetBounds, sContext, bounds);
}

static BOOL pf_gdi_dstblt(rdpContext* context, const DSTBLT_ORDER* dstblt)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->DstBlt, sContext, dstblt);
}

static BOOL pf_gdi_patblt(rdpContext* context, PATBLT_ORDER* patblt)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->PatBlt, sContext, patblt);
}

static BOOL pf_gdi_scrblt(rdpContext* context, const SCRBLT_ORDER* scrblt)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->ScrBlt, sContext, scrblt);
}

static BOOL pf_gdi_opaque_rect(rdpContext* context,
                               const OPAQUE_RECT_ORDER* opaque_rect)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->OpaqueRect, sContext, opaque_rect);
}

static BOOL pf_gdi_multi_opaque_rect(rdpContext* context,
                                     const MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;

	return IFCALLRESULT(FALSE, sContext->update->primary->MultiOpaqueRect, sContext, multi_opaque_rect);
}

static BOOL pf_gdi_line_to(rdpContext* context, const LINE_TO_ORDER* line_to)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->LineTo, sContext, line_to);
}

static BOOL pf_gdi_polyline(rdpContext* context,
                            const POLYLINE_ORDER* polyline)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->Polyline, sContext, polyline);
}

static BOOL pf_gdi_memblt(rdpContext* context, MEMBLT_ORDER* memblt)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->MemBlt, sContext, memblt);
}

static BOOL pf_gdi_mem3blt(rdpContext* context, MEM3BLT_ORDER* mem3blt)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->Mem3Blt, sContext, mem3blt);
}


static BOOL pf_gdi_polygon_sc(rdpContext* context,
                              const POLYGON_SC_ORDER* polygon_sc)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->PolygonSC, sContext, polygon_sc);
}

static BOOL pf_gdi_polygon_cb(rdpContext* context,
                              POLYGON_CB_ORDER* polygon_cb)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->PolygonCB, sContext, polygon_cb);
}

static BOOL pf_gdi_surface_frame_marker(rdpContext* context,
                                        const SURFACE_FRAME_MARKER* surface_frame_marker)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SurfaceFrameMarker, sContext, surface_frame_marker);
}

static BOOL pf_gdi_surface_bits(rdpContext* context,
                                const SURFACE_BITS_COMMAND* cmd)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SurfaceBits, sContext, cmd);
}

static BOOL pf_gdi_surface_frame_ack(rdpContext* context, UINT32 frameId)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SurfaceFrameAcknowledge, sContext, frameId);
}

static BOOL pf_update_send_cache_color_table(rdpContext* context,
        const CACHE_COLOR_TABLE_ORDER* cache_color_table)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheColorTable, sContext,
	                    cache_color_table);
}

static BOOL pf_gdi_cache_brush(rdpContext* context,
                               const CACHE_BRUSH_ORDER* cacheBrush)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->cache->brush->CacheBrush, sContext, cacheBrush);
}
static BOOL pf_update_send_cache_bitmap(rdpContext* context,
                                        const CACHE_BITMAP_ORDER* cache_bitmap)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->cache->bitmap->CacheBitmap, sContext, cache_bitmap);
}

static BOOL pf_update_send_cache_bitmap_v2(rdpContext* context,
        CACHE_BITMAP_V2_ORDER* cache_bitmap_v2)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->cache->bitmap->CacheBitmapV2, sContext, cache_bitmap_v2);
}

static BOOL pf_update_send_cache_bitmap_v3(rdpContext* context,
        CACHE_BITMAP_V3_ORDER* cache_bitmap_v3)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->cache->bitmap->CacheBitmapV3, sContext, cache_bitmap_v3);
}

static BOOL pf_gdi_glyph_index(rdpContext* context,
                               GLYPH_INDEX_ORDER* glyph_index)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->GlyphIndex, sContext, glyph_index);
}
static BOOL pf_gdi_fast_index(rdpContext* context,
                              const FAST_INDEX_ORDER* fast_index)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->FastIndex, sContext, fast_index);
}
static BOOL pf_gdi_fast_glyph(rdpContext* context,
                              const FAST_GLYPH_ORDER* fast_glyph)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->FastGlyph, sContext, fast_glyph);
}
static BOOL pf_gdi_cache_glyph(rdpContext* context,
                               const CACHE_GLYPH_ORDER* cache_glyph)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheGlyph, sContext, cache_glyph);
}

static BOOL pf_gdi_cache_glyph_v2(rdpContext* context,
                                  const CACHE_GLYPH_V2_ORDER* cache_glyph_v2)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheGlyphV2, sContext, cache_glyph_v2);
}

static BOOL pf_update_send_create_offscreen_bitmap(
    rdpContext* context,
    const CREATE_OFFSCREEN_BITMAP_ORDER* create_offscreen_bitmap)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->CreateOffscreenBitmap, sContext,
	                    create_offscreen_bitmap);
}

static BOOL pf_gdi_switch_surface(rdpContext* context,
                                  const SWITCH_SURFACE_ORDER* switch_surface)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->SwitchSurface, sContext, switch_surface);
}
void pf_gdi_register_update_callbacks(rdpUpdate* update)
{
	rdpPrimaryUpdate* primary = update->primary;
	update->SetBounds = pf_gdi_set_bounds;
	primary->DstBlt = pf_gdi_dstblt;
	primary->PatBlt = pf_gdi_patblt;
	primary->ScrBlt = pf_gdi_scrblt;
	primary->OpaqueRect = pf_gdi_opaque_rect;
	primary->MultiOpaqueRect = pf_gdi_multi_opaque_rect;
	primary->LineTo = pf_gdi_line_to;
	primary->Polyline = pf_gdi_polyline;
	primary->MemBlt = pf_gdi_memblt;
	primary->Mem3Blt = pf_gdi_mem3blt;
	primary->PolygonSC = pf_gdi_polygon_sc;
	primary->PolygonCB = pf_gdi_polygon_cb;
	update->SurfaceBits = pf_gdi_surface_bits;
	update->SurfaceFrameMarker = pf_gdi_surface_frame_marker;
	rdpCache* cache = update->context->cache;
	cache->bitmap->MemBlt = update->primary->MemBlt;
	cache->bitmap->Mem3Blt = update->primary->Mem3Blt;
	update->primary->MemBlt = pf_gdi_memblt;
	update->primary->Mem3Blt = pf_gdi_mem3blt;
	update->secondary->CacheBitmap = pf_update_send_cache_bitmap;
	update->secondary->CacheBitmapV2 = pf_update_send_cache_bitmap_v2;
	update->secondary->CacheBitmapV3 = pf_update_send_cache_bitmap_v3;
	update->SurfaceFrameAcknowledge = pf_gdi_surface_frame_ack;
	update->primary->GlyphIndex = pf_gdi_glyph_index;
	update->primary->FastIndex = pf_gdi_fast_index;
	update->primary->FastGlyph = pf_gdi_fast_glyph;
	update->secondary->CacheGlyph = pf_gdi_cache_glyph;
	update->secondary->CacheGlyphV2 = pf_gdi_cache_glyph_v2;
	cache->brush->PatBlt = update->primary->PatBlt;
	cache->brush->PolygonSC = update->primary->PolygonSC;
	cache->brush->PolygonCB = update->primary->PolygonCB;
	update->primary->PatBlt = pf_gdi_patblt;
	update->primary->PolygonSC = pf_gdi_polygon_sc;
	update->primary->PolygonCB = pf_gdi_polygon_cb;
	update->secondary->CacheBrush = pf_gdi_cache_brush;
	update->altsec->CreateOffscreenBitmap = pf_update_send_create_offscreen_bitmap;
	update->altsec->SwitchSurface = pf_gdi_switch_surface;
	update->secondary->CacheColorTable = pf_update_send_cache_color_table;
}


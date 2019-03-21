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
	return TRUE;
	return sContext->update->primary->MultiOpaqueRect(sContext, multi_opaque_rect);
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



static BOOL pf_update_send_create_offscreen_bitmap(
    rdpContext* context,
    const CREATE_OFFSCREEN_BITMAP_ORDER* create_offscreen_bitmap)
{
	
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	if (sContext->settings->OffscreenSupportLevel != 0)
		return TRUE;
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
static BOOL pf_pointer_cached(rdpContext* context,
        const POINTER_CACHED_UPDATE* pointerCached)
{
		WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->pointer->PointerCached, sContext, pointerCached);

}
static BOOL pf_pointer_position(rdpContext* context,
        const POINTER_POSITION_UPDATE* pointerPosition)
{
		WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->pointer->PointerPosition, sContext, pointerPosition);

}

static BOOL pf_pointer_system(rdpContext* context,
        const POINTER_SYSTEM_UPDATE* pointerSystem)
{
		WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->pointer->PointerSystem, sContext, pointerSystem);

}

static BOOL pf_pointer_color(rdpContext* context,
                                        const POINTER_COLOR_UPDATE* pointerColor)
{
		WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->pointer->PointerColor, sContext, pointerColor);

}

static BOOL pf_pointer_new(rdpContext* context,
                                      const POINTER_NEW_UPDATE* pointerNew)
{
			WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->pointer->PointerNew, sContext, pointerNew);

}

static BOOL pf_window_create(rdpContext *context, WINDOW_ORDER_INFO *orderInfo, WINDOW_STATE_ORDER *windowState)
{
			WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->WindowCreate, sContext, orderInfo, windowState);
}

static BOOL pf_window_update(rdpContext *context, WINDOW_ORDER_INFO *orderInfo, WINDOW_STATE_ORDER *window_state)
{
			WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->WindowUpdate, sContext, orderInfo, window_state);
}

static BOOL pf_window_cached_icon(rdpContext* context, WINDOW_ORDER_INFO* orderInfo,
        WINDOW_CACHED_ICON_ORDER* windowCachedIcon)
{
				WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->WindowCachedIcon, sContext, orderInfo, windowCachedIcon);

}

static BOOL pf_window_delete(rdpContext *context, WINDOW_ORDER_INFO *orderInfo)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->WindowDelete, sContext, orderInfo);
}

static BOOL pf_window_icon(rdpContext *context, WINDOW_ORDER_INFO *orderInfo, WINDOW_ICON_ORDER *window_icon)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->WindowIcon, sContext, orderInfo, window_icon);

}

static BOOL pf_window_notify_icon_create(rdpContext *context, WINDOW_ORDER_INFO *orderInfo, NOTIFY_ICON_STATE_ORDER *notify_icon_state)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->NotifyIconCreate, sContext, orderInfo, notify_icon_state);

}

static BOOL pf_window_notify_icon_update(rdpContext *context, WINDOW_ORDER_INFO *orderInfo, NOTIFY_ICON_STATE_ORDER *notify_icon_state){
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->NotifyIconUpdate, sContext, orderInfo, notify_icon_state);

}

static BOOL pf_surface_cmd(rdpContext* context, wStream* s)
{
	WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SurfaceCommand, sContext, s);

}

static BOOL pf_gdi_ellipse_sc(rdpContext* context,
                           const ELLIPSE_SC_ORDER* ellipse_sc)
{
		WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->EllipseSC, sContext, ellipse_sc);

}
static BOOL pf_gdi_ellipse_cb(rdpContext* context,
                           const ELLIPSE_CB_ORDER* ellipse_cb)
{
		WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->EllipseCB, sContext, ellipse_cb);

}

static BOOL pf_save_bitmap(rdpContext* context,
                            const SAVE_BITMAP_ORDER* save_bitmap)
{
			WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->SaveBitmap, sContext, save_bitmap);
}

static BOOL pf_multi_draw_nine_grid(rdpContext* context,
        const MULTI_DRAW_NINE_GRID_ORDER* multiDrawNineGrid)
{
			WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->MultiDrawNineGrid, sContext, multiDrawNineGrid);

}

static BOOL pf_multi_dstblt(rdpContext* context,
                                       const MULTI_DSTBLT_ORDER* multiDstBlt)
{
				WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->MultiDstBlt, sContext, multiDstBlt);
}

static BOOL pf_multi_patblt(rdpContext* context,
                                       const MULTI_PATBLT_ORDER* multiPatBlt)
{
				WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->MultiPatBlt, sContext, multiPatBlt);

}

static BOOL pf_multi_scrblt(rdpContext* context,
                                       const MULTI_SCRBLT_ORDER* multiScrBlt)
{
				WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->MultiScrBlt, sContext, multiScrBlt);

}

static BOOL pf_CreateNineGridBitmap(rdpContext* context,
                                      const CREATE_NINE_GRID_BITMAP_ORDER* create_nine_grid_bitmap)
{
					WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->CreateNineGridBitmap, sContext, create_nine_grid_bitmap);

}

static BOOL pf_FrameMarker(rdpContext* context,
                             const FRAME_MARKER_ORDER* frame_marker)
{
					WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->FrameMarker, sContext, frame_marker);

}

static BOOL pf_StreamBitmapFirst(rdpContext* context,
                            const STREAM_BITMAP_FIRST_ORDER* stream_bitmap_first)
{
						WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->StreamBitmapFirst, sContext, stream_bitmap_first);

}
static BOOL pf_StreamBitmapNext(rdpContext* context,
                           const STREAM_BITMAP_NEXT_ORDER* stream_bitmap_next)
{
							WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->StreamBitmapNext, sContext, stream_bitmap_next);

}
static BOOL pf_DrawGdiPlusFirst(rdpContext* context,
                           const DRAW_GDIPLUS_FIRST_ORDER* draw_gdiplus_first)
{
							WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->DrawGdiPlusFirst, sContext, draw_gdiplus_first);

}
static BOOL pf_DrawGdiPlusNext(rdpContext* context,
                          const DRAW_GDIPLUS_NEXT_ORDER* draw_gdiplus_next)
{
								WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	BOOL rc = FALSE;
	IFCALLRET(sContext->update->altsec->DrawGdiPlusNext, rc, sContext, draw_gdiplus_next);
	return rc;

}
static BOOL pf_DrawGdiPlusEnd(rdpContext* context,
                         const DRAW_GDIPLUS_END_ORDER* draw_gdiplus_end)
{
								WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
		BOOL rc = FALSE;
	IFCALLRET(sContext->update->altsec->DrawGdiPlusEnd, rc, sContext, draw_gdiplus_end);
	return rc;

}
static BOOL pf_DrawGdiPlusCacheFirst(rdpContext* context,
                                const DRAW_GDIPLUS_CACHE_FIRST_ORDER* draw_gdiplus_cache_first)
{
									WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->DrawGdiPlusCacheFirst, sContext, draw_gdiplus_cache_first);

}
static BOOL pf_DrawGdiPlusCacheNext(rdpContext* context,
                               const DRAW_GDIPLUS_CACHE_NEXT_ORDER* draw_gdiplus_cache_next)
{
									WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->DrawGdiPlusCacheNext, sContext, draw_gdiplus_cache_next);

}
static BOOL pf_DrawGdiPlusCacheEnd(rdpContext* context,
                                     const DRAW_GDIPLUS_CACHE_END_ORDER* draw_gdiplus_cache_end)
{
										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->altsec->DrawGdiPlusCacheEnd, sContext, draw_gdiplus_cache_end);

}

static BOOL pf_CacheBitmap(rdpContext* context,
                             const CACHE_BITMAP_ORDER* cache_bitmap_order)
{
										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheBitmap, sContext, cache_bitmap_order);

}
static BOOL pf_CacheBitmapV2(rdpContext* context,
                               CACHE_BITMAP_V2_ORDER* cache_bitmap_v2_order)
{

										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheBitmapV2, sContext, cache_bitmap_v2_order);

}
static BOOL pf_CacheBitmapV3(rdpContext* context,
                               CACHE_BITMAP_V3_ORDER* cache_bitmap_v3_order)
{
										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheBitmapV3, sContext, cache_bitmap_v3_order);

}

static BOOL pf_CacheGlyph(rdpContext* context,
                            const CACHE_GLYPH_ORDER* cache_glyph_order)
{
										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheGlyph, sContext, cache_glyph_order);

}
static BOOL pf_CacheGlyphV2(rdpContext* context,
                              const CACHE_GLYPH_V2_ORDER* cache_glyph_v2_order)
{
										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheGlyphV2, sContext, cache_glyph_v2_order);

}
static BOOL pf_CacheBrush(rdpContext* context,
                            const CACHE_BRUSH_ORDER* cache_brush_order)
{
										WLog_INFO(TAG, __FUNCTION__);
	proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->secondary->CacheBrush, sContext, cache_brush_order);

}

static BOOL pf_GlyphIndex(rdpContext* context,
                            GLYPH_INDEX_ORDER* glyph_index)
{
										WLog_INFO(TAG, __FUNCTION__);

				proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->GlyphIndex, sContext, glyph_index);

}
static BOOL pf_FastIndex(rdpContext* context,
                           const FAST_INDEX_ORDER* fast_index)
{
										WLog_INFO(TAG, __FUNCTION__);

			proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->FastIndex, sContext, fast_index);
}
static BOOL pf_FastGlyph(rdpContext* context,
                           const FAST_GLYPH_ORDER* fast_glyph)
{
										WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->primary->FastGlyph, sContext, fast_glyph);

}

static BOOL pf_NotifyIconDelete(rdpContext* context, WINDOW_ORDER_INFO* orderInfo)
{
											WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->NotifyIconDelete, sContext, orderInfo);

}
static BOOL pf_MonitoredDesktop(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, MONITORED_DESKTOP_ORDER* monitored_desktop)
{
											WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->MonitoredDesktop, sContext, orderInfo, monitored_desktop);

}
static BOOL pf_NonMonitoredDesktop(rdpContext* context, WINDOW_ORDER_INFO* orderInfo)
{
											WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->window->NonMonitoredDesktop, sContext, orderInfo);

}

static BOOL pf_SurfaceFrameBits(rdpContext* context,
                                  const SURFACE_BITS_COMMAND* cmd, BOOL first,
                                  BOOL last, UINT32 frameId)
{
	WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SurfaceFrameBits, sContext, cmd, first, last, frameId);
}

static BOOL pf_Sync(rdpContext* context)
{
		WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->Synchronize, sContext);

}

static BOOL pf_PlaySound(rdpContext* context, const PLAY_SOUND_UPDATE* play_sound)
{
		WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->PlaySound, sContext, play_sound);

}

static BOOL pf_SetKeyboardIndicators(rdpContext* context, UINT16 led_flags)
{
			WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SetKeyboardIndicators, sContext, led_flags);
}
static BOOL pf_SetKeyboardImeStatus(rdpContext* context, UINT16 imeId, UINT32 imeState,
                                      UINT32 imeConvMode)
{
			WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SetKeyboardImeStatus, sContext, imeId, imeState, imeConvMode);

}
static BOOL pf_SaveSessionInfo(rdpContext* context, UINT32 type, void* data)
{
			WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->SaveSessionInfo, sContext, type, data);

}
static BOOL pf_ServerStatusInfo(rdpContext* context, UINT32 status)
{
			WLog_INFO(TAG, __FUNCTION__);

		proxyContext* pContext = (proxyContext*)context;
	rdpContext* sContext = (rdpContext*)pContext->peerContext;
	return IFCALLRESULT(FALSE, sContext->update->ServerStatusInfo, sContext, status);

}


void pf_gdi_register_update_callbacks(rdpUpdate* update)
{
	rdpPrimaryUpdate* primary = update->primary;
	rdpSecondaryUpdate* secondary = update->secondary;

	update->SetKeyboardIndicators = pf_SetKeyboardIndicators;
	update->SetKeyboardImeStatus = pf_SetKeyboardImeStatus;
	update->SaveSessionInfo = pf_SaveSessionInfo;
	update->ServerStatusInfo = pf_ServerStatusInfo;

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
	primary->EllipseCB = pf_gdi_ellipse_cb;
	primary->EllipseSC = pf_gdi_ellipse_sc;
	primary->SaveBitmap = pf_save_bitmap;

	primary->GlyphIndex = pf_GlyphIndex;
	primary->FastIndex = pf_FastIndex;
	primary->FastGlyph = pf_FastGlyph;

	primary->MultiDstBlt = pf_multi_dstblt;
	primary->MultiScrBlt = pf_multi_scrblt;
	primary->MultiPatBlt = pf_multi_patblt;
	primary->MultiDrawNineGrid = pf_multi_draw_nine_grid;
	update->PlaySound = pf_PlaySound;
	update->SurfaceCommand = pf_surface_cmd;
	update->SurfaceBits = pf_gdi_surface_bits;
	update->SurfaceFrameBits = pf_SurfaceFrameBits;
	update->SurfaceFrameMarker = pf_gdi_surface_frame_marker;
	rdpCache* cache = update->context->cache;
	update->primary->MemBlt = pf_gdi_memblt;
	update->primary->Mem3Blt = pf_gdi_mem3blt;
	//cache->bitmap->MemBlt = update->primary->MemBlt;
	//cache->bitmap->Mem3Blt = update->primary->Mem3Blt;
	update->secondary->CacheBitmap = pf_update_send_cache_bitmap;
	update->secondary->CacheBitmapV2 = pf_update_send_cache_bitmap_v2;
	update->secondary->CacheBitmapV3 = pf_update_send_cache_bitmap_v3;
	
	update->primary->PatBlt = pf_gdi_patblt;
	update->primary->PolygonSC = pf_gdi_polygon_sc;
	update->primary->PolygonCB = pf_gdi_polygon_cb;
	//cache->brush->PatBlt = update->primary->PatBlt;
	//cache->brush->PolygonSC = update->primary->PolygonSC;
	//cache->brush->PolygonCB = update->primary->PolygonCB;
	update->secondary->CacheBrush = pf_gdi_cache_brush;

	update->altsec->CreateOffscreenBitmap = pf_update_send_create_offscreen_bitmap;
	update->altsec->SwitchSurface = pf_gdi_switch_surface;
	update->altsec->CreateNineGridBitmap = pf_CreateNineGridBitmap;
	update->altsec->FrameMarker = pf_FrameMarker;
	update->altsec->StreamBitmapFirst = pf_StreamBitmapFirst;
	update->altsec->StreamBitmapNext = pf_StreamBitmapNext;
	update->altsec->DrawGdiPlusFirst = pf_DrawGdiPlusFirst;
	update->altsec->DrawGdiPlusNext = pf_DrawGdiPlusNext;
	update->altsec->DrawGdiPlusEnd = pf_DrawGdiPlusEnd;
	update->altsec->DrawGdiPlusCacheFirst = pf_DrawGdiPlusCacheFirst;
	update->altsec->DrawGdiPlusCacheNext = pf_DrawGdiPlusCacheNext;
	update->altsec->DrawGdiPlusCacheEnd = pf_DrawGdiPlusCacheEnd;
	update->Synchronize = pf_Sync;
	update->pointer->PointerPosition = pf_pointer_position;
	update->pointer->PointerSystem = pf_pointer_system;
	update->pointer->PointerColor = pf_pointer_color;
	update->pointer->PointerNew = pf_pointer_new;
	update->pointer->PointerCached = pf_pointer_cached;

	update->window->WindowCreate = pf_window_create;
	update->window->WindowUpdate = pf_window_update;
	 update->window->WindowIcon = pf_window_icon;
	update->window->WindowCachedIcon = pf_window_cached_icon;
	 update->window->WindowDelete = pf_window_delete;
	update->window->NotifyIconCreate = pf_window_notify_icon_create;
	 update->window->NotifyIconUpdate = pf_window_notify_icon_update;
	update->window->NotifyIconDelete = pf_NotifyIconDelete;
	update->window->MonitoredDesktop = pf_MonitoredDesktop;
	update->window->NonMonitoredDesktop = pf_NonMonitoredDesktop;
	 /* sec */
	secondary->CacheBitmap = pf_CacheBitmap;
	secondary->CacheBitmapV2 = pf_CacheBitmapV2;
	secondary->CacheBitmapV3 = pf_CacheBitmapV3;
	secondary->CacheColorTable = pf_update_send_cache_color_table;
	secondary->CacheGlyph = pf_CacheGlyph;
	secondary->CacheGlyphV2 = pf_CacheGlyphV2;
	secondary->CacheBrush = pf_CacheBrush;
}


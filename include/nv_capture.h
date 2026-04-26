/*
 * NetVision - nv_capture.h
 * Packet capture interface using libpcap.
 * Developer: MERO:TG@QP4RM
 */

#ifndef NV_CAPTURE_H
#define NV_CAPTURE_H

#include "nv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nv_capture_ctx nv_capture_ctx_t;

NV_API nv_capture_ctx_t *nv_capture_create(const char *device, int snaplen, int promisc);
NV_API void              nv_capture_destroy(nv_capture_ctx_t *ctx);
NV_API int               nv_capture_start(nv_capture_ctx_t *ctx);
NV_API int               nv_capture_stop(nv_capture_ctx_t *ctx);
NV_API int               nv_capture_set_filter(nv_capture_ctx_t *ctx, const char *filter_expr);
NV_API int               nv_capture_next_packet(nv_capture_ctx_t *ctx, nv_packet_t *pkt);
NV_API int               nv_capture_is_running(nv_capture_ctx_t *ctx);
NV_API const char       *nv_capture_get_error(nv_capture_ctx_t *ctx);
NV_API int               nv_capture_list_devices(char devices[][NV_MAX_NAME_LEN], int max_devices);

#ifdef __cplusplus
}
#endif

#endif

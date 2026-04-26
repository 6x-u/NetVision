/*
 * NetVision - nv_capture_stub.c
 * Stub capture implementation for cross-compilation targets
 * where libpcap is not available. Link against npcap on Windows.
 * Developer: MERO:TG@QP4RM
 */

#include "nv_capture.h"
#include <stdlib.h>
#include <string.h>

struct nv_capture_ctx {
    char     device[NV_MAX_NAME_LEN];
    char     errbuf[256];
    int      snaplen;
    int      promisc;
    int      running;
    uint32_t pkt_id_counter;
};

NV_API nv_capture_ctx_t *nv_capture_create(const char *device, int snaplen, int promisc)
{
    nv_capture_ctx_t *ctx = (nv_capture_ctx_t *)calloc(1, sizeof(nv_capture_ctx_t));
    if (!ctx)
        return NULL;
    if (device)
        strncpy(ctx->device, device, NV_MAX_NAME_LEN - 1);
    ctx->snaplen = (snaplen > 0) ? snaplen : 65535;
    ctx->promisc = promisc;
    ctx->pkt_id_counter = 1;
    strncpy(ctx->errbuf, "stub: link with npcap for real capture", sizeof(ctx->errbuf) - 1);
    return ctx;
}

NV_API void nv_capture_destroy(nv_capture_ctx_t *ctx)
{
    if (ctx)
        free(ctx);
}

NV_API int nv_capture_start(nv_capture_ctx_t *ctx)
{
    if (!ctx)
        return -1;
    ctx->running = 1;
    return 0;
}

NV_API int nv_capture_stop(nv_capture_ctx_t *ctx)
{
    if (!ctx)
        return -1;
    ctx->running = 0;
    return 0;
}

NV_API int nv_capture_set_filter(nv_capture_ctx_t *ctx, const char *filter_expr)
{
    (void)ctx;
    (void)filter_expr;
    return 0;
}

NV_API int nv_capture_next_packet(nv_capture_ctx_t *ctx, nv_packet_t *pkt)
{
    (void)ctx;
    (void)pkt;
    return -1;
}

NV_API int nv_capture_is_running(nv_capture_ctx_t *ctx)
{
    return ctx ? ctx->running : 0;
}

NV_API const char *nv_capture_get_error(nv_capture_ctx_t *ctx)
{
    return ctx ? ctx->errbuf : "null context";
}

NV_API int nv_capture_list_devices(char devices[][NV_MAX_NAME_LEN], int max_devices)
{
    (void)devices;
    (void)max_devices;
    return 0;
}

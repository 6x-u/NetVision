/*
 * NetVision - nv_session.h
 * Network session tracking and management.
 * Developer: MERO:TG@QP4RM
 */

#ifndef NV_SESSION_H
#define NV_SESSION_H

#include "nv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nv_session_mgr nv_session_mgr_t;

NV_API nv_session_mgr_t *nv_session_mgr_create(void);
NV_API void              nv_session_mgr_destroy(nv_session_mgr_t *mgr);
NV_API uint32_t          nv_session_track(nv_session_mgr_t *mgr, const nv_packet_t *pkt);
NV_API nv_session_t     *nv_session_get(nv_session_mgr_t *mgr, uint32_t id);
NV_API nv_session_t     *nv_session_find(nv_session_mgr_t *mgr, uint32_t src_node, uint32_t dst_node, nv_protocol_t proto);
NV_API void              nv_session_close(nv_session_mgr_t *mgr, uint32_t id);
NV_API void              nv_session_cleanup_idle(nv_session_mgr_t *mgr, double timeout_sec);
NV_API uint32_t          nv_session_count(nv_session_mgr_t *mgr);
NV_API uint32_t          nv_session_active_count(nv_session_mgr_t *mgr);
NV_API nv_session_t     *nv_session_get_all(nv_session_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif

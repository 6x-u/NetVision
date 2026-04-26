/*
 * NetVision - nv_session.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_session.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct nv_session_mgr {
    nv_session_t sessions[NV_MAX_SESSIONS];
    uint32_t     count;
    uint32_t     next_id;
};

static double nv_sess_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

NV_API nv_session_mgr_t *nv_session_mgr_create(void)
{
    nv_session_mgr_t *mgr = (nv_session_mgr_t *)calloc(1, sizeof(nv_session_mgr_t));
    if (!mgr)
        return NULL;
    mgr->next_id = 1;
    return mgr;
}

NV_API void nv_session_mgr_destroy(nv_session_mgr_t *mgr)
{
    if (mgr)
        free(mgr);
}

NV_API uint32_t nv_session_track(nv_session_mgr_t *mgr, const nv_packet_t *pkt)
{
    nv_session_t *sess;
    double now;

    if (!mgr || !pkt)
        return 0;

    sess = nv_session_find(mgr, pkt->src_node_id, pkt->dst_node_id, pkt->protocol);
    now = nv_sess_time();

    if (sess) {
        sess->bytes_transferred += pkt->size;
        sess->pkt_count++;
        sess->last_activity = now;
        sess->total_latency_ms += pkt->latency_ms;
        if (pkt->status == NV_PKT_RETRANSMIT)
            sess->retransmits++;
        if (pkt->status == NV_PKT_ERROR)
            sess->errors++;
        return sess->id;
    }

    if (mgr->count >= NV_MAX_SESSIONS)
        return 0;

    sess = &mgr->sessions[mgr->count];
    memset(sess, 0, sizeof(nv_session_t));
    sess->id = mgr->next_id++;
    sess->src_node_id = pkt->src_node_id;
    sess->dst_node_id = pkt->dst_node_id;
    sess->protocol = pkt->protocol;
    sess->state = NV_SESS_ACTIVE;
    sess->bytes_transferred = pkt->size;
    sess->pkt_count = 1;
    sess->start_time = now;
    sess->last_activity = now;
    sess->total_latency_ms = pkt->latency_ms;
    mgr->count++;

    return sess->id;
}

NV_API nv_session_t *nv_session_get(nv_session_mgr_t *mgr, uint32_t id)
{
    uint32_t i;
    if (!mgr)
        return NULL;
    for (i = 0; i < mgr->count; i++) {
        if (mgr->sessions[i].id == id)
            return &mgr->sessions[i];
    }
    return NULL;
}

NV_API nv_session_t *nv_session_find(nv_session_mgr_t *mgr, uint32_t src_node, uint32_t dst_node, nv_protocol_t proto)
{
    uint32_t i;
    if (!mgr)
        return NULL;
    for (i = 0; i < mgr->count; i++) {
        nv_session_t *s = &mgr->sessions[i];
        if (s->state == NV_SESS_CLOSED)
            continue;
        if ((s->src_node_id == src_node && s->dst_node_id == dst_node) ||
            (s->src_node_id == dst_node && s->dst_node_id == src_node)) {
            if (s->protocol == proto)
                return s;
        }
    }
    return NULL;
}

NV_API void nv_session_close(nv_session_mgr_t *mgr, uint32_t id)
{
    nv_session_t *sess = nv_session_get(mgr, id);
    if (sess)
        sess->state = NV_SESS_CLOSED;
}

NV_API void nv_session_cleanup_idle(nv_session_mgr_t *mgr, double timeout_sec)
{
    uint32_t i;
    double now;

    if (!mgr)
        return;

    now = nv_sess_time();
    for (i = 0; i < mgr->count; i++) {
        if (mgr->sessions[i].state == NV_SESS_ACTIVE) {
            if ((now - mgr->sessions[i].last_activity) > timeout_sec)
                mgr->sessions[i].state = NV_SESS_IDLE;
        }
        if (mgr->sessions[i].state == NV_SESS_IDLE) {
            if ((now - mgr->sessions[i].last_activity) > timeout_sec * 3)
                mgr->sessions[i].state = NV_SESS_CLOSED;
        }
    }
}

NV_API uint32_t nv_session_count(nv_session_mgr_t *mgr)
{
    return mgr ? mgr->count : 0;
}

NV_API uint32_t nv_session_active_count(nv_session_mgr_t *mgr)
{
    uint32_t i, active = 0;
    if (!mgr)
        return 0;
    for (i = 0; i < mgr->count; i++) {
        if (mgr->sessions[i].state == NV_SESS_ACTIVE)
            active++;
    }
    return active;
}

NV_API nv_session_t *nv_session_get_all(nv_session_mgr_t *mgr)
{
    return mgr ? mgr->sessions : NULL;
}

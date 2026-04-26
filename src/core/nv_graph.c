/*
 * NetVision - nv_graph.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_graph.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

struct nv_graph {
    nv_node_t   nodes[NV_MAX_NODES];
    nv_edge_t   edges[NV_MAX_EDGES];
    uint32_t    node_count;
    uint32_t    edge_count;
    uint32_t    next_node_id;
    nv_stats_t  stats;
    nv_filter_t active_filter;
    double      last_update;
};

static double nv_time_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

NV_API nv_graph_t *nv_graph_create(void)
{
    nv_graph_t *g = (nv_graph_t *)calloc(1, sizeof(nv_graph_t));
    if (!g)
        return NULL;
    g->next_node_id = 1;
    g->last_update = nv_time_now();
    return g;
}

NV_API void nv_graph_destroy(nv_graph_t *g)
{
    if (g)
        free(g);
}

NV_API uint32_t nv_graph_add_node(nv_graph_t *g, const char *ip, uint16_t port, nv_node_type_t type)
{
    nv_node_t *existing;
    nv_node_t *node;

    if (!g || g->node_count >= NV_MAX_NODES)
        return 0;

    existing = nv_graph_find_node(g, ip);
    if (existing)
        return existing->id;

    node = &g->nodes[g->node_count];
    memset(node, 0, sizeof(nv_node_t));
    node->id = g->next_node_id++;
    strncpy(node->ip, ip, NV_MAX_IP_LEN - 1);
    node->port = port;
    node->type = type;
    node->active = 1;

    node->pos_x = 400.0f + (float)(g->node_count % 8) * 120.0f;
    node->pos_y = 200.0f + (float)(g->node_count / 8) * 100.0f;

    g->node_count++;
    return node->id;
}

NV_API nv_node_t *nv_graph_find_node(nv_graph_t *g, const char *ip)
{
    uint32_t i;
    if (!g || !ip)
        return NULL;
    for (i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i].ip, ip) == 0)
            return &g->nodes[i];
    }
    return NULL;
}

NV_API nv_node_t *nv_graph_get_node(nv_graph_t *g, uint32_t id)
{
    uint32_t i;
    if (!g)
        return NULL;
    for (i = 0; i < g->node_count; i++) {
        if (g->nodes[i].id == id)
            return &g->nodes[i];
    }
    return NULL;
}

NV_API int nv_graph_remove_node(nv_graph_t *g, uint32_t id)
{
    uint32_t i;
    if (!g)
        return -1;
    for (i = 0; i < g->node_count; i++) {
        if (g->nodes[i].id == id) {
            g->nodes[i].active = 0;
            return 0;
        }
    }
    return -1;
}

NV_API uint32_t nv_graph_add_edge(nv_graph_t *g, uint32_t src_id, uint32_t dst_id)
{
    nv_edge_t *existing;
    nv_edge_t *edge;

    if (!g || g->edge_count >= NV_MAX_EDGES)
        return 0;

    existing = nv_graph_get_edge(g, src_id, dst_id);
    if (existing) {
        existing->pkt_count++;
        return 1;
    }

    edge = &g->edges[g->edge_count];
    memset(edge, 0, sizeof(nv_edge_t));
    edge->src_node_id = src_id;
    edge->dst_node_id = dst_id;
    edge->pkt_count = 1;
    edge->active = 1;
    g->edge_count++;
    return 1;
}

NV_API nv_edge_t *nv_graph_get_edge(nv_graph_t *g, uint32_t src_id, uint32_t dst_id)
{
    uint32_t i;
    if (!g)
        return NULL;
    for (i = 0; i < g->edge_count; i++) {
        if (g->edges[i].src_node_id == src_id && g->edges[i].dst_node_id == dst_id)
            return &g->edges[i];
        if (g->edges[i].src_node_id == dst_id && g->edges[i].dst_node_id == src_id)
            return &g->edges[i];
    }
    return NULL;
}

NV_API void nv_graph_process_packet(nv_graph_t *g, const nv_packet_t *pkt)
{
    char src_ip[NV_MAX_IP_LEN], dst_ip[NV_MAX_IP_LEN];
    uint32_t src_addr, dst_addr;
    uint32_t src_id, dst_id;
    nv_node_type_t src_type, dst_type;
    nv_node_t *src_node, *dst_node;
    nv_edge_t *edge;

    if (!g || !pkt)
        return;

    src_addr = pkt->src_node_id;
    dst_addr = pkt->dst_node_id;
    snprintf(src_ip, NV_MAX_IP_LEN, "%u.%u.%u.%u",
             (src_addr >> 24) & 0xFF, (src_addr >> 16) & 0xFF,
             (src_addr >> 8) & 0xFF, src_addr & 0xFF);
    snprintf(dst_ip, NV_MAX_IP_LEN, "%u.%u.%u.%u",
             (dst_addr >> 24) & 0xFF, (dst_addr >> 16) & 0xFF,
             (dst_addr >> 8) & 0xFF, dst_addr & 0xFF);

    src_type = NV_NODE_CLIENT;
    dst_type = NV_NODE_SERVER;

    if (pkt->dst_port == 80 || pkt->dst_port == 443 || pkt->dst_port == 8080)
        dst_type = NV_NODE_SERVER;
    if (pkt->dst_port == 8443 || pkt->dst_port == 3000 || pkt->dst_port == 5000)
        dst_type = NV_NODE_API;

    src_id = nv_graph_add_node(g, src_ip, pkt->src_port, src_type);
    dst_id = nv_graph_add_node(g, dst_ip, pkt->dst_port, dst_type);

    if (src_id == 0 || dst_id == 0)
        return;

    src_node = nv_graph_get_node(g, src_id);
    dst_node = nv_graph_get_node(g, dst_id);

    if (src_node) {
        src_node->bytes_sent += pkt->size;
        src_node->pkt_count++;
    }
    if (dst_node) {
        dst_node->bytes_recv += pkt->size;
        dst_node->pkt_count++;
    }

    nv_graph_add_edge(g, src_id, dst_id);
    edge = nv_graph_get_edge(g, src_id, dst_id);
    if (edge) {
        edge->total_bytes += pkt->size;
        if (pkt->latency_ms > 0) {
            edge->avg_latency_ms = (edge->avg_latency_ms * (edge->pkt_count - 1) + pkt->latency_ms) / edge->pkt_count;
            if (pkt->latency_ms > edge->max_latency_ms)
                edge->max_latency_ms = pkt->latency_ms;
        }
        edge->congested = (edge->pkt_count > 100 && edge->avg_latency_ms > 200.0);
    }

    g->stats.active_connections = g->edge_count;
    if (pkt->status == NV_PKT_ERROR)
        g->stats.error_count++;
    if (pkt->status == NV_PKT_RETRANSMIT)
        g->stats.retransmit_count++;
}

NV_API void nv_graph_update_layout(nv_graph_t *g, float width, float height)
{
    uint32_t i, j;
    float dx, dy, dist, force, fx, fy;
    float repulsion = 5000.0f;
    float attraction = 0.01f;
    float damping = 0.9f;
    float center_x = width / 2.0f;
    float center_y = height / 2.0f;

    if (!g || g->node_count == 0)
        return;

    for (i = 0; i < g->node_count; i++) {
        if (!g->nodes[i].active)
            continue;
        fx = 0.0f;
        fy = 0.0f;

        for (j = 0; j < g->node_count; j++) {
            if (i == j || !g->nodes[j].active)
                continue;
            dx = g->nodes[i].pos_x - g->nodes[j].pos_x;
            dy = g->nodes[i].pos_y - g->nodes[j].pos_y;
            dist = sqrtf(dx * dx + dy * dy);
            if (dist < 1.0f) dist = 1.0f;
            force = repulsion / (dist * dist);
            fx += (dx / dist) * force;
            fy += (dy / dist) * force;
        }

        for (j = 0; j < g->edge_count; j++) {
            uint32_t other_id = 0;
            nv_node_t *other;
            if (g->edges[j].src_node_id == g->nodes[i].id)
                other_id = g->edges[j].dst_node_id;
            else if (g->edges[j].dst_node_id == g->nodes[i].id)
                other_id = g->edges[j].src_node_id;
            else
                continue;

            other = nv_graph_get_node(g, other_id);
            if (!other)
                continue;
            dx = other->pos_x - g->nodes[i].pos_x;
            dy = other->pos_y - g->nodes[i].pos_y;
            dist = sqrtf(dx * dx + dy * dy);
            fx += dx * attraction;
            fy += dy * attraction;
        }

        dx = center_x - g->nodes[i].pos_x;
        dy = center_y - g->nodes[i].pos_y;
        fx += dx * 0.001f;
        fy += dy * 0.001f;

        g->nodes[i].pos_x += fx * damping;
        g->nodes[i].pos_y += fy * damping;

        if (g->nodes[i].pos_x < 50.0f) g->nodes[i].pos_x = 50.0f;
        if (g->nodes[i].pos_y < 50.0f) g->nodes[i].pos_y = 50.0f;
        if (g->nodes[i].pos_x > width - 50.0f) g->nodes[i].pos_x = width - 50.0f;
        if (g->nodes[i].pos_y > height - 50.0f) g->nodes[i].pos_y = height - 50.0f;
    }
}

NV_API uint32_t nv_graph_node_count(nv_graph_t *g)
{
    return g ? g->node_count : 0;
}

NV_API uint32_t nv_graph_edge_count(nv_graph_t *g)
{
    return g ? g->edge_count : 0;
}

NV_API nv_node_t *nv_graph_get_nodes(nv_graph_t *g)
{
    return g ? g->nodes : NULL;
}

NV_API nv_edge_t *nv_graph_get_edges(nv_graph_t *g)
{
    return g ? g->edges : NULL;
}

NV_API nv_stats_t nv_graph_get_stats(nv_graph_t *g)
{
    nv_stats_t empty;
    if (!g) {
        memset(&empty, 0, sizeof(empty));
        return empty;
    }
    return g->stats;
}

NV_API void nv_graph_reset(nv_graph_t *g)
{
    if (!g)
        return;
    memset(g->nodes, 0, sizeof(g->nodes));
    memset(g->edges, 0, sizeof(g->edges));
    memset(&g->stats, 0, sizeof(g->stats));
    g->node_count = 0;
    g->edge_count = 0;
    g->next_node_id = 1;
}

NV_API void nv_graph_apply_filter(nv_graph_t *g, const nv_filter_t *filter)
{
    if (!g || !filter)
        return;
    g->active_filter = *filter;
}

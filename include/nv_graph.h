/*
 * NetVision - nv_graph.h
 * Dynamic network graph management.
 * Developer: MERO:TG@QP4RM
 */

#ifndef NV_GRAPH_H
#define NV_GRAPH_H

#include "nv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nv_graph nv_graph_t;

NV_API nv_graph_t  *nv_graph_create(void);
NV_API void         nv_graph_destroy(nv_graph_t *g);
NV_API uint32_t     nv_graph_add_node(nv_graph_t *g, const char *ip, uint16_t port, nv_node_type_t type);
NV_API nv_node_t   *nv_graph_find_node(nv_graph_t *g, const char *ip);
NV_API nv_node_t   *nv_graph_get_node(nv_graph_t *g, uint32_t id);
NV_API int          nv_graph_remove_node(nv_graph_t *g, uint32_t id);
NV_API uint32_t     nv_graph_add_edge(nv_graph_t *g, uint32_t src_id, uint32_t dst_id);
NV_API nv_edge_t   *nv_graph_get_edge(nv_graph_t *g, uint32_t src_id, uint32_t dst_id);
NV_API void         nv_graph_process_packet(nv_graph_t *g, const nv_packet_t *pkt);
NV_API void         nv_graph_update_layout(nv_graph_t *g, float width, float height);
NV_API uint32_t     nv_graph_node_count(nv_graph_t *g);
NV_API uint32_t     nv_graph_edge_count(nv_graph_t *g);
NV_API nv_node_t   *nv_graph_get_nodes(nv_graph_t *g);
NV_API nv_edge_t   *nv_graph_get_edges(nv_graph_t *g);
NV_API nv_stats_t   nv_graph_get_stats(nv_graph_t *g);
NV_API void         nv_graph_reset(nv_graph_t *g);
NV_API void         nv_graph_apply_filter(nv_graph_t *g, const nv_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif

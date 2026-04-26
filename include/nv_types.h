/*
 * NetVision - nv_types.h
 * Core type definitions for the NetVision network visualization system.
 * Developer: MERO:TG@QP4RM
 */

#ifndef NV_TYPES_H
#define NV_TYPES_H

#include <stdint.h>
#include <time.h>

#ifdef _WIN32
    #ifdef NV_BUILD_DLL
        #define NV_API __declspec(dllexport)
    #else
        #define NV_API __declspec(dllimport)
    #endif
#else
    #define NV_API
#endif

#define NV_MAX_NODES        1024
#define NV_MAX_EDGES        4096
#define NV_MAX_PACKETS      65536
#define NV_MAX_SESSIONS     2048
#define NV_MAX_IP_LEN       46
#define NV_MAX_NAME_LEN     128
#define NV_MAX_FILTER_RULES 64
#define NV_RING_BUFFER_SIZE 131072

typedef enum {
    NV_PROTO_UNKNOWN = 0,
    NV_PROTO_TCP,
    NV_PROTO_UDP,
    NV_PROTO_HTTP,
    NV_PROTO_HTTPS,
    NV_PROTO_DNS,
    NV_PROTO_ICMP,
    NV_PROTO_ARP,
    NV_PROTO_WEBSOCKET
} nv_protocol_t;

typedef enum {
    NV_NODE_CLIENT = 0,
    NV_NODE_SERVER,
    NV_NODE_API,
    NV_NODE_GATEWAY,
    NV_NODE_UNKNOWN
} nv_node_type_t;

typedef enum {
    NV_PKT_NORMAL = 0,
    NV_PKT_SLOW,
    NV_PKT_ERROR,
    NV_PKT_RETRANSMIT,
    NV_PKT_TIMEOUT
} nv_packet_status_t;

typedef enum {
    NV_SESS_ACTIVE = 0,
    NV_SESS_IDLE,
    NV_SESS_CLOSED,
    NV_SESS_ERROR
} nv_session_state_t;

typedef struct {
    char     ip[NV_MAX_IP_LEN];
    uint16_t port;
    char     name[NV_MAX_NAME_LEN];
    nv_node_type_t type;
    uint32_t id;
    float    pos_x;
    float    pos_y;
    uint64_t bytes_sent;
    uint64_t bytes_recv;
    uint32_t pkt_count;
    int      active;
} nv_node_t;

typedef struct {
    uint32_t src_node_id;
    uint32_t dst_node_id;
    uint32_t pkt_count;
    uint64_t total_bytes;
    double   avg_latency_ms;
    double   max_latency_ms;
    int      congested;
    int      active;
} nv_edge_t;

typedef struct {
    uint32_t           id;
    uint32_t           src_node_id;
    uint32_t           dst_node_id;
    nv_protocol_t      protocol;
    nv_packet_status_t status;
    uint32_t           size;
    double             latency_ms;
    double             timestamp;
    uint16_t           src_port;
    uint16_t           dst_port;
    uint8_t            flags;
    float              anim_progress;
    int                visible;
} nv_packet_t;

typedef struct {
    uint32_t          id;
    uint32_t          src_node_id;
    uint32_t          dst_node_id;
    nv_protocol_t     protocol;
    nv_session_state_t state;
    uint64_t          bytes_transferred;
    uint32_t          pkt_count;
    double            start_time;
    double            last_activity;
    double            total_latency_ms;
    uint32_t          retransmits;
    uint32_t          errors;
} nv_session_t;

typedef struct {
    int      enabled;
    char     src_ip[NV_MAX_IP_LEN];
    char     dst_ip[NV_MAX_IP_LEN];
    uint16_t port;
    nv_protocol_t protocol;
    int      show_errors_only;
} nv_filter_t;

typedef struct {
    nv_packet_t *buffer;
    uint32_t     capacity;
    uint32_t     head;
    uint32_t     tail;
    uint32_t     count;
} nv_ring_buffer_t;

typedef struct {
    double   total_bytes_per_sec;
    double   total_packets_per_sec;
    double   avg_latency_ms;
    uint32_t active_connections;
    uint32_t error_count;
    uint32_t retransmit_count;
    double   packet_loss_rate;
    double   uptime_seconds;
} nv_stats_t;

typedef struct {
    nv_packet_t *frames;
    uint32_t     frame_count;
    uint32_t     capacity;
    double       start_time;
    double       end_time;
    int          recording;
    int          playing;
    uint32_t     playback_index;
    double       playback_speed;
} nv_replay_t;

#endif

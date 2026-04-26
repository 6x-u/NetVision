/*
 * NetVision - nv_main_cli.c
 * Headless CLI entry point for Windows / non-GUI builds.
 * Developer: MERO:TG@QP4RM
 */

#include "nv_types.h"
#include "nv_capture.h"
#include "nv_graph.h"
#include "nv_session.h"
#include "nv_replay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define NV_SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#define NV_SLEEP(ms) usleep((ms) * 1000)
#endif

static const char *proto_name(nv_protocol_t p)
{
    switch (p) {
    case NV_PROTO_TCP:   return "TCP";
    case NV_PROTO_UDP:   return "UDP";
    case NV_PROTO_HTTP:  return "HTTP";
    case NV_PROTO_HTTPS: return "HTTPS";
    case NV_PROTO_DNS:   return "DNS";
    case NV_PROTO_ICMP:  return "ICMP";
    case NV_PROTO_ARP:   return "ARP";
    default:             return "UNKNOWN";
    }
}

static void print_banner(void)
{
    printf("============================================\n");
    printf("  NetVision v1.0.0 - CLI Mode\n");
    printf("  Developer: MERO:TG@QP4RM\n");
    printf("============================================\n\n");
}

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -d <device>    Capture device\n");
    printf("  -l             List available devices\n");
    printf("  -c <count>     Number of packets to capture (default: 100)\n");
    printf("  -r <file>      Load and replay .nvrp file\n");
    printf("  -h             Show this help\n");
}

static void cmd_list_devices(void)
{
    char devices[32][NV_MAX_NAME_LEN];
    int count = nv_capture_list_devices(devices, 32);

    if (count <= 0) {
        printf("No devices found (or insufficient permissions).\n");
        return;
    }

    printf("Available capture devices:\n");
    for (int i = 0; i < count; i++)
        printf("  [%d] %s\n", i, devices[i]);
}

static void cmd_capture(const char *device, int max_packets)
{
    nv_capture_ctx_t *cap;
    nv_graph_t *graph;
    nv_session_mgr_t *sess;
    nv_packet_t pkt;
    int captured = 0;

    graph = nv_graph_create();
    sess = nv_session_mgr_create();
    cap = nv_capture_create(device, 65535, 1);

    if (!cap) {
        printf("Error: Cannot create capture on device '%s'\n", device ? device : "default");
        printf("       %s\n", nv_capture_get_error(cap));
        nv_graph_destroy(graph);
        nv_session_mgr_destroy(sess);
        return;
    }

    if (nv_capture_start(cap) != 0) {
        printf("Error: Failed to start capture: %s\n", nv_capture_get_error(cap));
        nv_capture_destroy(cap);
        nv_graph_destroy(graph);
        nv_session_mgr_destroy(sess);
        return;
    }

    printf("Capturing on '%s' (max %d packets)...\n\n", device ? device : "default", max_packets);

    while (captured < max_packets && nv_capture_is_running(cap)) {
        if (nv_capture_next_packet(cap, &pkt) == 0) {
            nv_graph_process_packet(graph, &pkt);
            nv_session_track(sess, &pkt);
            captured++;

            printf("[%05d] %s  %u bytes  latency=%.1fms  status=%d\n",
                   pkt.id, proto_name(pkt.protocol), pkt.size,
                   pkt.latency_ms, pkt.status);
        } else {
            NV_SLEEP(10);
        }
    }

    nv_capture_stop(cap);

    printf("\n--- Capture Summary ---\n");
    printf("Packets captured: %d\n", captured);
    printf("Nodes discovered: %u\n", nv_graph_node_count(graph));
    printf("Edges (connections): %u\n", nv_graph_edge_count(graph));
    printf("Active sessions: %u\n", nv_session_active_count(sess));
    printf("Total sessions: %u\n", nv_session_count(sess));

    nv_stats_t stats = nv_graph_get_stats(graph);
    printf("Errors: %u\n", stats.error_count);
    printf("Retransmits: %u\n", stats.retransmit_count);

    nv_capture_destroy(cap);
    nv_graph_destroy(graph);
    nv_session_mgr_destroy(sess);
}

static void cmd_replay(const char *filepath)
{
    nv_replay_t *replay = nv_replay_create(1000000);

    if (nv_replay_load(replay, filepath) != 0) {
        printf("Error: Cannot load replay file '%s'\n", filepath);
        nv_replay_destroy(replay);
        return;
    }

    printf("Loaded replay: %u frames, %.2f seconds\n",
           nv_replay_get_frame_count(replay),
           nv_replay_get_duration(replay));

    nv_replay_destroy(replay);
}

int main(int argc, char **argv)
{
    const char *device = NULL;
    const char *replay_file = NULL;
    int max_packets = 100;
    int list_devs = 0;
    int i;

    print_banner();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
            device = argv[++i];
        else if (strcmp(argv[i], "-l") == 0)
            list_devs = 1;
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            max_packets = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc)
            replay_file = argv[++i];
        else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (list_devs) {
        cmd_list_devices();
        return 0;
    }

    if (replay_file) {
        cmd_replay(replay_file);
        return 0;
    }

    cmd_capture(device, max_packets);
    return 0;
}

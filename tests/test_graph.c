/*
 * NetVision - test_graph.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_graph.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  [TEST] %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

static void test_create_destroy(void)
{
    nv_graph_t *g;
    TEST("graph create/destroy");
    g = nv_graph_create();
    assert(g != NULL);
    assert(nv_graph_node_count(g) == 0);
    assert(nv_graph_edge_count(g) == 0);
    nv_graph_destroy(g);
    PASS();
}

static void test_add_nodes(void)
{
    nv_graph_t *g;
    uint32_t id1, id2, id3;

    TEST("add nodes");
    g = nv_graph_create();

    id1 = nv_graph_add_node(g, "192.168.1.1", 80, NV_NODE_CLIENT);
    assert(id1 > 0);
    id2 = nv_graph_add_node(g, "10.0.0.1", 443, NV_NODE_SERVER);
    assert(id2 > 0);
    id3 = nv_graph_add_node(g, "172.16.0.1", 8080, NV_NODE_API);
    assert(id3 > 0);
    assert(nv_graph_node_count(g) == 3);

    uint32_t dup = nv_graph_add_node(g, "192.168.1.1", 80, NV_NODE_CLIENT);
    assert(dup == id1);
    assert(nv_graph_node_count(g) == 3);

    nv_graph_destroy(g);
    PASS();
}

static void test_find_node(void)
{
    nv_graph_t *g;
    nv_node_t *node;

    TEST("find node by IP");
    g = nv_graph_create();
    nv_graph_add_node(g, "192.168.1.100", 8080, NV_NODE_SERVER);

    node = nv_graph_find_node(g, "192.168.1.100");
    assert(node != NULL);
    assert(strcmp(node->ip, "192.168.1.100") == 0);
    assert(node->type == NV_NODE_SERVER);

    node = nv_graph_find_node(g, "1.2.3.4");
    assert(node == NULL);

    nv_graph_destroy(g);
    PASS();
}

static void test_edges(void)
{
    nv_graph_t *g;
    uint32_t id1, id2;
    nv_edge_t *edge;

    TEST("add/find edges");
    g = nv_graph_create();
    id1 = nv_graph_add_node(g, "192.168.1.1", 80, NV_NODE_CLIENT);
    id2 = nv_graph_add_node(g, "10.0.0.1", 443, NV_NODE_SERVER);

    nv_graph_add_edge(g, id1, id2);
    assert(nv_graph_edge_count(g) == 1);

    edge = nv_graph_get_edge(g, id1, id2);
    assert(edge != NULL);
    assert(edge->src_node_id == id1);
    assert(edge->dst_node_id == id2);

    edge = nv_graph_get_edge(g, id2, id1);
    assert(edge != NULL);

    nv_graph_destroy(g);
    PASS();
}

static void test_process_packet(void)
{
    nv_graph_t *g;
    nv_packet_t pkt;

    TEST("process packet");
    g = nv_graph_create();

    memset(&pkt, 0, sizeof(pkt));
    pkt.src_node_id = 0xC0A80101;
    pkt.dst_node_id = 0x0A000001;
    pkt.src_port = 12345;
    pkt.dst_port = 80;
    pkt.protocol = NV_PROTO_HTTP;
    pkt.size = 1024;
    pkt.status = NV_PKT_NORMAL;
    pkt.latency_ms = 15.5;

    nv_graph_process_packet(g, &pkt);
    assert(nv_graph_node_count(g) == 2);
    assert(nv_graph_edge_count(g) == 1);

    nv_stats_t stats = nv_graph_get_stats(g);
    assert(stats.active_connections == 1);

    nv_graph_destroy(g);
    PASS();
}

static void test_layout(void)
{
    nv_graph_t *g;

    TEST("force-directed layout");
    g = nv_graph_create();
    nv_graph_add_node(g, "1.1.1.1", 80, NV_NODE_CLIENT);
    nv_graph_add_node(g, "2.2.2.2", 443, NV_NODE_SERVER);
    nv_graph_update_layout(g, 1280.0f, 720.0f);

    nv_node_t *nodes = nv_graph_get_nodes(g);
    assert(nodes[0].pos_x >= 50.0f && nodes[0].pos_x <= 1230.0f);
    assert(nodes[0].pos_y >= 50.0f && nodes[0].pos_y <= 670.0f);

    nv_graph_destroy(g);
    PASS();
}

static void test_reset(void)
{
    nv_graph_t *g;

    TEST("graph reset");
    g = nv_graph_create();
    nv_graph_add_node(g, "1.1.1.1", 80, NV_NODE_CLIENT);
    nv_graph_add_node(g, "2.2.2.2", 443, NV_NODE_SERVER);
    assert(nv_graph_node_count(g) == 2);

    nv_graph_reset(g);
    assert(nv_graph_node_count(g) == 0);
    assert(nv_graph_edge_count(g) == 0);

    nv_graph_destroy(g);
    PASS();
}

int main(void)
{
    printf("NetVision Graph Tests\n");
    printf("=====================\n");

    test_create_destroy();
    test_add_nodes();
    test_find_node();
    test_edges();
    test_process_packet();
    test_layout();
    test_reset();

    printf("\nResults: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

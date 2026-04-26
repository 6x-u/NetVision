/*
 * NetVision - test_session.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_session.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  [TEST] %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)

static void test_create_destroy(void)
{
    nv_session_mgr_t *mgr;
    TEST("session mgr create/destroy");
    mgr = nv_session_mgr_create();
    assert(mgr != NULL);
    assert(nv_session_count(mgr) == 0);
    nv_session_mgr_destroy(mgr);
    PASS();
}

static void test_track_session(void)
{
    nv_session_mgr_t *mgr;
    nv_packet_t pkt;
    uint32_t sid;

    TEST("track session");
    mgr = nv_session_mgr_create();

    memset(&pkt, 0, sizeof(pkt));
    pkt.src_node_id = 100;
    pkt.dst_node_id = 200;
    pkt.protocol = NV_PROTO_TCP;
    pkt.size = 512;
    pkt.latency_ms = 10.0;

    sid = nv_session_track(mgr, &pkt);
    assert(sid > 0);
    assert(nv_session_count(mgr) == 1);
    assert(nv_session_active_count(mgr) == 1);

    pkt.size = 256;
    pkt.latency_ms = 5.0;
    uint32_t sid2 = nv_session_track(mgr, &pkt);
    assert(sid2 == sid);
    assert(nv_session_count(mgr) == 1);

    nv_session_t *sess = nv_session_get(mgr, sid);
    assert(sess != NULL);
    assert(sess->pkt_count == 2);
    assert(sess->bytes_transferred == 768);

    nv_session_mgr_destroy(mgr);
    PASS();
}

static void test_multiple_sessions(void)
{
    nv_session_mgr_t *mgr;
    nv_packet_t pkt1, pkt2;

    TEST("multiple sessions");
    mgr = nv_session_mgr_create();

    memset(&pkt1, 0, sizeof(pkt1));
    pkt1.src_node_id = 100;
    pkt1.dst_node_id = 200;
    pkt1.protocol = NV_PROTO_TCP;
    pkt1.size = 100;

    memset(&pkt2, 0, sizeof(pkt2));
    pkt2.src_node_id = 300;
    pkt2.dst_node_id = 400;
    pkt2.protocol = NV_PROTO_UDP;
    pkt2.size = 200;

    nv_session_track(mgr, &pkt1);
    nv_session_track(mgr, &pkt2);
    assert(nv_session_count(mgr) == 2);

    nv_session_mgr_destroy(mgr);
    PASS();
}

static void test_close_session(void)
{
    nv_session_mgr_t *mgr;
    nv_packet_t pkt;

    TEST("close session");
    mgr = nv_session_mgr_create();

    memset(&pkt, 0, sizeof(pkt));
    pkt.src_node_id = 100;
    pkt.dst_node_id = 200;
    pkt.protocol = NV_PROTO_TCP;
    pkt.size = 100;

    uint32_t sid = nv_session_track(mgr, &pkt);
    assert(nv_session_active_count(mgr) == 1);

    nv_session_close(mgr, sid);
    assert(nv_session_active_count(mgr) == 0);

    nv_session_t *sess = nv_session_get(mgr, sid);
    assert(sess != NULL);
    assert(sess->state == NV_SESS_CLOSED);

    nv_session_mgr_destroy(mgr);
    PASS();
}

static void test_error_tracking(void)
{
    nv_session_mgr_t *mgr;
    nv_packet_t pkt;

    TEST("error and retransmit tracking");
    mgr = nv_session_mgr_create();

    memset(&pkt, 0, sizeof(pkt));
    pkt.src_node_id = 100;
    pkt.dst_node_id = 200;
    pkt.protocol = NV_PROTO_TCP;
    pkt.size = 100;
    pkt.status = NV_PKT_NORMAL;

    nv_session_track(mgr, &pkt);

    pkt.status = NV_PKT_ERROR;
    nv_session_track(mgr, &pkt);

    pkt.status = NV_PKT_RETRANSMIT;
    nv_session_track(mgr, &pkt);

    nv_session_t *all = nv_session_get_all(mgr);
    assert(all[0].errors == 1);
    assert(all[0].retransmits == 1);
    assert(all[0].pkt_count == 3);

    nv_session_mgr_destroy(mgr);
    PASS();
}

int main(void)
{
    printf("NetVision Session Tests\n");
    printf("=======================\n");

    test_create_destroy();
    test_track_session();
    test_multiple_sessions();
    test_close_session();
    test_error_tracking();

    printf("\nResults: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

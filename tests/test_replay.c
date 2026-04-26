/*
 * NetVision - test_replay.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_replay.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  [TEST] %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)

static void test_create_destroy(void)
{
    nv_replay_t *r;
    TEST("replay create/destroy");
    r = nv_replay_create(1000);
    assert(r != NULL);
    assert(nv_replay_get_frame_count(r) == 0);
    assert(!nv_replay_is_recording(r));
    assert(!nv_replay_is_playing(r));
    nv_replay_destroy(r);
    PASS();
}

static void test_record(void)
{
    nv_replay_t *r;
    nv_packet_t pkt;

    TEST("record packets");
    r = nv_replay_create(1000);

    assert(nv_replay_start_recording(r) == 0);
    assert(nv_replay_is_recording(r));

    memset(&pkt, 0, sizeof(pkt));
    pkt.id = 1;
    pkt.size = 100;
    pkt.timestamp = 1000.0;
    assert(nv_replay_record_packet(r, &pkt) == 0);

    pkt.id = 2;
    pkt.timestamp = 1001.0;
    assert(nv_replay_record_packet(r, &pkt) == 0);

    pkt.id = 3;
    pkt.timestamp = 1002.5;
    assert(nv_replay_record_packet(r, &pkt) == 0);

    assert(nv_replay_stop_recording(r) == 0);
    assert(!nv_replay_is_recording(r));
    assert(nv_replay_get_frame_count(r) == 3);

    double dur = nv_replay_get_duration(r);
    assert(dur > 2.0 && dur < 3.0);

    nv_replay_destroy(r);
    PASS();
}

static void test_save_load(void)
{
    nv_replay_t *r;
    nv_replay_t *r2;
    nv_packet_t pkt;
    const char *path = "/tmp/nv_test_replay.nvrp";

    TEST("save and load");
    r = nv_replay_create(1000);
    nv_replay_start_recording(r);

    memset(&pkt, 0, sizeof(pkt));
    for (int i = 0; i < 50; i++) {
        pkt.id = i + 1;
        pkt.size = 100 + i;
        pkt.timestamp = 1000.0 + i * 0.1;
        nv_replay_record_packet(r, &pkt);
    }
    nv_replay_stop_recording(r);

    assert(nv_replay_save(r, path) == 0);

    r2 = nv_replay_create(100);
    assert(nv_replay_load(r2, path) == 0);
    assert(nv_replay_get_frame_count(r2) == 50);

    nv_replay_destroy(r);
    nv_replay_destroy(r2);
    unlink(path);
    PASS();
}

static void test_playback(void)
{
    nv_replay_t *r;
    nv_packet_t pkt;

    TEST("playback");
    r = nv_replay_create(1000);
    nv_replay_start_recording(r);

    memset(&pkt, 0, sizeof(pkt));
    pkt.id = 1;
    pkt.timestamp = 100.0;
    nv_replay_record_packet(r, &pkt);

    pkt.id = 2;
    pkt.timestamp = 101.0;
    nv_replay_record_packet(r, &pkt);

    nv_replay_stop_recording(r);

    assert(nv_replay_start_playback(r, 1.0) == 0);
    assert(nv_replay_is_playing(r));

    nv_replay_stop_playback(r);
    assert(!nv_replay_is_playing(r));

    nv_replay_destroy(r);
    PASS();
}

static void test_speed(void)
{
    nv_replay_t *r;

    TEST("speed control");
    r = nv_replay_create(100);

    nv_replay_set_speed(r, 2.0);
    nv_replay_set_speed(r, 0.5);
    nv_replay_set_speed(r, 0.0);

    nv_replay_destroy(r);
    PASS();
}

int main(void)
{
    printf("NetVision Replay Tests\n");
    printf("======================\n");

    test_create_destroy();
    test_record();
    test_save_load();
    test_playback();
    test_speed();

    printf("\nResults: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

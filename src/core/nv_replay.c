/*
 * NetVision - nv_replay.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_replay.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

static double nv_replay_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

NV_API nv_replay_t *nv_replay_create(uint32_t capacity)
{
    nv_replay_t *r = (nv_replay_t *)calloc(1, sizeof(nv_replay_t));
    if (!r)
        return NULL;

    if (capacity == 0)
        capacity = 1000000;

    r->frames = (nv_packet_t *)calloc(capacity, sizeof(nv_packet_t));
    if (!r->frames) {
        free(r);
        return NULL;
    }
    r->capacity = capacity;
    r->playback_speed = 1.0;
    return r;
}

NV_API void nv_replay_destroy(nv_replay_t *r)
{
    if (!r)
        return;
    if (r->frames)
        free(r->frames);
    free(r);
}

NV_API int nv_replay_start_recording(nv_replay_t *r)
{
    if (!r || r->recording)
        return -1;
    r->recording = 1;
    r->frame_count = 0;
    r->start_time = nv_replay_time();
    return 0;
}

NV_API int nv_replay_stop_recording(nv_replay_t *r)
{
    if (!r || !r->recording)
        return -1;
    r->recording = 0;
    r->end_time = nv_replay_time();
    return 0;
}

NV_API int nv_replay_record_packet(nv_replay_t *r, const nv_packet_t *pkt)
{
    if (!r || !pkt || !r->recording)
        return -1;
    if (r->frame_count >= r->capacity)
        return -1;
    r->frames[r->frame_count] = *pkt;
    r->frame_count++;
    return 0;
}

NV_API int nv_replay_start_playback(nv_replay_t *r, double speed)
{
    if (!r || r->frame_count == 0)
        return -1;
    r->playing = 1;
    r->playback_index = 0;
    r->playback_speed = (speed > 0.0) ? speed : 1.0;
    r->start_time = nv_replay_time();
    return 0;
}

NV_API int nv_replay_stop_playback(nv_replay_t *r)
{
    if (!r)
        return -1;
    r->playing = 0;
    r->playback_index = 0;
    return 0;
}

NV_API nv_packet_t *nv_replay_next_frame(nv_replay_t *r, double current_time)
{
    double elapsed, frame_time;

    if (!r || !r->playing || r->playback_index >= r->frame_count)
        return NULL;

    elapsed = (current_time - r->start_time) * r->playback_speed;
    frame_time = r->frames[r->playback_index].timestamp - r->frames[0].timestamp;

    if (elapsed >= frame_time) {
        nv_packet_t *frame = &r->frames[r->playback_index];
        r->playback_index++;
        if (r->playback_index >= r->frame_count)
            r->playing = 0;
        return frame;
    }

    return NULL;
}

NV_API int nv_replay_save(nv_replay_t *r, const char *filepath)
{
    FILE *f;
    uint32_t magic = 0x4E565250;
    uint32_t version = 1;

    if (!r || !filepath || r->frame_count == 0)
        return -1;

    f = fopen(filepath, "wb");
    if (!f)
        return -1;

    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&r->frame_count, sizeof(r->frame_count), 1, f);
    fwrite(&r->start_time, sizeof(r->start_time), 1, f);
    fwrite(&r->end_time, sizeof(r->end_time), 1, f);
    fwrite(r->frames, sizeof(nv_packet_t), r->frame_count, f);

    fclose(f);
    return 0;
}

NV_API int nv_replay_load(nv_replay_t *r, const char *filepath)
{
    FILE *f;
    uint32_t magic, version, count;

    if (!r || !filepath)
        return -1;

    f = fopen(filepath, "rb");
    if (!f)
        return -1;

    if (fread(&magic, sizeof(magic), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    if (magic != 0x4E565250) {
        fclose(f);
        return -1;
    }

    if (fread(&version, sizeof(version), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (count > r->capacity) {
        nv_packet_t *tmp = (nv_packet_t *)realloc(r->frames, count * sizeof(nv_packet_t));
        if (!tmp) {
            fclose(f);
            return -1;
        }
        r->frames = tmp;
        r->capacity = count;
    }

    if (fread(&r->start_time, sizeof(r->start_time), 1, f) != 1 ||
        fread(&r->end_time, sizeof(r->end_time), 1, f) != 1 ||
        fread(r->frames, sizeof(nv_packet_t), count, f) != count) {
        fclose(f);
        return -1;
    }
    r->frame_count = count;

    fclose(f);
    return 0;
}

NV_API double nv_replay_get_duration(nv_replay_t *r)
{
    if (!r || r->frame_count < 2)
        return 0.0;
    return r->frames[r->frame_count - 1].timestamp - r->frames[0].timestamp;
}

NV_API uint32_t nv_replay_get_frame_count(nv_replay_t *r)
{
    return r ? r->frame_count : 0;
}

NV_API void nv_replay_set_speed(nv_replay_t *r, double speed)
{
    if (r && speed > 0.0)
        r->playback_speed = speed;
}

NV_API int nv_replay_is_recording(nv_replay_t *r)
{
    return r ? r->recording : 0;
}

NV_API int nv_replay_is_playing(nv_replay_t *r)
{
    return r ? r->playing : 0;
}

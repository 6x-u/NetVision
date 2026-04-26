/*
 * NetVision - nv_replay.h
 * Session recording and replay system.
 * Developer: MERO:TG@QP4RM
 */

#ifndef NV_REPLAY_H
#define NV_REPLAY_H

#include "nv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

NV_API nv_replay_t *nv_replay_create(uint32_t capacity);
NV_API void         nv_replay_destroy(nv_replay_t *r);
NV_API int          nv_replay_start_recording(nv_replay_t *r);
NV_API int          nv_replay_stop_recording(nv_replay_t *r);
NV_API int          nv_replay_record_packet(nv_replay_t *r, const nv_packet_t *pkt);
NV_API int          nv_replay_start_playback(nv_replay_t *r, double speed);
NV_API int          nv_replay_stop_playback(nv_replay_t *r);
NV_API nv_packet_t *nv_replay_next_frame(nv_replay_t *r, double current_time);
NV_API int          nv_replay_save(nv_replay_t *r, const char *filepath);
NV_API int          nv_replay_load(nv_replay_t *r, const char *filepath);
NV_API double       nv_replay_get_duration(nv_replay_t *r);
NV_API uint32_t     nv_replay_get_frame_count(nv_replay_t *r);
NV_API void         nv_replay_set_speed(nv_replay_t *r, double speed);
NV_API int          nv_replay_is_recording(nv_replay_t *r);
NV_API int          nv_replay_is_playing(nv_replay_t *r);

#ifdef __cplusplus
}
#endif

#endif

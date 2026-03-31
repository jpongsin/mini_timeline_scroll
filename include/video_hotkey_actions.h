//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info


#ifndef TIMELINE_SCRUB_QT_VIDEO_HOTKEY_ACTIONS_H
#define TIMELINE_SCRUB_QT_VIDEO_HOTKEY_ACTIONS_H

#include "video_implement.h"

#ifdef __cplusplus
extern "C" {
#endif
void toggle_playback(const VideoPlayer *player);
void toggle_mute(const VideoPlayer *player);
gboolean start_playback(VideoPlayer *player);
void seek_frames(VideoPlayer *player, gint64 direction);
void seek_begin(const VideoPlayer *player, gint64 pos_ns);
void change_rate(VideoPlayer *player, gdouble new_rate);
void change_volume(const VideoPlayer *player, gdouble direction);
gboolean pipeline_is_active(const VideoPlayer *player);
#ifdef __cplusplus
}
#endif
#endif //TIMELINE_SCRUB_QT_VIDEO_HOTKEY_ACTIONS_H

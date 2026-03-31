//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info


#include "../include/video_hotkey_actions.h"
//reverts back to the beginning of a video
void seek_begin(const VideoPlayer *player, const gint64 pos_ns) {
    if (!pipeline_is_active(player)) return;
    gst_element_seek_simple(player->pipeline,
                            GST_FORMAT_TIME,
                            GST_SEEK_FLAG_FLUSH |
                            GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER,
                            pos_ns);
}

//starts playback
gboolean start_playback(VideoPlayer *player) {
    if (!pipeline_is_active(player)) return FALSE;
    //check if the state fails
    if (gst_element_set_state(player->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline failed\n");

        //clean up from mem
        gst_element_set_state(player->pipeline, GST_STATE_NULL);
        gst_object_unref(player->pipeline);
        player->pipeline = NULL;
    }
    return G_SOURCE_REMOVE;
}

//video playback speed
void change_rate(VideoPlayer *player, gdouble direction) {
    if (!pipeline_is_active(player)) return;
    //position
    gint64 pos;

    //exit if query not found?
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &pos)) return;

    //add to the current rate from typedef struct
    player->current_rate += direction;

    // set maximum speed at 2.0 and minimum speed at 0.25
    if (player->current_rate < 0.25) player->current_rate = 0.25;
    if (player->current_rate > 2.0) player->current_rate = 2.0;

    //print on CLI
    g_print("\rNew Speed: %.2f (%s)",
            player->current_rate,
            player->current_rate == 1.0 ? "Normal" : player->current_rate > 1.0 ? "Fast" : "Slow");

    // call gst_element_seek to establish the new current rate
    // ACCURATE seek is good enough
    // TRICKMODE might be handy for 4K and up
    gst_element_seek(player->pipeline, player->current_rate,
                     GST_FORMAT_TIME,
                     GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                     GST_SEEK_TYPE_SET, pos,
                     GST_SEEK_TYPE_NONE, 0);
}

//changes the volume, changes attributes to typedef struct VideoPlayer
void change_volume(const VideoPlayer *player, gdouble direction) {
    if (!pipeline_is_active(player)) return;
    //get current volume
    gdouble current_vol;
    g_object_get(player->volume_stream, "volume", &current_vol, NULL);

    //add or decrease volume
    current_vol += direction;

    //set maximum volume at 200, minimum volume at 0
    if (current_vol < 0.0) current_vol = 0.0;
    if (current_vol > 2.0) current_vol = 2.0;

    //set the new volume
    g_object_set(player->volume_stream, "volume", current_vol, NULL);

    //print volume
    g_print("\rVolume: %.0f%%", current_vol * 100);
}

void seek_frames(VideoPlayer *player, gint64 direct_count) {
    if (!pipeline_is_active(player)) return;
    gint64 current_pos;
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &current_pos)) return;

    double fps = player->assignedFPS;
    // calculate duration as a double to avoid losing decimals
    double frame_dur_d = (double)GST_SECOND / fps;

    // calculate current frame and target frame for accuracy
    gint64 current_frame = (gint64)round((double)current_pos / frame_dur_d);
    gint64 target_frame = current_frame + direct_count;
    if (target_frame < 0) target_frame = 0;

    //from target frame, calculate nanoseconds
    gint64 target_ns = (gint64)(target_frame * frame_dur_d);

    //assign target frame to display frame
    gint64 display_frame = target_frame;
    char separator = ':';

    if (fps > 29.9 && fps < 30.0) {
        separator = ';';
        gint64 d = display_frame / 17982;
        gint64 m = display_frame % 17982;
        display_frame += (18 * d) + 2 * ((m - 2) / 1798);
    } else if (fps > 59.9 && fps < 60.0) {
        separator = ';';
        gint64 d = display_frame / 35964;
        gint64 m = display_frame % 35964;
        display_frame += (36 * d) + 4 * ((m - 4) / 1798);
    }

    //format for cli display
    int fps_int = (int)(fps + 0.5);
    guint f = (guint)(display_frame % fps_int);
    guint s = (guint)((display_frame / fps_int) % 60);
    guint m = (guint)((display_frame / (fps_int * 60)) % 60);
    guint h = (guint)(display_frame / (fps_int * 3600));

    g_print("\r FPS: %.2f | Jump: %ld frames | Seeking to: %02u:%02u:%02u%c%02u",
            fps, (long)direct_count, h, m, s, separator, f);
    fflush(stdout);
    
    //then, treat the target frame as if it was the beginning and let it run
    gst_element_seek(player->pipeline, 1.0, GST_FORMAT_TIME,
                     GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                     GST_SEEK_TYPE_SET, target_ns,
                     GST_SEEK_TYPE_NONE, -1);
}


// check if pipeline is active. check if state is playing, then pause, else: play.
void toggle_playback(const VideoPlayer *player) {
    if (!pipeline_is_active(player)) return;
    GstState current, pending;
    gst_element_get_state(player->pipeline, &current, &pending, 0);
    if (current == GST_STATE_PLAYING || pending == GST_STATE_PLAYING) {
        gst_element_set_state(player->pipeline, GST_STATE_PAUSED);
        gst_video_overlay_expose(GST_VIDEO_OVERLAY(player->video_sink));
        g_print("\rPaused");
    } else {
        gst_element_set_state(player->pipeline, GST_STATE_PLAYING);
        g_print("\rPlaying");
    }
    fflush(stdout);
}

void toggle_mute(const VideoPlayer *player) {
    if (!pipeline_is_active(player)) return;
    gboolean muted;
    g_object_get(player->volume_stream, "mute", &muted, NULL);

    gboolean next_state = !muted;
    g_object_set(player->volume_stream, "mute", next_state, NULL);

    // Using \r and spaces to overwrite the previous line
    g_print("\rStatus: %s          ", next_state ? "Muted  " : "Unmuted");
    fflush(stdout);
}
// returns TRUE if pipeline is ready for hotkey actions
gboolean pipeline_is_active(const VideoPlayer *player) {
    return player->pipeline != NULL
        && player->assignedFPS > 0.0
        && player->volume_stream != NULL;
}

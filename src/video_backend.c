// mini_timeline_scroll
//(c) 2026 jpongsin
// Licensed under MIT License
// See README.md and LICENSE.md for more info

#include "../include/video_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void configure_mpv_handle(mpv_handle *handle) {
  //cache
  mpv_set_option_string(handle, "demuxer-max-bytes", "5%");
  mpv_set_option_string(handle, "demuxer-max-back-bytes", "2%");

  //seeks
  mpv_set_option_string(handle, "hr-seek", "yes");
  mpv_set_option_string(handle, "hr-seek-framedrop", "no");
  mpv_set_option_string(handle, "vd-lavc-threads", "0");

  //rendering
  mpv_set_option_string(handle, "hwdec", "auto-safe");
  mpv_set_option_string(handle, "vo", "libmpv");
  mpv_set_option_string(handle, "vd-lavc-dr", "no");
  mpv_set_option_string(handle, "video-sync", "display-resample");
  mpv_set_option_string(handle, "interpolation", "yes");

  //
  mpv_set_option_string(handle, "video-timing-offset", "0");
  mpv_set_option_string(handle, "config", "no"); // Prevent user input.conf fighting

  //other settings
  mpv_set_option_string(handle, "keep-open", "yes"); //restarting playback is nicer.
  mpv_set_option_string(handle, "audio-buffer", "0.2");
  mpv_set_option_string(handle, "idle", "yes");
  mpv_set_option_string(handle, "pause", "yes");

  //debug
  mpv_set_option_string(handle, "terminal", "no");
  mpv_set_option_string(handle, "msg-level", "all=warn");
}

//finetune mpv
mpv_handle *init_backend(void) {
  mpv_handle *handle = mpv_create();
  if (!handle) return NULL;

  configure_mpv_handle(handle);

  if (mpv_initialize(handle) < 0) {
    mpv_terminate_destroy(handle);
    return NULL;
  }
  return handle;
}

//literally loads file
void load_file(mpv_handle *handle, const char *filename) {
  const char *cmd[] = {"loadfile", filename, NULL};
  mpv_command(handle, cmd);
}

//pause video directly
void toggle_pause(mpv_handle *handle) {
  int pause = 0;
  mpv_get_property(handle, "pause", MPV_FORMAT_FLAG, &pause);
  pause = !pause;
  mpv_set_property(handle, "pause", MPV_FORMAT_FLAG, &pause);
}

//seek frames or seconds
void seek_absolute(mpv_handle *handle, double seconds, int exact) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%f", seconds);
  const char *cmd[] = {"seek", buf, "absolute", exact ? "exact" : "keyframes",
                       NULL};
  mpv_command(handle, cmd);
}

//get video metadata
void get_video_metadata(mpv_node track, char **type, int *id, char **lang, char **title_str) {
  for (int j = 0; j < track.u.list->num; j++) {
    if (strcmp(track.u.list->keys[j], "type") == 0)
      {*type = track.u.list->values[j].u.string;}
    if (strcmp(track.u.list->keys[j], "id") == 0)
      {*id = track.u.list->values[j].u.int64;}
    if (strcmp(track.u.list->keys[j], "lang") == 0)
      {*lang = track.u.list->values[j].u.string;}
    if (strcmp(track.u.list->keys[j], "title") == 0)
      {*title_str = track.u.list->values[j].u.string;}
  }
}

//get video dimensions
void get_video_dimensions(VideoState *vs, mpv_node node) {
  if (node.format == MPV_FORMAT_NODE_MAP) {
    for (int i = 0; i < node.u.list->num; i++) {
      if (strcmp(node.u.list->keys[i], "w") == 0)
        {vs->width = node.u.list->values[i].u.int64;}
      if (strcmp(node.u.list->keys[i], "h") == 0)
        {vs->height = node.u.list->values[i].u.int64;}
    }
  }
}

//get audio metadata
void get_audio_metadata(VideoState *vs, char *type, int id, char *lang, char *title_str) {
  if (type && strcmp(type, "audio") == 0 && vs->nb_audio_tracks < 16) {
    vs->audio_tracks[vs->nb_audio_tracks].index = id;
    if (lang)
      strncpy(vs->audio_tracks[vs->nb_audio_tracks].language, lang, 15);
    if (title_str)
      strncpy(vs->audio_tracks[vs->nb_audio_tracks].title, title_str,
              127);
    vs->nb_audio_tracks++;
  }
}

void get_stream_metadata(VideoState *vs, mpv_node track) {
  if (track.format == MPV_FORMAT_NODE_MAP) {
    char *type = NULL;
    int id = 0;
    char *lang = NULL;
    char *title_str = NULL;

    //get video type, id, language, title
    get_video_metadata(track, &type, &id, &lang, &title_str);

    //get audio track
    get_audio_metadata(vs, type, id, lang, title_str);
  }
}

void get_multitrack_metadata(VideoState *vs, mpv_node node) {
  for (int i = 0; i < node.u.list->num; i++) {
    mpv_node track = node.u.list->values[i];
    get_stream_metadata(vs, track);
  }
}

//get overall metadata
void get_overall_metadata(VideoState *vs, mpv_node node) {
  if (node.format == MPV_FORMAT_NODE_ARRAY) {
    vs->nb_audio_tracks = 0;
    //audio tracks....
    get_multitrack_metadata(vs, node);
  }
}

VideoState get_metadata(mpv_handle *handle) {
  VideoState vs = {0};

  //get duration and fps
  mpv_get_property(handle, "duration", MPV_FORMAT_DOUBLE, &vs.duration);
  mpv_get_property(handle, "container-fps", MPV_FORMAT_DOUBLE, &vs.fps);

  //get dimensions from video-out-params
  mpv_node node;
  if (mpv_get_property(handle, "video-out-params", MPV_FORMAT_NODE, &node) >=
      0) {
    get_video_dimensions(&vs, node);
    mpv_free_node_contents(&node);
  }

  //get title
  char *title = mpv_get_property_string(handle, "media-title");
  if (title) {
    strncpy(vs.filename, title, sizeof(vs.filename) - 1);
    mpv_free(title);
  }

  //get codec
  char *codec = mpv_get_property_string(handle, "video-codec");
  if (codec) {
    strncpy(vs.codec_name, codec, sizeof(vs.codec_name) - 1);
    mpv_free(codec);
  }

  int hw = 0;
  mpv_get_property(handle, "hwdec-current", MPV_FORMAT_FLAG, &hw);
  vs.hw_accel_used = hw;

  // Track Extraction
  if (mpv_get_property(handle, "track-list", MPV_FORMAT_NODE, &node) >= 0) {
    get_overall_metadata(&vs, node);
    mpv_free_node_contents(&node);
  }

  return vs;
}

//handle audio track
void set_audio_track(mpv_handle *handle, int track_id) {
  int64_t id = track_id;
  mpv_set_property(handle, "aid", MPV_FORMAT_INT64, &id);
}

//zoom video
void set_zoom(mpv_handle *handle, double zoom) {
  mpv_set_property(handle, "video-zoom", MPV_FORMAT_DOUBLE, &zoom);
}

//pan video
void set_pan(mpv_handle *handle, double x, double y) {
  mpv_set_property(handle, "video-pan-x", MPV_FORMAT_DOUBLE, &x);
  mpv_set_property(handle, "video-pan-y", MPV_FORMAT_DOUBLE, &y);
}

//destroy video, for closing
void cleanup_backend(mpv_handle *handle) {
  if (handle)
    mpv_terminate_destroy(handle);
}

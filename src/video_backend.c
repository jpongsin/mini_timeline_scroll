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
void get_video_metadata(mpv_node *track, char **type, int *id, char **lang, char **title_str,int *external) {
  if (!track||track->format!= MPV_FORMAT_NODE_MAP||!track->u.list) return;

  for (int j = 0; j < track->u.list->num; j++) {
    char *tempKey=track->u.list->keys[j];
    mpv_node *tempVal=&track->u.list->values[j];

    if (strcmp(tempKey, "type") == 0
      && tempVal->format == MPV_FORMAT_STRING) {
      *type = tempVal->u.string;
    }
    else if (strcmp(tempKey, "id") == 0
      && tempVal->format == MPV_FORMAT_INT64) {
      *id = (int)tempVal->u.int64;
    }
    else if (strcmp(tempKey, "lang") == 0
      && tempVal->format == MPV_FORMAT_STRING) {
      *lang = tempVal->u.string;
    }
    else if (strcmp(tempKey, "title") == 0
      && tempVal->format == MPV_FORMAT_STRING) {
      *title_str = tempVal->u.string;
    }
    else if (strcmp(tempKey, "external") == 0 && tempVal->format == MPV_FORMAT_FLAG)
    {*external = tempVal->u.flag;}
  }
}

//get video dimensions
void get_video_dimensions(VideoState *vs, mpv_node node) {
  if (node.format != MPV_FORMAT_NODE_MAP || !node.u.list) return;

  for (int i = 0; i < node.u.list->num; i++) {
    const char *tempKey = node.u.list->keys[i];
    mpv_node *tempVal = &node.u.list->values[i];

    //width
    if (strcmp(tempKey, "w") == 0) {
      if (node.u.list->values[i].format == MPV_FORMAT_INT64) {
        vs->width = (int)tempVal->u.int64;
      } else if (node.u.list->values[i].format == MPV_FORMAT_DOUBLE) {
        vs->width = (int)tempVal->u.double_;
      }
    }

    //height
    if (strcmp(tempKey, "h") == 0) {
      if (node.u.list->values[i].format == MPV_FORMAT_INT64) {
        vs->height = (int)tempVal->u.int64;
      } else if (node.u.list->values[i].format == MPV_FORMAT_DOUBLE) {
        vs->height = (int)tempVal->u.double_;
      }
    }
  }
}

//get audio metadata
void get_audio_metadata(VideoState *vs, char *type, int id, char *lang, char *title_str) {
  if (!vs || !type || strcmp(type, "audio") != 0 || vs->nb_audio_tracks >= 16) return;

  AudioTrack *tempTrack = &vs->audio_tracks[vs->nb_audio_tracks];
  tempTrack->index = id;

  if (lang)
    snprintf(tempTrack->language, sizeof(tempTrack->language), "%s", lang);
  if (title_str)
    snprintf(tempTrack->title, sizeof(tempTrack->title), "%s", title_str);
  vs->nb_audio_tracks++;
}

//get subtitle metadata
void get_subtitle_metadata(VideoState *vs, char *type, int id, char *lang, char *title_str, int external) {
  if (!vs || !type || strcmp(type, "sub") != 0 || vs->nb_subtitle_tracks >= 16) return;

  SubtitleTrack *tempTrack = &vs->subtitle_tracks[vs->nb_subtitle_tracks];
  tempTrack->index = id;
  tempTrack->is_external =external;

  if (lang)
   {snprintf(tempTrack->language, sizeof(tempTrack->language), "%s", lang);}
  else {
    tempTrack->language[0] = '\0';
  }
  if (title_str)
  {  snprintf(tempTrack->title, sizeof(tempTrack->title), "%s", title_str);}
  else {
    tempTrack->title[0]= '\0';
  }
  vs->nb_subtitle_tracks++;
}

void get_stream_metadata(VideoState *vs, mpv_node *track) {
  if (!track || track->format != MPV_FORMAT_NODE_MAP) return;
  char *type = NULL;
  int id = 0;
  char *lang = NULL;
  char *title_str = NULL;
  int is_external = 0;

  //get video type, id, language, title
  get_video_metadata(track, &type, &id, &lang, &title_str, &is_external);

  //get audio track
  if (type&&strcmp(type,"audio")==0) {
    get_audio_metadata(vs, type, id, lang, title_str);
  }

  //get subtitles
  else if (type&&strcmp(type,"sub")==0) {
    get_subtitle_metadata(vs, type, id, lang, title_str, is_external);
  }

}

void get_multitrack_metadata(VideoState *vs, mpv_node node) {
  if (node.format != MPV_FORMAT_NODE_ARRAY || !node.u.list) return;
  vs->nb_audio_tracks = 0;

  for (int i = 0; i < node.u.list->num; i++) {
    mpv_node *track = &node.u.list->values[i];

    get_stream_metadata(vs, track);
  }
}

void set_subtitle_track(mpv_handle *handle, int id) {
  if (id <= 0) {
    // Turning subtitles off
    mpv_set_property_string(handle, "sid", "no");
  } else {
    // mpv expects the ID as a string or an int64
    int64_t sid = id;
    mpv_set_property(handle, "sid", MPV_FORMAT_INT64, &sid);
  }
}

VideoState get_metadata(mpv_handle *handle) {
  VideoState vs = {0};

  //get duration and fps
  mpv_get_property(handle, "duration", MPV_FORMAT_DOUBLE, &vs.duration);
  //try handling vfr
  if (mpv_get_property(handle, "estimated-vf-fps", MPV_FORMAT_DOUBLE, &vs.fps) < 0 || vs.fps <= 0) {
      mpv_get_property(handle, "container-fps", MPV_FORMAT_DOUBLE, &vs.fps);
  }

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
    snprintf(vs.filename, sizeof(vs.filename), "%s",title);
    mpv_free(title);
  }

  //get codec
  char *codec = mpv_get_property_string(handle, "video-codec");
  if (codec) {
    snprintf(vs.codec_name, sizeof(vs.codec_name), "%s", codec);
    mpv_free(codec);
  }

  int hw = 0;
  mpv_get_property(handle, "hwdec-current", MPV_FORMAT_FLAG, &hw);
  vs.hw_accel_used = hw;

  // Track Extraction
  if (mpv_get_property(handle, "track-list", MPV_FORMAT_NODE, &node) >= 0) {
    get_multitrack_metadata(&vs, node);
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

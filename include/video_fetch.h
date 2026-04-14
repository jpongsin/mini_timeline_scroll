//mini_timeline_scroll
//(c) 2026 jpongsin
//Licensed under MIT License
//See README.md and LICENSE.md for more info

#ifndef TIMELINE_SCRUB_QT_VIDEO_FETCH_H
#define TIMELINE_SCRUB_QT_VIDEO_FETCH_H

#include <libavformat/avformat.h>

#ifdef __cplusplus
extern "C" {
#endif


///////////////////////////////////////
///global variables
//using extern to prototype.... defined in video_fetch.c
extern const double fps29;
extern const double fps59;

////////////////////////////////////////
///prototypes
double fetchVideoInfo(const AVFormatContext *fic, int videoStreamIndex);

typedef struct {
    int index;
    char *label;
} AudioTrackInfo;

typedef struct {
    double fps;
    int audio_count;
    AudioTrackInfo *audio_tracks;
} VideoMediaInfo;

VideoMediaInfo load_video_for_qt(const char *videoFile);
void free_video_media_info(VideoMediaInfo *info);

///////////////////////////////////////
#ifdef __cplusplus
}
#endif
#endif //TIMELINE_SCRUB_QT_VIDEO_FETCH_H

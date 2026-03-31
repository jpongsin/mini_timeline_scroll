CC=gcc
CFLAGS= -g -Wall -o

TARGET= mini_timeline_scroll

${TARGET}: main.c video_fetch.c video_implement.c
	${CC} ${CFLAGS} ${TARGET} main.c video_fetch.c video_fetch.h video_implement.c video_implement.h timecode_check.c timecode_check.h -lm `pkg-config --cflags --libs gtk4 gstreamer-1.0 libavformat libavcodec libswscale libavutil`

.PHONY: clean
clean:
	-${RM} ${TARGET} *~

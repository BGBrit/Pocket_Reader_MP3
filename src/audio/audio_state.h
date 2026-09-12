#ifndef AUDIO_STATE_H
#define AUDIO_STATE_H

typedef enum
{
    PLAYBACK_NORMAL,
    PLAYBACK_REPEAT,
    PLAYBACK_SHUFFLE

} PlaybackMode;


typedef struct
{
    int current_playlist;

    int current_song;

    int playing;

    PlaybackMode mode;

} AudioState;


extern AudioState audio_state;

#endif

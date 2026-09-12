#include "audio_state.h"

AudioState audio_state =
{
    .current_playlist = 0,
    .current_song = 0,
    .playing = 1,
    .mode = PLAYBACK_NORMAL
};

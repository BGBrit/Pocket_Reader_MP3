#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
typedef enum
{
    AUDIO_PLAYLISTS,
    AUDIO_PLAYLIST_OPTIONS,
    AUDIO_PLAYLIST_VIEW,
    AUDIO_NOW_PLAYING

} AudioPage;



void audio_open(void);


void audio_handle_key(
    uint32_t key
);



void audio_init(void);



#ifdef __cplusplus
}
#endif

#endif

#ifndef NOW_PLAYING_H
#define NOW_PLAYING_H

#include <stdint.h>

void now_playing_open(
    int playlist_index,
    int song_index
);

int now_playing_handle_key(
    uint32_t key
);
void now_playing_refresh(void);

int now_playing_is_open(void);

void now_playing_close(void);
#endif

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void audio_open(void);

void audio_handle_key(uint32_t key);
void audio_load_songs(void);
#endif

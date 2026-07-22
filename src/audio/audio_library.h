#ifndef AUDIO_LIBRARY_H
#define AUDIO_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif


void audio_library_init(void);


int audio_library_get_count(void);


const char *audio_library_get_path(
    int index
);


const char *audio_library_get_name(
    int index
);


#ifdef __cplusplus
}
#endif

#endif

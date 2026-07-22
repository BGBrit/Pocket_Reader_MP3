#ifndef PLAYLIST_H
#define PLAYLIST_H

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_PLAYLISTS 50
#define MAX_PLAYLIST_SONGS 10000
#define PLAYLIST_NAME_SIZE 64



typedef struct
{
    char name[PLAYLIST_NAME_SIZE];


    int song_indices[MAX_PLAYLIST_SONGS];


    int song_count;


} Playlist;



void playlist_init(void);


void playlist_build_all_songs(void);



int playlist_get_count(void);


Playlist *playlist_get(
    int index
);



int playlist_create(
    const char *name
);



void playlist_delete(
    int index
);



void playlist_rename(
    int index,
    const char *name
);



void playlist_add_song(
    int playlist_index,
    int song_index
);



void playlist_remove_song(
    int playlist_index,
    int song_index
);

int playlist_is_all_songs(int index);

#ifdef __cplusplus
}
#endif

#endif

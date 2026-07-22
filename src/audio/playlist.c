#include "playlist.h"

#include "audio_library.h"

#include <string.h>
#include <stdio.h>

#include "playlist_storage.h"


static Playlist playlists[MAX_PLAYLISTS];


static int playlist_count = 0;

int playlist_is_all_songs(int index)
{
    return index == 0;
}

void playlist_init(void)
{
    playlist_count = 0;


    memset(
        playlists,
        0,
        sizeof(playlists)
    );


    strcpy(
        playlists[0].name,
        "All Songs"
    );


    playlists[0].song_count = 0;


    playlist_count = 1;


    printf(
        "Playlist system initialized\n"
    );
}



void playlist_build_all_songs(void)
{
    Playlist *all =
        playlist_get(0);


    if(!all)
        return;


    all->song_count = 0;


    for(int i = 0;
        i < audio_library_get_count();
        i++)
    {
        all->song_indices[all->song_count] = i;

        all->song_count++;
    }


    printf(
        "All Songs created: %d songs\n",
        all->song_count
    );
}



int playlist_get_count(void)
{
    return playlist_count;
}



Playlist *playlist_get(
    int index
)
{
    if(index < 0 ||
       index >= playlist_count)
    {
        return NULL;
    }


    return &playlists[index];
}


int playlist_create(
    const char *name
)
{
    if(playlist_count >= MAX_PLAYLISTS)
        return -1;


    Playlist *p =
        &playlists[playlist_count];


    memset(
        p,
        0,
        sizeof(Playlist)
    );


    strncpy(
        p->name,
        name,
        PLAYLIST_NAME_SIZE - 1
    );


    /*
     * Create empty playlist file
     */
    char path[512];

    snprintf(
        path,
        sizeof(path),
        "%s/%s.txt",
        PLAYLIST_DIRECTORY,
        name
    );


    FILE *fp =
        fopen(
            path,
            "w"
        );


    if(fp)
    {
        fclose(fp);

        printf(
            "Created playlist file: %s\n",
            path
        );
    }
    else
    {
        printf(
            "Failed creating playlist file\n"
        );
    }


    playlist_count++;


    printf(
        "Created playlist: %s\n",
        p->name
    );


    return playlist_count - 1;
}



void playlist_delete(
    int index
)
{
    /*
     * All Songs cannot be deleted
     */
    if(index <= 0 ||
       index >= playlist_count)
    {
        return;
    }


    for(int i = index;
        i < playlist_count - 1;
        i++)
    {
        playlists[i] =
            playlists[i + 1];
    }


    playlist_count--;
}



void playlist_rename(
    int index,
    const char *name
)
{
    if(index <= 0 ||
       index >= playlist_count)
    {
        return;
    }


    strncpy(
        playlists[index].name,
        name,
        PLAYLIST_NAME_SIZE - 1
    );
}



void playlist_add_song(
    int playlist_index,
    int song_index
)
{
    if(playlist_index < 0 ||
       playlist_index >= playlist_count)
    {
        return;
    }


    Playlist *p =
        &playlists[playlist_index];


    if(p->song_count >= MAX_PLAYLIST_SONGS)
        return;



    p->song_indices[p->song_count] =
        song_index;


    p->song_count++;
}



void playlist_remove_song(
    int playlist_index,
    int song_index
)
{
    if(playlist_index < 0 ||
       playlist_index >= playlist_count)
    {
        return;
    }


    Playlist *p =
        &playlists[playlist_index];


    for(int i = 0;
        i < p->song_count;
        i++)
    {
        if(p->song_indices[i] == song_index)
        {
            for(int j = i;
                j < p->song_count - 1;
                j++)
            {
                p->song_indices[j] =
                    p->song_indices[j + 1];
            }


            p->song_count--;

            return;
        }
    }
}

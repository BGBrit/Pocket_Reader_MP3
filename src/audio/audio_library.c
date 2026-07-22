#include "audio_library.h"

#include <dirent.h>
#include <string.h>
#include <stdio.h>


#define MAX_SONGS 10000


#define SONGS_DIRECTORY \
"/Users/beaubritain/Desktop/fakemicroSD/songs"



typedef struct
{
    char path[512];

} AudioSong;



static AudioSong songs[MAX_SONGS];


static int song_count = 0;



void audio_library_init(void)
{
    song_count = 0;


    DIR *dir =
        opendir(
            SONGS_DIRECTORY
        );


    if(!dir)
    {
        printf(
            "Could not open songs directory\n"
        );

        return;
    }


    struct dirent *entry;


    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_name[0] == '.')
            continue;


        const char *ext =
            strrchr(
                entry->d_name,
                '.'
            );


        if(!ext)
            continue;


        if(strcmp(ext, ".mp3") != 0)
            continue;



        snprintf(
            songs[song_count].path,
            sizeof(songs[song_count].path),
            "%s/%s",
            SONGS_DIRECTORY,
            entry->d_name
        );


        printf(
            "Loaded song: %s\n",
            songs[song_count].path
        );


        song_count++;


        if(song_count >= MAX_SONGS)
            break;
    }


    closedir(dir);


    printf(
        "Audio library loaded: %d songs\n",
        song_count
    );
}



int audio_library_get_count(void)
{
    return song_count;
}



const char *audio_library_get_path(
    int index
)
{
    if(index < 0 ||
       index >= song_count)
    {
        return NULL;
    }


    return songs[index].path;
}



const char *audio_library_get_name(
    int index
)
{
    static char name[256];


    if(index < 0 ||
       index >= song_count)
    {
        return "";
    }


    const char *file =
        strrchr(
            songs[index].path,
            '/'
        );


    if(file)
        file++;
    else
        file = songs[index].path;



    snprintf(
        name,
        sizeof(name),
        "%s",
        file
    );


    char *ext =
        strrchr(
            name,
            '.'
        );


    if(ext &&
       strcmp(ext, ".mp3") == 0)
    {
        *ext = '\0';
    }


    return name;
}

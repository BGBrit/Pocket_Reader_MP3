#include "playlist_storage.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>



void playlist_storage_load(void)
{
    DIR *dir =
        opendir(
            PLAYLIST_DIRECTORY
        );


    if(!dir)
    {
        printf(
            "No playlist directory\n"
        );

        return;
    }


    struct dirent *entry;


    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_name[0] == '.')
            continue;


        char name[128];


        strncpy(
            name,
            entry->d_name,
            sizeof(name)
        );


        char *ext =
            strrchr(
                name,
                '.'
            );


        if(ext)
            *ext = '\0';



        int index =
            playlist_create(
                name
            );


        if(index < 0)
            continue;


        char path[512];


        snprintf(
            path,
            sizeof(path),
            "%s/%s",
            PLAYLIST_DIRECTORY,
            entry->d_name
        );


        FILE *fp =
            fopen(
                path,
                "r"
            );


        if(!fp)
            continue;


        int song;


        while(
            fscanf(
                fp,
                "%d",
                &song
            ) == 1
        )
        {
            playlist_add_song(
                index,
                song
            );
        }


        fclose(fp);
    }


    closedir(dir);
}



void playlist_storage_save(void)
{
    /*
     * We will add this after
     * rename/delete is working.
     *
     * Saving individual playlists
     * is cleaner than rewriting
     * everything.
     */
}

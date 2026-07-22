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
    DIR *dir =
        opendir(
            PLAYLIST_DIRECTORY
        );


    if(dir)
    {
        struct dirent *entry;


        while((entry = readdir(dir)) != NULL)
        {
            if(entry->d_name[0] == '.')
                continue;


            char path[512];


            snprintf(
                path,
                sizeof(path),
                "%s/%s",
                PLAYLIST_DIRECTORY,
                entry->d_name
            );


            remove(path);
        }


        closedir(dir);
    }



    for(int i = 1;
        i < playlist_get_count();
        i++)
    {
        Playlist *p =
            playlist_get(i);


        if(!p)
            continue;


        char path[512];


        snprintf(
            path,
            sizeof(path),
            "%s/%s.txt",
            PLAYLIST_DIRECTORY,
            p->name
        );


        FILE *fp =
            fopen(
                path,
                "w"
            );


        if(!fp)
        {
            printf(
                "Could not save playlist %s\n",
                p->name
            );

            continue;
        }


        for(int s = 0;
            s < p->song_count;
            s++)
        {
            fprintf(
                fp,
                "%d\n",
                p->song_indices[s]
            );
        }


        fclose(fp);


        printf(
            "Saved playlist: %s\n",
            p->name
        );
    }
}

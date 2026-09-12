#include "playlist_storage.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


/*
 * Load playlists from disk.
 */

void playlist_storage_load(void)
{
    DIR *dir =
        opendir(PLAYLIST_DIRECTORY);

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
        /*
         * Ignore hidden files.
         */

        if(entry->d_name[0] == '.')
            continue;


        /*
         * Only load .txt playlist files.
         */

        const char *ext =
            strrchr(entry->d_name, '.');

        if(!ext ||
           strcmp(ext, ".txt") != 0)
        {
            continue;
        }


        /*
         * Copy filename so we can remove
         * the .txt extension.
         */

        char name[PLAYLIST_NAME_SIZE];

        size_t name_length =
            (size_t)(ext - entry->d_name);


        if(name_length >=
           sizeof(name))
        {
            name_length =
                sizeof(name) - 1;
        }


        memcpy(
            name,
            entry->d_name,
            name_length
        );

        name[name_length] = '\0';


        /*
         * Create the playlist.
         *
         * Index 0 remains All Songs.
         */

        int index =
            playlist_create(name);


        if(index < 0)
        {
            printf(
                "Could not load playlist: %s\n",
                name
            );

            continue;
        }


        /*
         * Build full file path.
         */

        char path[512];

        snprintf(
            path,
            sizeof(path),
            "%s/%s",
            PLAYLIST_DIRECTORY,
            entry->d_name
        );


        FILE *fp =
            fopen(path, "r");


        if(!fp)
        {
            printf(
                "Could not open playlist file: %s\n",
                path
            );

            continue;
        }


        /*
         * Read one song index per line.
         */

        int song_index;

        while(
            fscanf(
                fp,
                "%d",
                &song_index
            ) == 1
        )
        {
            playlist_add_song(
                index,
                song_index
            );
        }


        fclose(fp);


        Playlist *loaded =
            playlist_get(index);


        if(loaded)
        {
            printf(
                "Loaded playlist: %s (%d songs)\n",
                loaded->name,
                loaded->song_count
            );
        }
    }


    closedir(dir);
}


/*
 * Save all playlists to disk.
 *
 * The playlist directory is treated as a mirror
 * of the current in-memory playlist list.
 *
 * Existing .txt playlist files are removed first.
 * Then every current user-created playlist is
 * written back to disk.
 *
 * All Songs (index 0) is never saved.
 */

void playlist_storage_save(void)
{
    /*
     * Make sure the playlist directory exists.
     */

    struct stat st;

    if(
        stat(
            PLAYLIST_DIRECTORY,
            &st
        ) != 0
    )
    {
        if(
            mkdir(
                PLAYLIST_DIRECTORY,
                0755
            ) != 0
        )
        {
            printf(
                "Could not create playlist directory\n"
            );

            return;
        }


        printf(
            "Created playlist directory\n"
        );
    }


    /*
     * Remove old playlist files.
     *
     * This is important for rename/delete.
     */

    DIR *dir =
        opendir(PLAYLIST_DIRECTORY);


    if(dir)
    {
        struct dirent *entry;


        while(
            (entry = readdir(dir)) != NULL
        )
        {
            /*
             * Ignore hidden files.
             */

            if(entry->d_name[0] == '.')
                continue;


            /*
             * Only remove .txt files.
             */

            const char *ext =
                strrchr(
                    entry->d_name,
                    '.'
                );


            if(
                !ext ||
                strcmp(ext, ".txt") != 0
            )
            {
                continue;
            }


            /*
             * Build full path.
             */

            char path[512];

            snprintf(
                path,
                sizeof(path),
                "%s/%s",
                PLAYLIST_DIRECTORY,
                entry->d_name
            );


            /*
             * Delete old playlist file.
             */

            if(
                unlink(path) != 0
            )
            {
                printf(
                    "Could not remove old playlist file: %s\n",
                    path
                );
            }
        }


        closedir(dir);
    }


    /*
     * Write the current user-created playlists.
     *
     * Index 0 is All Songs and is excluded.
     */

    for(
        int i = 1;
        i < playlist_get_count();
        i++
    )
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
            fopen(path, "w");


        if(!fp)
        {
            printf(
                "Could not save playlist: %s\n",
                p->name
            );

            continue;
        }


        /*
         * Write one song index per line.
         */

        for(
            int j = 0;
            j < p->song_count;
            j++
        )
        {
            fprintf(
                fp,
                "%d\n",
                p->song_indices[j]
            );
        }


        fclose(fp);


        printf(
            "Saved playlist: %s (%d songs)\n",
            p->name,
            p->song_count
        );
    }
}

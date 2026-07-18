#include "audio.h"
#include "../my_custom_ui.h"
#include "lvgl/lvgl.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../common/scroll_list.h"

#define MAX_SONGS 10000

#define SONGS_DIRECTORY \
"/Users/beaubritain/Desktop/fakemicroSD/songs"

static lv_group_t *button_group;

typedef struct
{
    char path[512];

} AudioSong;


static AudioSong songs[MAX_SONGS];

static int song_count = 0;

static int selected_song = 0;

static lv_group_t *button_group;


static const char *audio_get_name(int index)
{
    static char name[256];


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

void audio_load_songs(void)
{
    song_count = 0;


    DIR *dir =
        opendir(SONGS_DIRECTORY);


    if(!dir)
    {
        printf("Could not open songs directory\n");
        return;
    }


    struct dirent *entry;


    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_name[0] == '.')
            continue;


        const char *ext =
            strrchr(entry->d_name, '.');


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
        "Songs loaded: %d\n",
        song_count
    );
}

void audio_open(void)
{
    scroll_list_create(
        "AUDIO"
    );


    scroll_list_set_count(
        song_count
    );


    scroll_list_set_text_callback(
        audio_get_name
    );


    scroll_list_set_selected(
        selected_song
    );


    ui_focus_keyboard();
}
void audio_handle_key(
    uint32_t key
)
{
    printf(
        "AUDIO KEY: %d\n",
        key
    );


    if(key == LV_KEY_DOWN ||
       key == LV_KEY_RIGHT)
    {
        selected_song++;


        if(selected_song >= song_count)
            selected_song = 0;


        scroll_list_set_selected(
            selected_song
        );
    }


    else if(key == LV_KEY_UP ||
            key == LV_KEY_LEFT)
    {
        selected_song--;


        if(selected_song < 0)
            selected_song =
                song_count - 1;


        scroll_list_set_selected(
            selected_song
        );
    }


    else if(key == ' ')
    {
        printf(
            "Selected song: %s\n",
            songs[selected_song].path
        );


        // Later:
        // now_playing_open()
    }
}

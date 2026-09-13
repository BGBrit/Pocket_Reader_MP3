#include "audio.h"
#include "now_playing.h"
#include "../my_custom_ui.h"
#include "audio_state.h"
#include "../common/scroll_list.h"

#include "audio_library.h"
#include "../common/text_entry.h"
#include "playlist.h"
#include "playlist_storage.h"
#include "lvgl/lvgl.h"

#include <stdio.h>
#include <string.h>

static int current_playlist;
static int selected_song;
static int removing_songs = 0;

static int editing_playlist_index = -1;
static lv_obj_t *playlist_options_popup = NULL;
static lv_obj_t *playlist_menu_popup = NULL;
static lv_obj_t *now_playing_popup = NULL;


static lv_obj_t *playlist_options_labels[2];
static lv_obj_t *playlist_menu_labels[4];
static lv_obj_t *now_playing_labels[2];

static int selected_playlist_option = 0;
static int selected_playlist_menu = 0;
static int selected_now_playing_option = 0;

static AudioPage current_page =
    AUDIO_PLAYLISTS;



static int selected_playlist = 0;




static void create_playlist_finished(
    const char *name
)
{
    if(strlen(name) == 0)
        return;


    playlist_create(
        name
    );


    playlist_storage_save();


    audio_open();


    printf(
        "Created playlist: %s\n",
        name
    );
}


static void rename_playlist_finished(
    const char *name
)
{
    if(editing_playlist_index <= 0)
        return;


    if(strlen(name) == 0)
        return;


    playlist_rename(
        editing_playlist_index,
        name
    );


    playlist_storage_save();


    audio_open();


    editing_playlist_index = -1;


    printf(
        "Renamed playlist: %s\n",
        name
    );
}

static void update_playlist_options_menu(void)
{
    const char *items[] =
    {
        "Create Playlist",
        "Cancel"
    };

    for(int i = 0; i < 2; i++)
    {
        if(i == selected_playlist_option)
        {
            lv_label_set_text_fmt(
                playlist_options_labels[i],
                "> %s",
                items[i]
            );
        }
        else
        {
            lv_label_set_text(
                playlist_options_labels[i],
                items[i]
            );
        }
    }
}

static void create_playlist_options_menu(void)
{
    playlist_options_popup =
        lv_obj_create(
            lv_screen_active()
        );

    lv_obj_set_size(
        playlist_options_popup,
        200,
        140
    );

    lv_obj_center(
        playlist_options_popup
    );

    lv_obj_clear_flag(
        playlist_options_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_set_style_bg_color(
        playlist_options_popup,
        lv_color_make(220,220,220),
        0
    );

    lv_obj_t *title =
        lv_label_create(
            playlist_options_popup
        );

    lv_label_set_text(
        title,
        "Playlist Options"
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_make(40,90,180),
        0
    );

    for(int i=0;i<2;i++)
    {
        playlist_options_labels[i] =
            lv_label_create(
                playlist_options_popup
            );

        lv_obj_align(
            playlist_options_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            45 + i*30
        );
    }

    selected_playlist_option = 0;

    update_playlist_options_menu();
}

static void close_playlist_options_menu(void)
{
    if(playlist_options_popup)
    {
        lv_obj_delete(
            playlist_options_popup
        );

        playlist_options_popup = NULL;
    }
}

static void update_playlist_menu(void)
{
    const char *items[] =
    {
        "Rename Playlist",
        "Remove Songs",
        "Shuffle",
        "Delete Playlist"
    };

    for(int i=0;i<4;i++)
    {
        if(i==selected_playlist_menu)
        {
            lv_label_set_text_fmt(
                playlist_menu_labels[i],
                "> %s",
                items[i]
            );
        }
        else
        {
            lv_label_set_text(
                playlist_menu_labels[i],
                items[i]
            );
        }
    }
}

static void create_playlist_menu(void)
{
    playlist_menu_popup =
        lv_obj_create(
            lv_screen_active()
        );

    lv_obj_set_size(
        playlist_menu_popup,
        210,
        190
    );

    lv_obj_center(
        playlist_menu_popup
    );

    lv_obj_set_style_bg_color(
        playlist_menu_popup,
        lv_color_make(220,220,220),
        0
    );

    lv_obj_clear_flag(
        playlist_menu_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_t *title =
        lv_label_create(
            playlist_menu_popup
        );

    lv_label_set_text(
        title,
        "Playlist"

    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_make(40,90,180),
        0
    );

    for(int i=0;i<4;i++)
    {
        playlist_menu_labels[i] =
            lv_label_create(
                playlist_menu_popup
            );

        lv_obj_align(
            playlist_menu_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            45 + i*30
        );
    }

    selected_playlist_menu = 0;

    update_playlist_menu();
}

static void close_playlist_menu(void)
{
    if(playlist_menu_popup)
    {
        lv_obj_delete(
            playlist_menu_popup
        );

        playlist_menu_popup = NULL;
    }
}

static const char *playlist_name_callback(
    int index
)
{
    Playlist *p =
        playlist_get(index);


    if(!p)
        return "";


    return p->name;
}



static const char *playlist_song_name_callback(
    int index
)
{
    Playlist *p =
        playlist_get(
            current_playlist
        );


    if(!p)
        return "";


    if(index >= p->song_count)
        return "";


    return audio_library_get_name(
        p->song_indices[index]
    );
}



void audio_init(void)
{
    audio_library_init();

    playlist_init();

    playlist_build_all_songs();

    playlist_storage_load();
}


void audio_open(void)
{
    current_page =
        AUDIO_PLAYLISTS;


    scroll_list_create(
        "PLAYLISTS"
    );


    scroll_list_set_count(
        playlist_get_count()
    );


    scroll_list_set_text_callback(
        playlist_name_callback
    );


    scroll_list_set_selected(
        selected_playlist
    );


    ui_focus_keyboard();
}



static void open_playlist_view(void)
{
    Playlist *p =
        playlist_get(
            selected_playlist
        );


    if(!p)
        return;


    current_playlist =
        selected_playlist;


    selected_song = 0;


    scroll_list_create(
        p->name
    );


    scroll_list_set_count(
        p->song_count
    );


    scroll_list_set_text_callback(
        playlist_song_name_callback
    );


    scroll_list_set_selected(
        selected_song
    );


    current_page =
        AUDIO_PLAYLIST_VIEW;


    ui_focus_keyboard();
}



static void open_playlists(void)
{
    current_page =
        AUDIO_PLAYLISTS;


    scroll_list_create(
        "PLAYLISTS"
    );


    scroll_list_set_count(
        playlist_get_count()
    );


    scroll_list_set_text_callback(
        playlist_name_callback
    );


    scroll_list_set_selected(
        selected_playlist
    );


    ui_focus_keyboard();
}


static int selected_option = 0;


static const char *playlist_option_callback(
    int index
)
{
    switch(index)
    {
        case 0:
            return "Create Playlist";

        default:
            return "";
    }
}


static void create_playlist(void)
{
    playlist_create(
        "New Playlist"
    );


    open_playlists();
}

static void open_playlist_options(void)
{
    selected_option = 0;


    scroll_list_create(
        "OPTIONS"
    );


    scroll_list_set_count(
        1
    );


    scroll_list_set_text_callback(
        playlist_option_callback
    );


    scroll_list_set_selected(
        selected_option
    );


    current_page =
        AUDIO_PLAYLIST_OPTIONS;


    ui_focus_keyboard();
}


static void open_remove_songs(void)
{
    Playlist *p =
        playlist_get(
            current_playlist
        );

    if(!p)
        return;


    /*
     * All Songs cannot have songs removed.
     */
    if(current_playlist <= 0)
        return;


    removing_songs = 1;


    /*
     * Keep the current selection valid.
     */
    if(p->song_count == 0)
    {
        selected_song = 0;
    }
    else if(selected_song >= p->song_count)
    {
        selected_song =
            p->song_count - 1;
    }


    scroll_list_create(
        "REMOVE SONGS"
    );


    scroll_list_set_count(
        p->song_count
    );


    scroll_list_set_text_callback(
        playlist_song_name_callback
    );


    scroll_list_set_selected(
        selected_song
    );


    ui_focus_keyboard();
}


static void close_remove_songs(void)
{
    removing_songs = 0;


    open_playlist_view();
}


void audio_handle_key(
    uint32_t key
)
{
    printf(
        "AUDIO KEY %d\n",
        key
    );

    /*
    * Remove Songs screen owns the keyboard.
    */
    if(removing_songs)
    {
        Playlist *p =
            playlist_get(
                current_playlist
            );


        if(!p)
        {
            removing_songs = 0;
            return;
        }


        /*
        * Back returns to the playlist.
        */
        if(key == 'b' ||
        key == 'B' ||
        key == LV_KEY_ESC)
        {
            close_remove_songs();
            return;
        }


        /*
        * Down / Right
        */
        if(key == LV_KEY_DOWN ||
        key == LV_KEY_RIGHT)
        {
            if(p->song_count > 0)
            {
                selected_song++;

                if(selected_song >= p->song_count)
                    selected_song = 0;


                scroll_list_set_selected(
                    selected_song
                );
            }

            return;
        }


        /*
        * Up / Left
        */
        if(key == LV_KEY_UP ||
        key == LV_KEY_LEFT)
        {
            if(p->song_count > 0)
            {
                selected_song--;

                if(selected_song < 0)
                    selected_song =
                        p->song_count - 1;


                scroll_list_set_selected(
                    selected_song
                );
            }

            return;
        }


        /*
        * Space removes the selected song.
        */
        if(key == ' ')
        {
            if(p->song_count > 0 &&
            selected_song >= 0 &&
            selected_song < p->song_count)
            {
                int song_index =
                    p->song_indices[selected_song];


                printf(
                    "Removing '%s' from playlist '%s'\n",
                    audio_library_get_name(song_index),
                    p->name
                );


                playlist_remove_song(
                    current_playlist,
                    song_index
                );


                playlist_storage_save();


                /*
                * Keep selection valid after removal.
                */
                if(p->song_count == 0)
                {
                    selected_song = 0;
                }
                else if(selected_song >= p->song_count)
                {
                    selected_song =
                        p->song_count - 1;
                }


                scroll_list_set_count(
                    p->song_count
                );


                scroll_list_set_selected(
                    selected_song
                );
            }

            return;
        }


        return;
    }
    /*
     * Text entry / popups own the keyboard.
     */
    if(playlist_options_popup)
    {
        if(key == LV_KEY_LEFT)
        {
            if(selected_playlist_option > 0)
                selected_playlist_option--;

            update_playlist_options_menu();
        }
        else if(key == LV_KEY_RIGHT)
        {
            if(selected_playlist_option < 1)
                selected_playlist_option++;

            update_playlist_options_menu();
        }
        else if(key == ' ')
        {
            if(selected_playlist_option == 0)
            {
                close_playlist_options_menu();

                text_entry_open(
                    "Create Playlist",
                    "",
                    create_playlist_finished
                );
            }
            else
            {
                close_playlist_options_menu();
            }
        }
        else if(
            key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC)
        {
            close_playlist_options_menu();
        }

        return;
    }


    if(playlist_menu_popup)
    {
        if(key == LV_KEY_LEFT)
        {
            if(selected_playlist_menu > 0)
                selected_playlist_menu--;

            update_playlist_menu();
        }

        else if(key == LV_KEY_RIGHT)
        {
            if(selected_playlist_menu < 3)
                selected_playlist_menu++;

            update_playlist_menu();
        }

        else if(key == ' ')
        {
            switch(selected_playlist_menu)
            {
                case 0:
                {
                    Playlist *p =
                        playlist_get(
                            current_playlist
                        );

                    if(p && current_playlist > 0)
                    {
                        editing_playlist_index =
                            current_playlist;

                        close_playlist_menu();

                        text_entry_open(
                            "Rename Playlist",
                            p->name,
                            rename_playlist_finished
                        );
                    }

                    break;
                }

                case 1:
                    if(current_playlist > 0)
                    {
                        close_playlist_menu();

                        open_remove_songs();
                    }
                    break;

                case 2:
                {
                    Playlist *p =
                        playlist_get(
                            current_playlist
                        );

                    if(p && p->song_count > 0)
                    {
                        /*
                        * Enable shuffle mode.
                        */
                        audio_state.mode =
                            PLAYBACK_SHUFFLE;

                        /*
                        * Start from the first song.
                        * now_playing_open() will use the
                        * existing select_random_song()
                        * to choose a random song.
                        */
                        audio_state.current_song = 0;

                        close_playlist_menu();

                        now_playing_open(
                            current_playlist,
                            audio_state.current_song
                        );

                        current_page =
                            AUDIO_NOW_PLAYING;
                    }
                    break;
                }
                case 3:
                {
                    if(current_playlist > 0)
                    {
                        playlist_delete(
                            current_playlist
                        );

                        playlist_storage_save();

                        close_playlist_menu();

                        open_playlists();
                    }

                    break;
                }
            }

            close_playlist_menu();
        }

        else if(
            key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC)
        {
            close_playlist_menu();
        }

        return;
    }


    /*
     * Now Playing owns its own controls.
     */
    if(current_page == AUDIO_NOW_PLAYING)
    {
        if(now_playing_handle_key(key))
        {
            now_playing_close();

            open_playlist_view();

            current_page =
                AUDIO_PLAYLIST_VIEW;
        }

        return;
    }


    /*
     * Back navigation
     */
    if(key == 'b' ||
       key == 'B' ||
       key == LV_KEY_ESC)
    {
        switch(current_page)
        {
            case AUDIO_PLAYLIST_VIEW:

                open_playlists();

                return;

            case AUDIO_PLAYLISTS:

                open_home();

                return;

            default:

                return;
        }
    }


    /*
     * Options
     */
    if(key == 's' ||
       key == 'S')
    {
        if(current_page == AUDIO_PLAYLISTS)
        {
            create_playlist_options_menu();
        }
        else if(current_page == AUDIO_PLAYLIST_VIEW)
        {
            create_playlist_menu();
        }

        return;
    }


    /*
     * Select
     */
    if(key == ' ')
    {
        if(current_page == AUDIO_PLAYLISTS)
        {
            open_playlist_view();
        }
        else if(current_page == AUDIO_PLAYLIST_VIEW)
        {
            audio_state.current_playlist =
                current_playlist;

            audio_state.current_song =
                selected_song;

            now_playing_open(
                audio_state.current_playlist,
                audio_state.current_song
            );

            current_page =
                AUDIO_NOW_PLAYING;
        }

        return;
    }


    /*
     * Down / Right
     */
    if(key == LV_KEY_DOWN ||
       key == LV_KEY_RIGHT)
    {
        if(current_page == AUDIO_PLAYLISTS)
        {
            selected_playlist++;

            if(selected_playlist >= playlist_get_count())
                selected_playlist = 0;

            scroll_list_set_selected(
                selected_playlist
            );
        }
        else if(current_page == AUDIO_PLAYLIST_VIEW)
        {
            Playlist *p =
                playlist_get(
                    current_playlist
                );

            if(p && p->song_count > 0)
            {
                selected_song++;

                if(selected_song >= p->song_count)
                    selected_song = 0;

                scroll_list_set_selected(
                    selected_song
                );
            }
        }

        return;
    }


    /*
     * Up / Left
     */
    if(key == LV_KEY_UP ||
       key == LV_KEY_LEFT)
    {
        if(current_page == AUDIO_PLAYLISTS)
        {
            selected_playlist--;

            if(selected_playlist < 0)
                selected_playlist =
                    playlist_get_count() - 1;

            scroll_list_set_selected(
                selected_playlist
            );
        }
        else if(current_page == AUDIO_PLAYLIST_VIEW)
        {
            Playlist *p =
                playlist_get(
                    current_playlist
                );

            if(p && p->song_count > 0)
            {
                selected_song--;

                if(selected_song < 0)
                    selected_song =
                        p->song_count - 1;

                scroll_list_set_selected(
                    selected_song
                );
            }
        }

        return;
    }
}

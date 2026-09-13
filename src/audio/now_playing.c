#include "now_playing.h"

#include "audio_library.h"
#include "audio_player.h"
#include "playlist.h"
#include "playlist_storage.h"
#include "../assets/wallpaper_test.h"

#include "lvgl/lvgl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "audio_state.h"

/*
 * Add to Playlist popup
 */

static lv_obj_t *add_to_playlist_popup = NULL;

static lv_obj_t *add_to_playlist_labels[
    MAX_PLAYLISTS - 1
];

static int add_to_playlist_count = 0;

static int selected_add_to_playlist = 0;

/*
 * Playback state
 */

static double elapsed_seconds = 0.0;

static lv_timer_t *progress_timer = NULL;

static int now_playing_open_flag = 0;


/*
 * Screen widgets
 */

static lv_obj_t *song_label;

static lv_obj_t *playlist_label;

static lv_obj_t *progress_bar;

static lv_obj_t *time_label;

static lv_obj_t *play_label;

static lv_obj_t *mode_label;


/*
 * Now Playing Options popup
 */

static lv_obj_t *now_playing_options_popup = NULL;

static lv_obj_t *now_playing_options_labels[2];

static int selected_now_playing_option = 0;


/*
 * Playback Mode popup
 */

static lv_obj_t *playback_mode_popup = NULL;

static lv_obj_t *playback_mode_labels[3];

static int selected_playback_mode = 0;


/*
 * Internal helpers
 */

static void update_time_label(void);

static void update_progress(void);

static void update_play_button(void);

static void update_mode_label(void);

static void progress_timer_callback(
    lv_timer_t *timer
);

static void update_now_playing_options_menu(void);

static void create_now_playing_options_menu(void);

static void close_now_playing_options_menu(void);

static void update_playback_mode_menu(void);

static void create_playback_mode_menu(void);

static void close_playback_mode_menu(void);

static void update_add_to_playlist_menu(void);

static void create_add_to_playlist_menu(void);

static void close_add_to_playlist_menu(void);

static void advance_to_next_song(void);

static void select_random_song(
    Playlist *p
);


/*
 * Open Now Playing
 */

void now_playing_open(
    int playlist_index,
    int song_index
)
{
    audio_state.current_playlist =
        playlist_index;

    audio_state.current_song =
        song_index;

    if(audio_state.mode == PLAYBACK_SHUFFLE)
    {
        Playlist *p =
            playlist_get(playlist_index);

        if(p && p->song_count > 1)
        {
            select_random_song(p);
        }
    }

    elapsed_seconds = 0;

    audio_state.playing = 1;

    now_playing_open_flag = 1;


    /*
     * Seed random number generator once.
     */

    static int random_seeded = 0;

    if(!random_seeded)
    {
        srand(
            (unsigned int)time(NULL)
        );

        random_seeded = 1;
    }


    /*
     * Clear existing screen.
     */

    lv_obj_clean(
        lv_screen_active()
    );


    /*
     * Wallpaper
     */

    lv_obj_t *bg =
        lv_image_create(
            lv_screen_active()
        );

    lv_image_set_src(
        bg,
        &wallpaper_test
    );

    lv_obj_center(
        bg
    );

    lv_obj_move_background(
        bg
    );


    /*
     * Title
     */

    lv_obj_t *title =
        lv_label_create(
            lv_screen_active()
        );

    lv_label_set_text(
        title,
        "NOW PLAYING"
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        20
    );


    /*
     * Song title
     */

    song_label =
        lv_label_create(
            lv_screen_active()
        );

    lv_obj_set_width(
        song_label,
        210
    );

    lv_label_set_long_mode(
        song_label,
        LV_LABEL_LONG_WRAP
    );

    lv_obj_set_style_text_align(
        song_label,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_style_text_color(
        song_label,
        lv_color_white(),
        0
    );

    lv_obj_align(
        song_label,
        LV_ALIGN_TOP_MID,
        0,
        70
    );


    /*
     * Playlist name
     */

    playlist_label =
        lv_label_create(
            lv_screen_active()
        );

    lv_obj_set_style_text_color(
        playlist_label,
        lv_palette_lighten(
            LV_PALETTE_BLUE,
            2
        ),
        0
    );

    lv_obj_align(
        playlist_label,
        LV_ALIGN_TOP_MID,
        0,
        125
    );


    /*
     * Progress bar
     */

    progress_bar =
        lv_bar_create(
            lv_screen_active()
        );

    lv_obj_set_size(
        progress_bar,
        180,
        10
    );

    lv_obj_align(
        progress_bar,
        LV_ALIGN_TOP_MID,
        0,
        170
    );

    lv_bar_set_range(
        progress_bar,
        0,
        100
    );


    /*
     * Time
     */

    time_label =
        lv_label_create(
            lv_screen_active()
        );

    lv_obj_set_style_text_color(
        time_label,
        lv_color_white(),
        0
    );

    lv_obj_align(
        time_label,
        LV_ALIGN_TOP_MID,
        0,
        190
    );


    /*
     * Play / Pause
     */

    play_label =
        lv_label_create(
            lv_screen_active()
        );

    lv_obj_set_style_text_font(
        play_label,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_set_style_text_color(
        play_label,
        lv_color_white(),
        0
    );

    lv_obj_align(
        play_label,
        LV_ALIGN_TOP_MID,
        0,
        225
    );


    /*
     * Playback mode
     */

    mode_label =
        lv_label_create(
            lv_screen_active()
        );

    lv_obj_set_style_text_color(
        mode_label,
        lv_palette_lighten(
            LV_PALETTE_BLUE,
            2
        ),
        0
    );

    lv_obj_align(
        mode_label,
        LV_ALIGN_BOTTOM_MID,
        0,
        -18
    );


    /*
     * Display song information.
     */

    Playlist *p =
        playlist_get(
            audio_state.current_playlist
        );

    if(p &&
       audio_state.current_song >= 0 &&
       audio_state.current_song < p->song_count)
    {
        lv_label_set_text(
            song_label,
            audio_library_get_name(
                p->song_indices[
                    audio_state.current_song
                ]
            )
        );

        lv_label_set_text_fmt(
            playlist_label,
            "Playlist: %s",
            p->name
        );

        int song_index =
            p->song_indices[audio_state.current_song];

        const char *path =
            audio_library_get_path(song_index);

        if(path)
        {
            if(audio_player_play(path) != 0)
            {
                printf(
                    "Now Playing: failed to start audio.\n"
                );

                audio_state.playing = 0;
            }
            else
            {
                audio_state.playing = 1;
            }
        }
    }


    update_progress();

    update_time_label();

    update_play_button();

    update_mode_label();


    /*
     * Start timer.
     */

    if(progress_timer)
    {
        lv_timer_delete(
            progress_timer
        );
    }

    progress_timer =
        lv_timer_create(
            progress_timer_callback,
            1000,
            NULL
        );
}


/*
 * Update play button.
 */

static void update_play_button(void)
{
    if(audio_state.playing)
    {
        lv_label_set_text(
            play_label,
            LV_SYMBOL_PAUSE
        );
    }
    else
    {
        lv_label_set_text(
            play_label,
            LV_SYMBOL_PLAY
        );
    }
}


/*
 * Update playback mode label.
 */

static void update_mode_label(void)
{
    switch(audio_state.mode)
    {
        case PLAYBACK_NORMAL:

            lv_label_set_text(
                mode_label,
                "Mode: Normal"
            );

            break;


        case PLAYBACK_REPEAT:

            lv_label_set_text(
                mode_label,
                "Mode: Repeat"
            );

            break;


        case PLAYBACK_SHUFFLE:

            lv_label_set_text(
                mode_label,
                "Mode: Shuffle"
            );

            break;
    }
}


/*
 * Update time label.
 */

static void update_time_label(void)
{
    double position =
        audio_player_get_position();

    double duration =
        audio_player_get_duration();

    int current_total_seconds =
        (int)position;

    int total_seconds =
        (int)duration;

    int current_minutes =
        current_total_seconds / 60;

    int current_seconds =
        current_total_seconds % 60;

    int total_minutes =
        total_seconds / 60;

    int remaining_seconds =
        total_seconds % 60;

    lv_label_set_text_fmt(
        time_label,
        "%02d:%02d / %02d:%02d",
        current_minutes,
        current_seconds,
        total_minutes,
        remaining_seconds
    );
}
/*
 * Update progress bar.
 */

static void update_progress(void)
{
    double position =
        audio_player_get_position();

    double duration =
        audio_player_get_duration();

    if(duration <= 0.0)
    {
        lv_bar_set_value(
            progress_bar,
            0,
            LV_ANIM_OFF
        );

        return;
    }

    int progress =
        (int)((position / duration) * 100.0);

    if(progress < 0)
        progress = 0;

    if(progress > 100)
        progress = 100;

    lv_bar_set_value(
        progress_bar,
        progress,
        LV_ANIM_OFF
    );
}


/*
 * Refresh Now Playing.
 */

void now_playing_refresh(void)
{
    Playlist *p =
        playlist_get(
            audio_state.current_playlist
        );

    if(!p)
        return;


    if(audio_state.current_song < 0 ||
       audio_state.current_song >= p->song_count)
    {
        return;
    }


    lv_label_set_text(
        song_label,
        audio_library_get_name(
            p->song_indices[
                audio_state.current_song
            ]
        )
    );


    lv_label_set_text_fmt(
        playlist_label,
        "Playlist: %s",
        p->name
    );


    update_progress();

    update_time_label();

    update_play_button();

    update_mode_label();
}


/*
 * Select a random song.
 */

static void select_random_song(
    Playlist *p
)
{
    if(!p)
        return;

    if(p->song_count <= 1)
        return;


    int new_song =
        audio_state.current_song;


    while(new_song ==
          audio_state.current_song)
    {
        new_song =
            rand() % p->song_count;
    }


    audio_state.current_song =
        new_song;
}


/*
 * Advance when a song finishes.
 */

static void advance_to_next_song(void)
{
    Playlist *p =
        playlist_get(
            audio_state.current_playlist
        );

    if(!p ||
       p->song_count <= 0)
    {
        audio_state.playing = 0;
        return;
    }


    switch(audio_state.mode)
    {
        case PLAYBACK_NORMAL:

            audio_state.current_song++;

            if(audio_state.current_song >=
               p->song_count)
            {
                /*
                 * Normal mode stops at the
                 * end of the playlist.
                 */

                audio_state.current_song =
                    p->song_count - 1;

                audio_state.playing = 0;

                audio_player_stop();
                now_playing_refresh();

                return;
            }

            break;


        case PLAYBACK_REPEAT:

            /*
             * Stay on the current song.
             */

            break;


        case PLAYBACK_SHUFFLE:

            select_random_song(p);

            break;
    }


    Playlist *next_playlist =
        playlist_get(
            audio_state.current_playlist
        );

    if(next_playlist &&
    audio_state.current_song >= 0 &&
    audio_state.current_song < next_playlist->song_count)
    {
        int song_index =
            next_playlist->song_indices[
                audio_state.current_song
            ];

        const char *path =
            audio_library_get_path(song_index);

        if(path)
        {
            if(audio_player_play(path) == 0)
            {
                audio_state.playing = 1;
            }
            else
            {
                audio_state.playing = 0;
            }
        }
    }

    now_playing_refresh();
}


/*
 * Playback timer.
 */

static void progress_timer_callback(
    lv_timer_t *timer
)
{
    LV_UNUSED(timer);

    double position =
        audio_player_get_position();

    double duration =
        audio_player_get_duration();

    /*
     * Keep UI state synchronized with
     * the real audio player.
     */
    audio_state.playing =
        audio_player_is_playing();

    /*
     * Song has finished.
     */
    if(duration > 0.0 &&
       position >= duration)
    {
        audio_state.playing = 0;

        advance_to_next_song();

        return;
    }

    update_progress();
    update_time_label();
    update_play_button();
}

/*
 * Update Now Playing options menu.
 */

static void update_now_playing_options_menu(void)
{
    const char *items[] =
    {
        "Add to Playlist",
        "Playback Mode"
    };


    for(int i = 0; i < 2; i++)
    {
        if(i == selected_now_playing_option)
        {
            lv_label_set_text_fmt(
                now_playing_options_labels[i],
                "> %s",
                items[i]
            );
        }
        else
        {
            lv_label_set_text(
                now_playing_options_labels[i],
                items[i]
            );
        }
    }
}


/*
 * Create Now Playing options menu.
 */

static void create_now_playing_options_menu(void)
{
    now_playing_options_popup =
        lv_obj_create(
            lv_screen_active()
        );

    lv_obj_set_size(
        now_playing_options_popup,
        200,
        140
    );

    lv_obj_center(
        now_playing_options_popup
    );

    lv_obj_clear_flag(
        now_playing_options_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_set_style_bg_color(
        now_playing_options_popup,
        lv_color_make(
            220,
            220,
            220
        ),
        0
    );


    lv_obj_t *title =
        lv_label_create(
            now_playing_options_popup
        );

    lv_label_set_text(
        title,
        "Now Playing"
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_make(
            40,
            90,
            180
        ),
        0
    );


    for(int i = 0; i < 2; i++)
    {
        now_playing_options_labels[i] =
            lv_label_create(
                now_playing_options_popup
            );

        lv_obj_align(
            now_playing_options_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            45 + (i * 30)
        );
    }


    selected_now_playing_option = 0;

    update_now_playing_options_menu();
}


/*
 * Close Now Playing options.
 */

static void close_now_playing_options_menu(void)
{
    if(now_playing_options_popup)
    {
        lv_obj_delete(
            now_playing_options_popup
        );

        now_playing_options_popup =
            NULL;
    }
}


/*
 * Update Playback Mode menu.
 */

static void update_playback_mode_menu(void)
{
    const char *items[] =
    {
        "Normal",
        "Repeat",
        "Shuffle"
    };


    for(int i = 0; i < 3; i++)
    {
        if(i == selected_playback_mode)
        {
            lv_label_set_text_fmt(
                playback_mode_labels[i],
                "> %s",
                items[i]
            );
        }
        else
        {
            lv_label_set_text(
                playback_mode_labels[i],
                items[i]
            );
        }
    }
}


/*
 * Create Playback Mode menu.
 */

static void create_playback_mode_menu(void)
{
    playback_mode_popup =
        lv_obj_create(
            lv_screen_active()
        );

    lv_obj_set_size(
        playback_mode_popup,
        200,
        170
    );

    lv_obj_center(
        playback_mode_popup
    );

    lv_obj_clear_flag(
        playback_mode_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_set_style_bg_color(
        playback_mode_popup,
        lv_color_make(
            220,
            220,
            220
        ),
        0
    );


    lv_obj_t *title =
        lv_label_create(
            playback_mode_popup
        );

    lv_label_set_text(
        title,
        "Playback Mode"
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_make(
            40,
            90,
            180
        ),
        0
    );


    for(int i = 0; i < 3; i++)
    {
        playback_mode_labels[i] =
            lv_label_create(
                playback_mode_popup
            );

        lv_obj_align(
            playback_mode_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            45 + (i * 30)
        );
    }


    selected_playback_mode =
        (int)audio_state.mode;


    if(selected_playback_mode < 0 ||
       selected_playback_mode > 2)
    {
        selected_playback_mode = 0;
    }


    update_playback_mode_menu();
}


/*
 * Close Playback Mode menu.
 */

static void close_playback_mode_menu(void)
{
    if(playback_mode_popup)
    {
        lv_obj_delete(
            playback_mode_popup
        );

        playback_mode_popup =
            NULL;
    }
    if(add_to_playlist_popup)
    {
        lv_obj_delete(
            add_to_playlist_popup
        );

        add_to_playlist_popup =
            NULL;
    }
}

/*
 * Update Add to Playlist menu.
 */

static void update_add_to_playlist_menu(void)
{
    for(int i = 0;
        i < add_to_playlist_count;
        i++)
    {
        /*
         * Playlist index is i + 1 because
         * index 0 is All Songs.
         */

        Playlist *p =
            playlist_get(i + 1);


        if(!p)
            continue;


        if(i == selected_add_to_playlist)
        {
            lv_label_set_text_fmt(
                add_to_playlist_labels[i],
                "> %s",
                p->name
            );
        }
        else
        {
            lv_label_set_text(
                add_to_playlist_labels[i],
                p->name
            );
        }
    }


    /*
     * Keep the selected item visible.
     */

    if(add_to_playlist_count > 0)
    {
        lv_obj_scroll_to_view(
            add_to_playlist_labels[
                selected_add_to_playlist
            ],
            LV_ANIM_OFF
        );
    }
}


/*
 * Create Add to Playlist menu.
 */

static void create_add_to_playlist_menu(void)
{
    int playlist_count =
        playlist_get_count();


    /*
     * All Songs is index 0, so only
     * playlists after it are selectable.
     */

    add_to_playlist_count =
        playlist_count - 1;


    if(add_to_playlist_count <= 0)
    {
        printf(
            "No user playlists available\n"
        );

        return;
    }


    add_to_playlist_popup =
        lv_obj_create(
            lv_screen_active()
        );


    /*
     * Keep the popup a reasonable size even
     * if many playlists exist.
     */

    int popup_height =
        80 + (add_to_playlist_count * 30);


    if(popup_height > 260)
    {
        popup_height = 260;
    }


    lv_obj_set_size(
        add_to_playlist_popup,
        210,
        popup_height
    );


    lv_obj_center(
        add_to_playlist_popup
    );


    /*
     * Allow scrolling when there are many
     * playlists.
     */

    if(add_to_playlist_count > 6)
    {
        lv_obj_add_flag(
            add_to_playlist_popup,
            LV_OBJ_FLAG_SCROLLABLE
        );
    }
    else
    {
        lv_obj_clear_flag(
            add_to_playlist_popup,
            LV_OBJ_FLAG_SCROLLABLE
        );
    }


    lv_obj_set_style_bg_color(
        add_to_playlist_popup,
        lv_color_make(
            220,
            220,
            220
        ),
        0
    );


    /*
     * Title.
     */

    lv_obj_t *title =
        lv_label_create(
            add_to_playlist_popup
        );


    lv_label_set_text(
        title,
        "Add to Playlist"
    );


    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );


    lv_obj_set_style_text_color(
        title,
        lv_color_make(
            40,
            90,
            180
        ),
        0
    );


    /*
     * Create playlist labels.
     */

    for(int i = 0;
        i < add_to_playlist_count;
        i++)
    {
        add_to_playlist_labels[i] =
            lv_label_create(
                add_to_playlist_popup
            );


        lv_obj_align(
            add_to_playlist_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            45 + (i * 30)
        );
    }


    selected_add_to_playlist = 0;


    update_add_to_playlist_menu();
}


/*
 * Close Add to Playlist menu.
 */

static void close_add_to_playlist_menu(void)
{
    if(add_to_playlist_popup)
    {
        lv_obj_delete(
            add_to_playlist_popup
        );

        add_to_playlist_popup = NULL;
    }


    add_to_playlist_count = 0;

    selected_add_to_playlist = 0;
}

/*
 * Is Now Playing open?
 */

int now_playing_is_open(void)
{
    return now_playing_open_flag;
}


/*
 * Close Now Playing.
 */

void now_playing_close(void)
{
    now_playing_open_flag = 0;


    if(now_playing_options_popup)
    {
        lv_obj_delete(
            now_playing_options_popup
        );

        now_playing_options_popup =
            NULL;
    }


    if(playback_mode_popup)
    {
        lv_obj_delete(
            playback_mode_popup
        );

        playback_mode_popup =
            NULL;
    }


    if(progress_timer)
    {
        lv_timer_delete(
            progress_timer
        );

        progress_timer = NULL;
    }
}


/*
 * Handle keyboard input.
 */

int now_playing_handle_key(
    uint32_t key
)
{

    /*
    * Add to Playlist popup.
    */

    if(add_to_playlist_popup)
    {
        if(key == LV_KEY_LEFT ||
        key == LV_KEY_UP)
        {
            if(selected_add_to_playlist > 0)
            {
                selected_add_to_playlist--;

                update_add_to_playlist_menu();
            }

            return 0;
        }


        if(key == LV_KEY_RIGHT ||
        key == LV_KEY_DOWN)
        {
            if(selected_add_to_playlist <
            add_to_playlist_count - 1)
            {
                selected_add_to_playlist++;

                update_add_to_playlist_menu();
            }

            return 0;
        }


        /*
        * Space = add current song.
        */

        if(key == ' ')
        {
            /*
            * Convert the popup selection back
            * to the actual playlist index.
            */

            int playlist_index =
                selected_add_to_playlist + 1;


            /*
            * Get the currently playing playlist.
            */

            Playlist *current_playlist =
                playlist_get(
                    audio_state.current_playlist
                );


            if(current_playlist &&
            audio_state.current_song >= 0 &&
            audio_state.current_song <
                current_playlist->song_count)
            {
                int song_index =
                    current_playlist->song_indices[
                        audio_state.current_song
                    ];


                Playlist *destination =
                    playlist_get(
                        playlist_index
                    );


                if(destination)
                {
                    int old_count =
                        destination->song_count;


                    playlist_add_song(
                        playlist_index,
                        song_index
                    );


                    if(destination->song_count >
                    old_count)
                    {
                        printf(
                            "Added '%s' to playlist '%s'\n",
                            audio_library_get_name(
                                song_index
                            ),
                            destination->name
                        );
                        playlist_storage_save();
                    }
                    else
                    {
                        printf(
                            "Song already in playlist '%s'\n",
                            destination->name
                        );
                    }
                }
            }


            close_add_to_playlist_menu();

            return 0;
        }


        /*
        * Back = close popup.
        */

        if(key == 'b' ||
        key == 'B' ||
        key == LV_KEY_ESC)
        {
            close_add_to_playlist_menu();

            return 0;
        }


        return 0;
    }
    /*
     * Playback Mode popup.
     */

    if(playback_mode_popup)
    {
        if(key == LV_KEY_LEFT ||
           key == LV_KEY_UP)
        {
            if(selected_playback_mode > 0)
            {
                selected_playback_mode--;

                update_playback_mode_menu();
            }

            return 0;
        }


        if(key == LV_KEY_RIGHT ||
           key == LV_KEY_DOWN)
        {
            if(selected_playback_mode < 2)
            {
                selected_playback_mode++;

                update_playback_mode_menu();
            }

            return 0;
        }


        if(key == ' ')
        {
            audio_state.mode =
                (PlaybackMode)
                selected_playback_mode;

            close_playback_mode_menu();

            update_mode_label();

            return 0;
        }


        if(key == 'b' ||
           key == 'B' ||
           key == LV_KEY_ESC)
        {
            close_playback_mode_menu();

            return 0;
        }


        return 0;
    }


    /*
     * Now Playing options popup.
     */

    if(now_playing_options_popup)
    {
        if(key == LV_KEY_LEFT ||
           key == LV_KEY_UP)
        {
            if(selected_now_playing_option > 0)
            {
                selected_now_playing_option--;

                update_now_playing_options_menu();
            }

            return 0;
        }


        if(key == LV_KEY_RIGHT ||
           key == LV_KEY_DOWN)
        {
            if(selected_now_playing_option < 1)
            {
                selected_now_playing_option++;

                update_now_playing_options_menu();
            }

            return 0;
        }


        if(key == ' ')
        {
            if(selected_now_playing_option == 0)
            {
                close_now_playing_options_menu();

                create_add_to_playlist_menu();
            }
            else
            {
                close_now_playing_options_menu();

                create_playback_mode_menu();
            }

            return 0;
        }


        if(key == 'b' ||
           key == 'B' ||
           key == LV_KEY_ESC)
        {
            close_now_playing_options_menu();

            return 0;
        }


        return 0;
    }


    /*
     * Space = Play / Pause.
     */

    if(key == ' ')
    {
        if(audio_state.playing)
        {
            audio_player_pause();

            audio_state.playing = 0;
        }
        else
        {
            audio_player_resume();

            audio_state.playing =
                audio_player_is_playing();
        }

        update_play_button();

        return 0;
    }


    /*
     * Right = Next song.
     */

    if(key == LV_KEY_RIGHT)
    {
        Playlist *p =
            playlist_get(
                audio_state.current_playlist
            );

        if(p &&
           p->song_count > 0)
        {
            if(audio_state.mode ==
               PLAYBACK_SHUFFLE)
            {
                select_random_song(p);
            }
            else
            {
                audio_state.current_song++;

                if(audio_state.current_song >=
                   p->song_count)
                {
                    audio_state.current_song = 0;
                }
            }

            Playlist *new_playlist =
                playlist_get(
                    audio_state.current_playlist
                );

            if(new_playlist &&
            audio_state.current_song >= 0 &&
            audio_state.current_song < new_playlist->song_count)
            {
                int song_index =
                    new_playlist->song_indices[
                        audio_state.current_song
                    ];

                const char *path =
                    audio_library_get_path(song_index);

                if(path)
                {
                    if(audio_player_play(path) == 0)
                    {
                        audio_state.playing = 1;
                    }
                    else
                    {
                        audio_state.playing = 0;
                    }
                }
            }

            now_playing_refresh();
        }

        return 0;
    }


    /*
     * Left = Previous song.
     */

    if(key == LV_KEY_LEFT)
    {
        Playlist *p =
            playlist_get(
                audio_state.current_playlist
            );

        if(p &&
           p->song_count > 0)
        {
            if(audio_state.mode ==
               PLAYBACK_SHUFFLE)
            {
                select_random_song(p);
            }
            else
            {
                audio_state.current_song--;

                if(audio_state.current_song < 0)
                {
                    audio_state.current_song =
                        p->song_count - 1;
                }
            }

            Playlist *new_playlist =
                playlist_get(
                    audio_state.current_playlist
                );

            if(new_playlist &&
            audio_state.current_song >= 0 &&
            audio_state.current_song < new_playlist->song_count)
            {
                int song_index =
                    new_playlist->song_indices[
                        audio_state.current_song
                    ];

                const char *path =
                    audio_library_get_path(song_index);

                if(path)
                {
                    if(audio_player_play(path) == 0)
                    {
                        audio_state.playing = 1;
                    }
                    else
                    {
                        audio_state.playing = 0;
                    }
                }
            }

            now_playing_refresh();
        }

        return 0;
    }


    /*
     * S = Now Playing options.
     */

    if(key == 's' ||
       key == 'S')
    {
        create_now_playing_options_menu();

        return 0;
    }


    /*
     * Back.
     */

    if(key == 'b' ||
       key == 'B' ||
       key == LV_KEY_ESC)
    {
        return 1;
    }


    return 0;
}

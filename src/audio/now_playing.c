#include "now_playing.h"

#include "audio_library.h"
#include "playlist.h"

#include "../assets/wallpaper_test.h"

#include "lvgl/lvgl.h"

#include <stdio.h>
#include <string.h>



#define SONG_DURATION_SECONDS 210



/*
 * Current playback state
 */
static int current_playlist = 0;
static int current_song = 0;

static int elapsed_seconds = 0;

static int playing = 1;

static int playback_mode = 0;



/*
 * Screen widgets
 */
static lv_obj_t *song_label;

static lv_obj_t *playlist_label;

static lv_obj_t *progress_bar;

static lv_obj_t *time_label;

static lv_obj_t *play_label;

static lv_obj_t *mode_label;



static lv_timer_t *progress_timer = NULL;



static int now_playing_open_flag = 0;



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


void now_playing_open(
    int playlist_index,
    int song_index
)
{
    current_playlist =
        playlist_index;

    current_song =
        song_index;

    elapsed_seconds = 0;

    playing = 1;

    now_playing_open_flag = 1;



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
        SONG_DURATION_SECONDS
    );

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

    Playlist *p =
        playlist_get(
            current_playlist
        );

    if(p)
    {
        lv_label_set_text(
            song_label,
            audio_library_get_name(
                p->song_indices[current_song]
            )
        );

        lv_label_set_text_fmt(
            playlist_label,
            "Playlist: %s",
            p->name
        );
    }



    update_progress();

    update_time_label();

    update_play_button();

    update_mode_label();

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


static void update_play_button(void)
{
    if(playing)
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


static void update_mode_label(void)
{
    switch(playback_mode)
    {
        case 0:
            lv_label_set_text(
                mode_label,
                "Mode: Sequence"
            );
            break;

        case 1:
            lv_label_set_text(
                mode_label,
                "Mode: Shuffle"
            );
            break;

        case 2:
            lv_label_set_text(
                mode_label,
                "Mode: Repeat"
            );
            break;
    }
}


static void update_time_label(void)
{
    int current_minutes =
        elapsed_seconds / 60;

    int current_seconds =
        elapsed_seconds % 60;

    int total_minutes =
        SONG_DURATION_SECONDS / 60;

    int total_seconds =
        SONG_DURATION_SECONDS % 60;


    lv_label_set_text_fmt(
        time_label,
        "%02d:%02d / %02d:%02d",
        current_minutes,
        current_seconds,
        total_minutes,
        total_seconds
    );
}


static void update_progress(void)
{
    lv_bar_set_value(
        progress_bar,
        elapsed_seconds,
        LV_ANIM_OFF
    );
}


void now_playing_refresh(void)
{
    Playlist *p =
        playlist_get(
            current_playlist
        );

    if(!p)
        return;


    lv_label_set_text(
        song_label,
        audio_library_get_name(
            p->song_indices[current_song]
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


static void progress_timer_callback(
    lv_timer_t *timer
)
{
    LV_UNUSED(timer);


    if(!playing)
        return;


    elapsed_seconds++;


    if(elapsed_seconds >
       SONG_DURATION_SECONDS)
    {
        elapsed_seconds =
            SONG_DURATION_SECONDS;

        playing = 0;
    }


    update_progress();

    update_time_label();

    update_play_button();
}


int now_playing_is_open(void)
{
    return
        now_playing_open_flag;
}



void now_playing_close(void)
{
    now_playing_open_flag = 0;


    if(progress_timer)
    {
        lv_timer_delete(
            progress_timer
        );

        progress_timer = NULL;
    }
}


int now_playing_handle_key(
    uint32_t key
)
{
    Playlist *p =
        playlist_get(
            current_playlist
        );

    if(!p)
        return 0;

    if(key == ' ')
    {
        playing = !playing;
        update_play_button();
        return 0;
    }

    if(key == LV_KEY_RIGHT)
    {
        current_song++;

        if(current_song >= p->song_count)
            current_song = 0;

        elapsed_seconds = 0;

        now_playing_refresh();

        return 0;
    }

    if(key == LV_KEY_LEFT)
    {
        current_song--;

        if(current_song < 0)
            current_song =
                p->song_count - 1;

        elapsed_seconds = 0;

        now_playing_refresh();

        return 0;
    }

    if(key == 's' ||
       key == 'S')
    {
        printf("Now Playing options\n");
        return 0;
    }

    if(key == 'b' ||
       key == 'B' ||
       key == LV_KEY_ESC)
    {
        return 1;
    }

    return 0;
}

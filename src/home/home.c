#include "home.h"
#include "../bookshelf/bookshelf.h"
#include "../assets/wallpaper_test.h"
#include "../audio/audio.h"
#include <stdio.h>
#include "../my_custom_ui.h"
static lv_group_t *button_group;
static lv_obj_t *reader_label;
static lv_obj_t *audio_label;

typedef enum
{
    HOME_APP_READER = 0,
    HOME_APP_AUDIO,
    HOME_APP_COUNT

} HomeSelection;

static int selected_app = HOME_APP_READER;

static void open_selected_app(void)
{
    switch(selected_app)
    {
        case HOME_APP_READER:

            ui_open_bookshelf();

            break;


        case HOME_APP_AUDIO:

            ui_open_audio();

            break;


        default:
            break;
    }
}
void home_open(void)
{
    button_group =
        lv_group_get_default();


    lv_obj_clean(
        lv_screen_active()
    );

    lv_obj_t *bg =
    lv_image_create(
        lv_screen_active()
    );

    lv_image_set_src(
        bg,
        &wallpaper_test
    );

    lv_obj_center(bg);

    lv_obj_move_background(bg);

    lv_obj_t *title =
        lv_label_create(
            lv_screen_active()
        );


    lv_label_set_text(
        title,
        "BEAU POD"
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
        30
    );


    lv_obj_t *reader =
        lv_btn_create(
            lv_screen_active()
        );

    lv_obj_set_user_data(
        reader,
        (void *)HOME_APP_READER
    );


    lv_obj_set_size(
        reader,
        150,
        50
    );


    lv_obj_align(
        reader,
        LV_ALIGN_CENTER,
        0,
        0
    );


    reader_label =
    lv_label_create(reader);


    if(selected_app == HOME_APP_READER)
    {
        lv_label_set_text(
            reader_label,
            "> BOOKS"
        );
    }
    else
    {
        lv_label_set_text(
            reader_label,
            "BOOKS"
        );
    }


    lv_obj_center(reader_label);


    lv_group_add_obj(
        button_group,
        reader
    );

    lv_obj_t *audio =
        lv_btn_create(
            lv_screen_active()
        );

        lv_obj_set_user_data(
        audio,
        (void *)HOME_APP_AUDIO
    );

    lv_obj_set_size(
        audio,
        150,
        50
    );


    lv_obj_align(
        audio,
        LV_ALIGN_CENTER,
        0,
        70
    );


    audio_label =
    lv_label_create(audio);


    if(selected_app == HOME_APP_AUDIO)
    {
        lv_label_set_text(
            audio_label,
            "> AUDIO"
        );
    }
    else
    {
        lv_label_set_text(
            audio_label,
            "AUDIO"
        );
    }


    lv_obj_center(audio_label);

    lv_group_add_obj(
        button_group,
        audio
    );
}


static void home_update_selection(void)
{
    if(selected_app == HOME_APP_READER)
    {
        lv_label_set_text(
            reader_label,
            "> BOOKS"
        );

        lv_label_set_text(
            audio_label,
            "AUDIO"
        );
    }
    else
    {
        lv_label_set_text(
            reader_label,
            "BOOKS"
        );

        lv_label_set_text(
            audio_label,
            "> AUDIO"
        );
    }
}


void home_handle_key(uint32_t key)
{
    if(key == LV_KEY_RIGHT ||
       key == LV_KEY_DOWN)
    {
        selected_app++;

        if(selected_app >= HOME_APP_COUNT)
            selected_app = 0;

        home_update_selection();
    }


    else if(key == LV_KEY_LEFT ||
            key == LV_KEY_UP)
    {
        selected_app--;

        if(selected_app < 0)
            selected_app = HOME_APP_COUNT - 1;

        home_update_selection();
    }


    else if(key == ' ')
    {
        printf("HOME selected_app = %d\n", selected_app);
        open_selected_app();
    }
}

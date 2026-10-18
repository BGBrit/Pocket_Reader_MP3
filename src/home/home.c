
#include "home.h"
#include "../bookshelf/bookshelf.h"
#include "../assets/wallpaper_test.h"
#include "../audio/audio.h"
#include "../battery/battery.h"
#include "../my_custom_ui.h"

#include <stdio.h>
#include "../battery/battery.h"

/* Battery UI objects */
static lv_obj_t *battery_outline;
static lv_obj_t *battery_fill;
static lv_obj_t *battery_terminal;
static lv_obj_t *battery_text;
static lv_timer_t *battery_timer;

static lv_group_t *button_group;
static lv_obj_t *reader_label;
static lv_obj_t *audio_label;
static lv_obj_t *battery_label;
static lv_timer_t *battery_timer;

typedef enum
{
    HOME_APP_READER = 0,
    HOME_APP_AUDIO,
    HOME_APP_COUNT
} HomeSelection;

static int selected_app = HOME_APP_READER;


static void home_update_battery_icon(void)
{
    if(!battery_fill || !battery_text)
        return;

    BatteryStatus status = battery_get_status();

    if(!status.is_valid ||
       status.percentage < 0 ||
       status.percentage > 100)
    {
        lv_obj_set_width(battery_fill, 0);
        lv_label_set_text(battery_text, "--%");
        lv_obj_set_style_bg_color(
            battery_fill,
            lv_color_white(),
            0
        );
        return;
    }

    /*
     * Battery body is 30px wide with a 2px border.
     * The usable inner width is 22px.
     */
    int fill_width = (status.percentage * 22) / 100;

    lv_obj_set_width(battery_fill, fill_width);

    lv_color_t fill_color;

    if(status.percentage <= 15)
    {
        fill_color = lv_palette_main(LV_PALETTE_RED);
    }
    else if(status.percentage <= 30)
    {
        fill_color = lv_palette_main(LV_PALETTE_AMBER);
    }
    else
    {
        fill_color = lv_palette_main(LV_PALETTE_GREEN);
    }

    lv_obj_set_style_bg_color(
        battery_fill,
        fill_color,
        0
    );

    char text[12];

    if(status.is_charging)
    {
        snprintf(
            text,
            sizeof(text),
            "%d%% +",
            status.percentage
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "%d%%",
            status.percentage
        );
    }

    lv_label_set_text(battery_text, text);
}

static void home_create_battery_icon(lv_obj_t *parent)
{
    /* Battery outline */
    battery_outline = lv_obj_create(parent);

    lv_obj_set_size(battery_outline, 30, 14);

    lv_obj_set_style_radius(battery_outline, 3, 0);
    lv_obj_set_style_bg_opa(
        battery_outline,
        LV_OPA_TRANSP,
        0
    );
    lv_obj_set_style_border_width(
        battery_outline,
        2,
        0
    );
    lv_obj_set_style_border_color(
        battery_outline,
        lv_color_white(),
        0
    );
    lv_obj_set_style_pad_all(battery_outline, 0, 0);

    lv_obj_clear_flag(
        battery_outline,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /* Battery terminal */
    battery_terminal = lv_obj_create(parent);

    lv_obj_set_size(battery_terminal, 3, 6);
    lv_obj_set_style_radius(battery_terminal, 1, 0);
    lv_obj_set_style_border_width(battery_terminal, 0, 0);
    lv_obj_set_style_bg_color(
        battery_terminal,
        lv_color_white(),
        0
    );
    lv_obj_set_style_pad_all(battery_terminal, 0, 0);

    lv_obj_clear_flag(
        battery_terminal,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /* Green fill inside the battery */
    battery_fill = lv_obj_create(battery_outline);

    lv_obj_set_pos(battery_fill, 2, 2);
    lv_obj_set_size(battery_fill, 17, 6);

    lv_obj_set_style_radius(battery_fill, 1, 0);
    lv_obj_set_style_border_width(battery_fill, 0, 0);
    lv_obj_set_style_bg_color(
        battery_fill,
        lv_palette_main(LV_PALETTE_GREEN),
        0
    );
    lv_obj_set_style_pad_all(battery_fill, 0, 0);

    lv_obj_clear_flag(
        battery_fill,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /* Percentage label */
    battery_text = lv_label_create(parent);

    lv_label_set_text(battery_text, "78%");

    lv_obj_set_style_text_color(
        battery_text,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_font(
        battery_text,
        &lv_font_montserrat_12,
        0
    );

    /* Initial layout */
    lv_obj_align(
        battery_outline,
        LV_ALIGN_TOP_RIGHT,
        -20,
        3
    );

    lv_obj_align_to(
        battery_terminal,
        battery_outline,
        LV_ALIGN_OUT_RIGHT_MID,
        0,
        0
    );

    lv_obj_align_to(
        battery_text,
        battery_outline,
        LV_ALIGN_OUT_LEFT_MID,
        -5,
        0
    );

    home_update_battery_icon();
}


static void home_update_battery_label(void)
{
    if(!battery_label)
        return;

    BatteryStatus status = battery_get_status();

    if(!status.is_valid ||
       status.percentage < 0 ||
       status.percentage > 100)
    {
        lv_label_set_text(battery_label, "BAT --");
        lv_obj_set_style_text_color(
            battery_label,
            lv_color_white(),
            0
        );
        return;
    }

    char text[24];

    if(status.is_charging)
    {
        snprintf(
            text,
            sizeof(text),
            "CHG %d%%",
            status.percentage
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "BAT %d%%",
            status.percentage
        );
    }

    lv_label_set_text(battery_label, text);

    /*
     * Basic charge-level indication.
     * These thresholds are illustrative and can be adjusted.
     */
    if(status.percentage <= 15)
    {
        lv_obj_set_style_text_color(
            battery_label,
            lv_palette_main(LV_PALETTE_RED),
            0
        );
    }
    else if(status.percentage <= 30)
    {
        lv_obj_set_style_text_color(
            battery_label,
            lv_palette_main(LV_PALETTE_AMBER),
            0
        );
    }
    else
    {
        lv_obj_set_style_text_color(
            battery_label,
            lv_color_white(),
            0
        );
    }
}

static void home_battery_timer_callback(lv_timer_t *timer)
{
    (void)timer;

    battery_update();
    home_update_battery_label();
}

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
    button_group = lv_group_get_default();

    /* Clean up the previous battery timer if the home screen is reopened. */
    if(battery_timer)
    {
        lv_timer_delete(battery_timer);
        battery_timer = NULL;
    }

    /* Reset battery UI pointers before removing screen objects. */
    battery_outline = NULL;
    battery_fill = NULL;
    battery_terminal = NULL;
    battery_text = NULL;

    reader_label = NULL;
    audio_label = NULL;

    /* Clear the current screen. */
    lv_obj_clean(lv_screen_active());

    /* Wallpaper */
    lv_obj_t *bg = lv_image_create(lv_screen_active());

    lv_image_set_src(bg, &wallpaper_test);
    lv_obj_center(bg);
    lv_obj_move_background(bg);

    /* Battery indicator */
    battery_init();
    battery_update();

    home_create_battery_icon(lv_screen_active());

    battery_timer = lv_timer_create(
        home_battery_timer_callback,
        30000,
        NULL
    );

    /* Title */
    lv_obj_t *title = lv_label_create(lv_screen_active());

    lv_label_set_text(title, "BEAU POD");

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

    /* Books button */
    lv_obj_t *reader = lv_btn_create(lv_screen_active());

    lv_obj_set_user_data(
        reader,
        (void *)(uintptr_t)HOME_APP_READER
    );

    lv_obj_set_size(reader, 150, 50);

    lv_obj_align(
        reader,
        LV_ALIGN_CENTER,
        0,
        0
    );

    reader_label = lv_label_create(reader);

    if(selected_app == HOME_APP_READER)
    {
        lv_label_set_text(reader_label, "> BOOKS");
    }
    else
    {
        lv_label_set_text(reader_label, "BOOKS");
    }

    lv_obj_center(reader_label);

    lv_group_add_obj(button_group, reader);

    /* Music button */
    lv_obj_t *audio = lv_btn_create(lv_screen_active());

    lv_obj_set_user_data(
        audio,
        (void *)(uintptr_t)HOME_APP_AUDIO
    );

    lv_obj_set_size(audio, 150, 50);

    lv_obj_align(
        audio,
        LV_ALIGN_CENTER,
        0,
        70
    );

    audio_label = lv_label_create(audio);

    if(selected_app == HOME_APP_AUDIO)
    {
        lv_label_set_text(audio_label, "> MUSIC");
    }
    else
    {
        lv_label_set_text(audio_label, "MUSIC");
    }

    lv_obj_center(audio_label);

    lv_group_add_obj(button_group, audio);
}



static void home_update_selection(void)
{
    if(selected_app == HOME_APP_READER)
    {
        lv_label_set_text(reader_label, "> BOOKS");
        lv_label_set_text(audio_label, "MUSIC");
    }
    else
    {
        lv_label_set_text(reader_label, "BOOKS");
        lv_label_set_text(audio_label, "> MUSIC");
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

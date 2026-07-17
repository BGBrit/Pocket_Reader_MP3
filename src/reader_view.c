#include "lvgl/lvgl.h"

#include "reader_view.h"
#include "bookshelf/bookshelf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static EReaderBook *active_book = NULL;

static lv_obj_t *loading_box = NULL;
static lv_obj_t *offset_bar = NULL;
static lv_obj_t *offset_label = NULL;

static lv_obj_t *reader_bg = NULL;
static lv_obj_t *reader_text = NULL;
static lv_obj_t *reader_footer = NULL;
static lv_obj_t *reader_divider = NULL;

static lv_obj_t *bookmark_toast = NULL;
static lv_timer_t *bookmark_timer = NULL;

static lv_obj_t *reader_options_popup = NULL;
static lv_obj_t *reader_options_labels[2];

static int selected_reader_option = 0;

static lv_obj_t *mode_popup = NULL;
static lv_obj_t *mode_title = NULL;
static lv_obj_t *mode_labels[3];
static lv_obj_t *jump_page_popup = NULL;
static lv_obj_t *jump_page_label = NULL;

static int jump_page_selection = 0;

static int selected_mode = 0;

typedef enum
{
    READER_MODE_INDOOR,
    READER_MODE_OUTDOOR,
    READER_MODE_NIGHT

} ReaderMode;
static ReaderMode current_reader_mode =
    READER_MODE_INDOOR;



static void hide_bookmark_toast(lv_timer_t *timer)
{
    if(bookmark_toast)
    {
        lv_obj_delete(bookmark_toast);
        bookmark_toast = NULL;
    }

    bookmark_timer = NULL;
}

static void show_bookmark_saved(void)
{
    if(bookmark_toast)
    {
        lv_obj_delete(bookmark_toast);
    }

    bookmark_toast =
        lv_label_create(
            reader_bg
        );

    lv_label_set_text(
        bookmark_toast,
        "✓ Bookmark Saved"
    );

    lv_obj_set_style_bg_color(
        bookmark_toast,
        lv_color_black(),
        0
    );

    lv_obj_set_style_bg_opa(
        bookmark_toast,
        LV_OPA_70,
        0
    );

    lv_obj_set_style_text_color(
        bookmark_toast,
        lv_color_white(),
        0
    );

    lv_obj_set_style_pad_all(
        bookmark_toast,
        8,
        0
    );

    lv_obj_set_style_radius(
        bookmark_toast,
        8,
        0
    );

    lv_obj_align(
        bookmark_toast,
        LV_ALIGN_BOTTOM_MID,
        0,
        -45
    );

    if(bookmark_timer)
    {
        lv_timer_delete(bookmark_timer);
    }

    bookmark_timer =
        lv_timer_create(
            hide_bookmark_toast,
            1000,
            NULL
        );
}

bool reader_has_popup(void)
{
    return
        reader_options_popup ||
        mode_popup ||
        jump_page_popup;
}

static void update_jump_page_label(void)
{
    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "Page %d / %d",
        jump_page_selection,
        ereader_get_total_pages()
    );

    lv_label_set_text(
        jump_page_label,
        buffer
    );
}

static void create_jump_page_menu(void)
{
    jump_page_popup =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_set_size(
        jump_page_popup,
        190,
        120
    );


    lv_obj_center(
        jump_page_popup
    );


    lv_obj_clear_flag(
        jump_page_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_style_bg_color(
        jump_page_popup,
        lv_color_make(220,220,220),
        0
    );


    lv_obj_t *title =
        lv_label_create(
            jump_page_popup
        );


    lv_label_set_text(
        title,
        "Jump To Page"
    );


    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_make(40, 90, 180),
        0
    );


    jump_page_label =
        lv_label_create(
            jump_page_popup
        );


    lv_obj_align(
        jump_page_label,
        LV_ALIGN_CENTER,
        0,
        0
    );


    jump_page_selection =
    ereader_get_current_page_number();

    update_jump_page_label();
}


static void close_jump_page_menu(void)
{
    if(jump_page_popup)
    {
        lv_obj_delete(
            jump_page_popup
        );

        jump_page_popup = NULL;
        jump_page_label = NULL;
    }
}

static const char *reader_option_name(int option)
{
    switch(option)
    {
        case 0:
            return "Reading Mode";

        case 1:
            return "Jump to Page";
    }

    return "";
}


static void update_reader_options_menu(void)
{
    for(int i = 0; i < 2; i++)
    {
        char buffer[32];


        if(i == selected_reader_option)
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "> %s",
                reader_option_name(i)
            );
        }
        else
        {
            snprintf(
                buffer,
                sizeof(buffer),
                "  %s",
                reader_option_name(i)
            );
        }


        lv_label_set_text(
            reader_options_labels[i],
            buffer
        );
    }
}

static void create_reader_options_menu(void)
{
    reader_options_popup =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_set_size(
        reader_options_popup,
        190,
        130
    );


    lv_obj_center(
        reader_options_popup
    );


    lv_obj_clear_flag(
        reader_options_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_style_bg_color(
        reader_options_popup,
        lv_color_make(220,220,220),
        0
    );


    lv_obj_t *title =
        lv_label_create(
            reader_options_popup
        );


    lv_label_set_text(
        title,
        "Reader Options"
    );


    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_make(40, 90, 180),
        0
    );

    for(int i = 0; i < 2; i++)
    {
        reader_options_labels[i] =
            lv_label_create(
                reader_options_popup
            );


        lv_obj_align(
            reader_options_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            45 + (i * 30)
        );
    }


    selected_reader_option = 0;

    update_reader_options_menu();
}

static void close_reader_options_menu(void)
{
    if(reader_options_popup)
    {
        lv_obj_delete(
            reader_options_popup
        );

        reader_options_popup = NULL;
    }
}


static const char *reader_mode_name(int mode)
{
    switch(mode)
    {
        case READER_MODE_INDOOR:
            return "Indoor";

        case READER_MODE_OUTDOOR:
            return "Outdoor";

        case READER_MODE_NIGHT:
            return "Night";
    }

    return "";
}

void reader_apply_mode(void)
{
    switch(current_reader_mode)
    {
        case READER_MODE_INDOOR:

            lv_obj_set_style_bg_color(
                reader_bg,
                lv_color_make(245,240,225),
                0
            );

            lv_obj_set_style_text_color(
                reader_text,
                lv_color_make(35,35,35),
                0
            );

            lv_obj_set_style_bg_color(
                reader_divider,
                lv_color_make(170,170,170),
                0
            );

            lv_obj_set_style_text_color(
                reader_footer,
                lv_color_make(70,70,70),
                0
            );

            break;



        case READER_MODE_OUTDOOR:

            lv_obj_set_style_bg_color(
                reader_bg,
                lv_color_white(),
                0
            );

            lv_obj_set_style_text_color(
                reader_text,
                lv_color_black(),
                0
            );

            lv_obj_set_style_bg_color(
                reader_divider,
                lv_color_make(80,80,80),
                0
            );

            lv_obj_set_style_text_color(
                reader_footer,
                lv_color_black(),
                0
            );

            break;



        case READER_MODE_NIGHT:

            lv_obj_set_style_bg_color(
                reader_bg,
                lv_color_make(24,24,24),
                0
            );

            lv_obj_set_style_text_color(
                reader_text,
                lv_color_make(220,220,220),
                0
            );

            lv_obj_set_style_bg_color(
                reader_divider,
                lv_color_make(90,90,90),
                0
            );

            lv_obj_set_style_text_color(
                reader_footer,
                lv_color_make(170,170,170),
                0
            );

            break;
    }
}

static void update_reader_mode_menu(void)
{
    for(int i = 0; i < 3; i++)
    {
        if(i == selected_mode)
        {
            char buf[32];

            snprintf(
                buf,
                sizeof(buf),
                "> %s",
                reader_mode_name(i)
            );

            lv_label_set_text(
                mode_labels[i],
                buf
            );
        }
        else
        {
            lv_label_set_text(
                mode_labels[i],
                reader_mode_name(i)
            );
        }
    }
}

static void create_reader_mode_menu(void)
{
    mode_popup =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_set_size(
        mode_popup,
        180,
        150
    );


    lv_obj_center(
        mode_popup
    );


    lv_obj_clear_flag(
        mode_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_style_bg_color(
        mode_popup,
        lv_color_make(220,220,220),
        0
    );


    lv_obj_set_style_border_width(
        mode_popup,
        1,
        0
    );


    mode_title =
        lv_label_create(
            mode_popup
        );


    lv_label_set_text(
        mode_title,
        "Reading Mode"
    );


    lv_obj_align(
        mode_title,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

        lv_obj_set_style_text_color(
        mode_title,
        lv_color_make(40, 90, 180),
        0
    );

    for(int i = 0; i < 3; i++)
    {
        mode_labels[i] =
            lv_label_create(
                mode_popup
            );


        lv_label_set_text(
            mode_labels[i],
            reader_mode_name(i)
        );


        lv_obj_align(
            mode_labels[i],
            LV_ALIGN_TOP_LEFT,
            20,
            40 + (i * 25)
        );
    }


    selected_mode =
        current_reader_mode;


    update_reader_mode_menu();
}


static void close_reader_mode_menu(void)
{
    if(mode_popup)
    {
        lv_obj_delete(
            mode_popup
        );

        mode_popup = NULL;
    }
}

static void create_offset_loading_ui(void)
{
    /*
     * Center box
     */
    loading_box = lv_obj_create(
        lv_screen_active()
    );

    lv_obj_set_size(
        loading_box,
        210,
        100
    );

    lv_obj_center(
        loading_box
    );


    lv_obj_clear_flag(
        loading_box,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_style_bg_color(
        loading_box,
        lv_color_make(230,230,230),
        0
    );


    lv_obj_set_style_border_width(
        loading_box,
        1,
        0
    );


    lv_obj_set_style_radius(
        loading_box,
        8,
        0
    );


    /*
     * Text
     */
    offset_label = lv_label_create(
        loading_box
    );

    lv_label_set_text(
        offset_label,
        "Building page index..."
    );

    lv_obj_align(
        offset_label,
        LV_ALIGN_TOP_MID,
        0,
        10
    );


    /*
     * Progress bar
     */
    offset_bar = lv_bar_create(
        loading_box
    );

    lv_obj_set_size(
        offset_bar,
        170,
        15
    );

    lv_obj_align(
        offset_bar,
        LV_ALIGN_CENTER,
        0,
        15
    );


    lv_bar_set_value(
        offset_bar,
        0,
        LV_ANIM_OFF
    );
}

static void reader_offset_progress_callback(int percent)
{
    printf("OFFSET PROGRESS %d%%\n", percent);

    if(offset_bar)
    {
        lv_bar_set_value(
            offset_bar,
            percent,
            LV_ANIM_OFF
        );
    }

    if(offset_label)
    {
        lv_label_set_text_fmt(
            offset_label,
            "Building page index...\n%d%%",
            percent
        );
    }

    lv_timer_handler();

    lv_refr_now(
        lv_display_get_default()
    );
}

// =====================================================
// Refresh displayed page
// =====================================================

static void reader_refresh(void)
{
    PageResult page =
        ereader_get_page();

    printf(
    "DISPLAY TEXT END=[%s]\n",
    strlen(page.text) > 50 ?
       &page.text[strlen(page.text)-50] :
       page.text
);
    lv_point_t size;

lv_text_get_size(
    &size,
    page.text,
    LV_FONT_DEFAULT,
    0,
    0,
    220,
    LV_TEXT_FLAG_NONE
);

printf(
    "TEXT SIZE w=%d h=%d\n",
    size.x,
    size.y
);

    lv_label_set_text(
        reader_text,
        page.text
    );


    char footer[64];

    snprintf(
        footer,
        sizeof(footer),
        "Page %d     %d%%",
        ereader_get_current_page_number(),
        ereader_get_progress_percent(active_book)
    );


    lv_label_set_text(
        reader_footer,
        footer
    );
}



// =====================================================
// Open reader
// =====================================================

void reader_open(EReaderBook *book)
{
    active_book = book;

    create_offset_loading_ui();

    lv_timer_handler();

    lv_refr_now(
        lv_display_get_default()
    );
    ereader_set_offset_progress_callback(
        reader_offset_progress_callback
    );

    ereader_open_book(
        active_book
    );

    ereader_set_offset_progress_callback(NULL);
    lv_obj_clean(
        lv_screen_active()
    );



    reader_bg =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_set_size(
        reader_bg,
        240,
        320
    );

    lv_obj_clear_flag(
        reader_bg,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_set_scrollbar_mode(
        reader_bg,
        LV_SCROLLBAR_MODE_OFF
    );

    lv_obj_set_style_radius(reader_bg, 0, 0);

    lv_obj_set_style_border_width(reader_bg, 0, 0);

    lv_obj_set_style_bg_color(
        reader_bg,
        lv_color_make(250,248,245),
        0
    );



    reader_text =
        lv_label_create(reader_bg);

    lv_obj_set_size(
        reader_text,
        220,
        240
    );


    lv_label_set_long_mode(
        reader_text,
        LV_LABEL_LONG_WRAP
    );


    lv_obj_align(
        reader_text,
        LV_ALIGN_TOP_MID,
        0,
        12
    );



    reader_footer =
        lv_label_create(reader_bg);

    lv_obj_t *reader_divider =
        lv_obj_create(reader_bg);

    lv_obj_set_size(
        reader_divider,
        220,
        1
    );

    lv_obj_align(
        reader_divider,
        LV_ALIGN_BOTTOM_MID,
        0,
        -28
    );

    lv_obj_set_style_radius(
        reader_divider,
        0,
        0
    );

    lv_obj_set_style_pad_all(
        reader_divider,
        0,
        0
    );

    lv_obj_align(
        reader_footer,
        LV_ALIGN_BOTTOM_MID,
        0,
        -8
    );

    /*
     * Apply current reading mode
     * after all reader objects exist
     */
    //current_reader_mode = READER_MODE_NIGHT;
    //current_reader_mode = READER_MODE_INDOOR;
    //current_reader_mode = READER_MODE_OUTDOOR;
    reader_apply_mode();

    reader_refresh();
}



// =====================================================
// Reader controls
// =====================================================

void reader_handle_key(uint32_t key)
{
    if(jump_page_popup)
    {
        if(key == LV_KEY_RIGHT)
        {
            if(jump_page_selection <
            ereader_get_total_pages() - 20)
            {
                jump_page_selection = jump_page_selection + 20;
                update_jump_page_label();
            }
        }

        else if(key == LV_KEY_LEFT)
        {
            if(jump_page_selection - 20 >= 0)
            {
                jump_page_selection = jump_page_selection - 20;
                update_jump_page_label();
            }
        }

        else if(key == ' ')
        {
            ereader_jump_to_page(
                jump_page_selection
            );

            close_jump_page_menu();

            reader_refresh();
        }

        else if(
            key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC
        )
        {
            close_jump_page_menu();
        }

        return;
    }
    if(reader_options_popup)
    {
        if(key == LV_KEY_LEFT)
        {
            if(selected_reader_option > 0)
                selected_reader_option--;

            update_reader_options_menu();
        }


        else if(key == LV_KEY_RIGHT)
        {
            if(selected_reader_option < 1)
                selected_reader_option++;

            update_reader_options_menu();
        }


        else if(key == ' ')
        {
            if(selected_reader_option == 0)
            {
                close_reader_options_menu();

                create_reader_mode_menu();
            }
            else if(selected_reader_option == 1)
            {
                close_reader_options_menu();

                create_jump_page_menu();
            }
        }


        else if(
            key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC
        )
        {
            close_reader_options_menu();
        }


        return;
    }
    if(mode_popup)
    {
        if(key == LV_KEY_LEFT)
        {
            if(selected_mode > 0)
                selected_mode--;

            update_reader_mode_menu();
        }


        else if(key == LV_KEY_RIGHT)
        {
            if(selected_mode < 2)
                selected_mode++;

            update_reader_mode_menu();
        }


        else if(key == ' ')
        {
            current_reader_mode =
                selected_mode;

            reader_apply_mode();

            close_reader_mode_menu();
        }


        else if(
            key == LV_KEY_ESC ||
            key == 'b' ||
            key == 'B'
        )
        {
            close_reader_mode_menu();
        }


        return;
    }
    if(key == LV_KEY_RIGHT)
    {
        PageResult page =
            ereader_get_page();


        ereader_next_page();


        reader_refresh();
    }



    else if(key == LV_KEY_LEFT)
    {
        ereader_prev_page();

        reader_refresh();
    }



    else if(key == ' ')
    {
        ereader_save_bookmark(
            active_book
        );
        bookshelf_save_books();

        //bookshelf_save_books();
        show_bookmark_saved();
        printf("Bookmark saved\n");
    }
    else if(key == 's' || key == 'S')
    {
        create_reader_options_menu();
    }
}



// =====================================================
// Close reader
// =====================================================

void reader_close(void)
{
    ereader_close_book();

    active_book = NULL;
}

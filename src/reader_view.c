#include "lvgl/lvgl.h"

#include "reader_view.h"
#include "bookshelf/bookshelf.h"

#include <stdio.h>
#include <string.h>


static EReaderBook *active_book = NULL;


static lv_obj_t *reader_text;

static lv_obj_t *reader_footer;

static lv_obj_t *loading_box = NULL;
static lv_obj_t *offset_bar = NULL;
static lv_obj_t *offset_label = NULL;


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



    lv_obj_t *bg =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_set_size(
        bg,
        240,
        320
    );

    lv_obj_clear_flag(
        bg,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_set_scrollbar_mode(
        bg,
        LV_SCROLLBAR_MODE_OFF
    );

    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_set_style_border_width(bg, 0, 0);

    lv_obj_set_style_bg_color(
        bg,
        lv_color_make(250,248,245),
        0
    );



    reader_text =
        lv_label_create(bg);

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
        lv_label_create(bg);

    lv_obj_t *divider =
        lv_obj_create(bg);

    lv_obj_set_size(
        divider,
        220,
        1
    );

    lv_obj_align(
        divider,
        LV_ALIGN_BOTTOM_MID,
        0,
        -28
    );

    lv_obj_set_style_radius(
        divider,
        0,
        0
    );

    lv_obj_set_style_pad_all(
        divider,
        0,
        0
    );

    lv_obj_align(
        reader_footer,
        LV_ALIGN_BOTTOM_MID,
        0,
        -8
    );


    reader_refresh();
}



// =====================================================
// Reader controls
// =====================================================

void reader_handle_key(uint32_t key)
{

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

        printf("Bookmark saved\n");
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

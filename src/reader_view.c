#include "lvgl/lvgl.h"

#include "reader_view.h"
#include "bookshelf/bookshelf.h"

#include <stdio.h>


static EReaderBook *active_book = NULL;


static lv_obj_t *reader_text;

static lv_obj_t *reader_footer;



// =====================================================
// Refresh displayed page
// =====================================================

static void reader_refresh(void)
{
    PageResult page =
        ereader_get_page();


    lv_label_set_text(
        reader_text,
        page.text
    );


    char footer[64];

    snprintf(
        footer,
        sizeof(footer),
        "Page %d   %d%%",
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


    ereader_open_book(
        active_book
    );


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



    reader_text =
        lv_label_create(bg);


    lv_obj_set_width(
        reader_text,
        220
    );


    lv_label_set_long_mode(
        reader_text,
        LV_LABEL_LONG_WRAP
    );


    lv_obj_align(
        reader_text,
        LV_ALIGN_TOP_MID,
        0,
        10
    );



    reader_footer =
        lv_label_create(bg);


    lv_obj_align(
        reader_footer,
        LV_ALIGN_BOTTOM_MID,
        0,
        -5
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


        ereader_next_page(
            page.bytes_used
        );


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

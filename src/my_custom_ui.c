#include "lvgl/lvgl.h"

#include "home/home.h"
#include "bookshelf/bookshelf.h"
#include "reader_view.h"

#include <stdio.h>
#include <stdint.h>


typedef enum
{
    APP_HOME,
    APP_BOOKSHELF,
    APP_READER

} AppState;


static AppState current_app = APP_HOME;


static lv_group_t *button_group;

static lv_obj_t *key_receiver;

static bool reader_page_fits(const char *text)
{
    lv_point_t size;

    lv_text_get_size(
        &size,
        text,
        LV_FONT_DEFAULT,
        0,
        0,
        220,
        LV_TEXT_FLAG_NONE
    );

    return size.y <= 250;
}

// =====================================================
// Forward declarations
// =====================================================

static void ui_key_handler(lv_event_t *e);

static void create_key_receiver(void);



// =====================================================
// App switching
// =====================================================

static void open_home(void)
{
    current_app = APP_HOME;

    home_open();

    lv_group_focus_obj(key_receiver);
}


static void open_bookshelf(void)
{
    current_app = APP_BOOKSHELF;

    bookshelf_open();

    lv_group_focus_obj(key_receiver);
}



// =====================================================
// Keyboard routing
// =====================================================

static void ui_key_handler(lv_event_t *e)
{
    uint32_t key =
        lv_event_get_key(e);


    printf("UI KEY: %d\n", key);



    switch(current_app)
    {

        case APP_HOME:

            if(key == ' ')
            {
                ereader_set_page_fits_callback(
        reader_page_fits
    );
                bookshelf_load_books();
                open_bookshelf();
            }
            else
            {
                home_handle_key(key);
            }

            break;



        case APP_BOOKSHELF:

            if(key == 'b' ||
               key == 'B' ||
               key == LV_KEY_ESC)
            {
                open_home();
            }
            else if(key == ' ')
            {
                EReaderBook *book =
                    bookshelf_get_selected();

                if(book)
                {
                    current_app = APP_READER;

                     reader_open(book);
                }
            }
            else
            {
                bookshelf_handle_key(key);
            }

            break;

        case APP_READER:
            if(key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC)
            {
                reader_close();

                current_app = APP_BOOKSHELF;

                bookshelf_open();
            }

            else
            {
                reader_handle_key(key);
            }

            break;
    }
}



// =====================================================
// Persistent keyboard object
// =====================================================

static void create_key_receiver(void)
{
    key_receiver =
        lv_obj_create(
            lv_layer_top()
        );


    lv_obj_set_size(
        key_receiver,
        1,
        1
    );


    lv_obj_set_pos(
        key_receiver,
        0,
        0
    );


    lv_obj_add_event_cb(
        key_receiver,
        ui_key_handler,
        LV_EVENT_KEY,
        NULL
    );


    lv_group_add_obj(
        button_group,
        key_receiver
    );


    lv_group_focus_obj(
        key_receiver
    );
}



// =====================================================
// External focus helper
// =====================================================

void ui_focus_keyboard(void)
{
    if(key_receiver)
    {
        lv_group_focus_obj(
            key_receiver
        );
    }
}



// =====================================================
// Startup
// =====================================================

void init_pocket_reader_ui(void)
{
    button_group =
        lv_group_get_default();


    create_key_receiver();


    open_home();
}

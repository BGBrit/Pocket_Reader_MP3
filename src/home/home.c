#include "home.h"
#include "../bookshelf/bookshelf.h"

static lv_group_t *button_group;


typedef enum
{
    HOME_APP_READER = 0,
    HOME_APP_COUNT
} HomeSelection;


static HomeSelection selected_app = HOME_APP_READER;


static void open_selected_app(void)
{
    if(selected_app == HOME_APP_READER)
    {
        bookshelf_open();
    }
}


void home_open(void)
{
    button_group =
        lv_group_get_default();


    lv_obj_clean(
        lv_screen_active()
    );


    lv_obj_t *title =
        lv_label_create(
            lv_screen_active()
        );


    lv_label_set_text(
        title,
        "BEAU POD"
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


    lv_obj_t *label =
        lv_label_create(reader);


    lv_label_set_text(
        label,
        "E-READER"
    );


    lv_obj_center(label);


    lv_group_add_obj(
        button_group,
        reader
    );


    lv_group_focus_obj(
        reader
    );
}


void home_handle_key(uint32_t key)
{
    if(key == ' ')
    {
        open_selected_app();
    }
}

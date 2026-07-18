#include "scroll_list.h"

#include "../assets/wallpaper_test.h"

#include <stdio.h>
#include <string.h>


#define VISIBLE_ITEMS 6


static lv_obj_t *buttons[VISIBLE_ITEMS];

static lv_obj_t *labels[VISIBLE_ITEMS];


static lv_obj_t *list_container;


static int total_items = 0;

static int selected_index = 0;

static int top_index = 0;


static ScrollListTextCallback text_callback;



static void create_row(
    int row
)
{
    buttons[row] =
        lv_btn_create(
            list_container
        );


    lv_obj_set_size(
        buttons[row],
        210,
        35
    );


    lv_obj_align(
        buttons[row],
        LV_ALIGN_TOP_MID,
        0,
        row * 40
    );


    labels[row] =
        lv_label_create(
            buttons[row]
        );


    lv_obj_center(
        labels[row]
    );
}



void scroll_list_create(
    const char *title
)
{
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



    lv_obj_t *label =
        lv_label_create(
            lv_screen_active()
        );


    lv_label_set_text(
        label,
        title
    );


    lv_obj_set_style_text_color(
        label,
        lv_color_white(),
        0
    );


    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_20,
        0
    );


    lv_obj_align(
        label,
        LV_ALIGN_TOP_MID,
        0,
        20
    );


    list_container =
    lv_obj_create(
        lv_screen_active()
    );


    lv_obj_set_size(
        list_container,
        220,
        290
    );


    lv_obj_align(
        list_container,
        LV_ALIGN_TOP_MID,
        0,
        55
    );


    /*
    * Make container transparent
    */
    lv_obj_set_style_bg_opa(
        list_container,
        LV_OPA_TRANSP,
        0
    );


    lv_obj_set_style_border_width(
        list_container,
        0,
        0
    );


    lv_obj_set_style_pad_all(
        list_container,
        0,
        0
    );


    lv_obj_clear_flag(
        list_container,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_set_size(
        list_container,
        220,
        290
    );


    lv_obj_align(
        list_container,
        LV_ALIGN_TOP_MID,
        0,
        55
    );


    lv_obj_clear_flag(
        list_container,
        LV_OBJ_FLAG_SCROLLABLE
    );


    for(int i = 0; i < VISIBLE_ITEMS; i++)
    {
        create_row(i);
    }


    total_items = 0;
    selected_index = 0;
    top_index = 0;
}



void scroll_list_set_count(
    int count
)
{
    total_items = count;

    if(selected_index >= total_items)
        selected_index = 0;

    scroll_list_refresh();
}



void scroll_list_set_text_callback(
    ScrollListTextCallback callback
)
{
    text_callback = callback;

    scroll_list_refresh();
}



void scroll_list_refresh(void)
{
    if(!text_callback)
        return;


    for(int row = 0; row < VISIBLE_ITEMS; row++)
    {
        int index =
            top_index + row;


        if(index < total_items)
        {
            char buffer[256];


            if(index == selected_index)
            {
                snprintf(
                    buffer,
                    sizeof(buffer),
                    "> %s",
                    text_callback(index)
                );
            }
            else
            {
                snprintf(
                    buffer,
                    sizeof(buffer),
                    "%s",
                    text_callback(index)
                );
            }


            lv_label_set_text(
                labels[row],
                buffer
            );


            lv_obj_clear_flag(
                buttons[row],
                LV_OBJ_FLAG_HIDDEN
            );
        }
        else
        {
            lv_obj_add_flag(
                buttons[row],
                LV_OBJ_FLAG_HIDDEN
            );
        }
    }
}



void scroll_list_set_selected(
    int index
)
{
    if(total_items == 0)
        return;


    selected_index = index;


    if(selected_index < top_index)
    {
        top_index = selected_index;
    }


    if(selected_index >= top_index + VISIBLE_ITEMS)
    {
        top_index =
            selected_index -
            VISIBLE_ITEMS +
            1;
    }


    scroll_list_refresh();
}



int scroll_list_get_selected(void)
{
    return selected_index;
}

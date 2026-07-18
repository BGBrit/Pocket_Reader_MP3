#include "audio.h"

#include "lvgl/lvgl.h"

#include <stdio.h>


static lv_group_t *button_group;


void audio_open(void)
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
        "AUDIO"
    );


    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        30
    );


    lv_obj_t *label =
        lv_label_create(
            lv_screen_active()
        );


    lv_label_set_text(
        label,
        "No songs loaded"
    );


    lv_obj_center(label);


    printf("Audio opened\n");
}



void audio_handle_key(uint32_t key)
{
    if(key == ' ')
    {
        printf("Audio select\n");
    }
}

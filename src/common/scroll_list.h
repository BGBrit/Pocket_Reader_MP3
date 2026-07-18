#ifndef SCROLL_LIST_H
#define SCROLL_LIST_H

#include "lvgl/lvgl.h"


typedef const char *(*ScrollListTextCallback)(
    int index
);


void scroll_list_create(
    const char *title
);


void scroll_list_set_count(
    int count
);


void scroll_list_set_text_callback(
    ScrollListTextCallback callback
);


void scroll_list_set_selected(
    int index
);


int scroll_list_get_selected(void);


void scroll_list_refresh(void);


#endif

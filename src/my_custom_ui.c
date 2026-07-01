#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define SDCARD_MOUNT_PATH "/Users/beaubritain/Desktop/fakemicroSD"
#define BOOKS_DIRECTORY   SDCARD_MOUNT_PATH "/books"
#define SONGS_DIRECTORY   SDCARD_MOUNT_PATH "/songs"

typedef enum {
    STATE_HOME,
    STATE_BOOKSHELF,
    STATE_MP3_LIST
} AppState;

static AppState current_state = STATE_HOME;
static lv_group_t * button_group;

void draw_home_page(void);
void draw_bookshelf_page(void);
void draw_songs_page(void);

static void clear_screen(void) {
    lv_obj_clean(lv_screen_active());
    lv_group_remove_all_objs(button_group);
}

static void global_navigation_handler(lv_event_t * e) {
    uint32_t key = lv_event_get_key(e);
    lv_obj_t * target = lv_event_get_target(e);
    int action_id = (intptr_t)lv_obj_get_user_data(target);

    // SPACEBAR OR ENTER -> Confirm Selection
    if(key == ' ' || key == LV_KEY_ENTER) {
        clear_screen();
        if(action_id == 1) { current_state = STATE_BOOKSHELF; draw_bookshelf_page(); }
        if(action_id == 2) { current_state = STATE_MP3_LIST; draw_songs_page(); }
        return;
    }

    // PHYSICAL "B" KEY OR ESCAPE -> Step Backward
    if(key == 'b' || key == 'B' || key == LV_KEY_ESC) {
        if(current_state != STATE_HOME) {
            clear_screen();
            draw_home_page();
        }
        return;
    }

    // ARROW KEYS -> Menu Selection Traversal
    if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
        lv_group_focus_prev(button_group);
    }
    if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
        lv_group_focus_next(button_group);
    }
}

void draw_home_page(void) {
    current_state = STATE_HOME;

    lv_obj_t * bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_bg_color(bg, lv_color_make(60, 64, 67), 0);
    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_t * title = lv_label_create(bg);
    lv_label_set_text(title, "POCKET HOME");
    lv_obj_set_style_text_color(title, lv_color_make(255, 255, 255), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t * reader_btn = lv_btn_create(bg);
    lv_obj_set_size(reader_btn, 160, 45);
    lv_obj_align(reader_btn, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_user_data(reader_btn, (void*)1);
    lv_obj_add_event_cb(reader_btn, global_navigation_handler, LV_EVENT_KEY, NULL);

    lv_obj_t * lbl1 = lv_label_create(reader_btn);
    lv_label_set_text(lbl1, "📖 E-Reader");
    lv_obj_center(lbl1);

    lv_obj_t * mp3_btn = lv_btn_create(bg);
    lv_obj_set_size(mp3_btn, 160, 45);
    lv_obj_align(mp3_btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_user_data(mp3_btn, (void*)2);
    lv_obj_add_event_cb(mp3_btn, global_navigation_handler, LV_EVENT_KEY, NULL);

    lv_obj_t * lbl2 = lv_label_create(mp3_btn);
    lv_label_set_text(lbl2, "🎵 MP3 Player");
    lv_obj_center(lbl2);

    lv_group_add_obj(button_group, reader_btn);
    lv_group_add_obj(button_group, mp3_btn);
    lv_group_focus_obj(reader_btn);
}

void draw_bookshelf_page(void) {
    current_state = STATE_BOOKSHELF;

    lv_obj_t * bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_bg_color(bg, lv_color_make(45, 48, 50), 0);
    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_t * title = lv_label_create(bg);
    lv_label_set_text(title, "BOOKSHELF");
    lv_obj_set_style_text_color(title, lv_color_make(255, 255, 255), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t * list = lv_obj_create(bg);
    lv_obj_set_size(list, 220, 240);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(list, lv_color_make(35, 37, 38), 0);

    lv_obj_t * back_detector = lv_obj_create(bg);
    lv_obj_set_size(back_detector, 0, 0);
    lv_obj_add_event_cb(back_detector, global_navigation_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(button_group, back_detector);

    DIR *dir = opendir(BOOKS_DIRECTORY);
    struct dirent *entry;
    int items_found = 0;

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            // FIX: Correctly check the first character to ignore hidden files
            if (entry->d_name[0] != '.') {
                lv_obj_t * btn = lv_btn_create(list);
                lv_obj_set_width(btn, lv_pct(100));
                lv_obj_add_event_cb(btn, global_navigation_handler, LV_EVENT_KEY, NULL);

                lv_obj_t * lbl = lv_label_create(btn);
                lv_label_set_text(lbl, entry->d_name);

                lv_group_add_obj(button_group, btn);
                if(items_found == 0) lv_group_focus_obj(btn);
                items_found++;
            }
        }
        closedir(dir);
    }

    if (items_found == 0) {
        lv_obj_t * err_lbl = lv_label_create(list);
        lv_label_set_text(err_lbl, "No books found.\nPlace .txt files in:\nDesktop/fakemicroSD/books");
        lv_obj_set_style_text_color(err_lbl, lv_color_make(200, 50, 50), 0);
        lv_group_focus_obj(back_detector);
    }
}

void draw_songs_page(void) {
    current_state = STATE_MP3_LIST;

    lv_obj_t * bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_bg_color(bg, lv_color_make(33, 37, 43), 0);
    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_t * title = lv_label_create(bg);
    lv_label_set_text(title, "SONGS LIST");
    lv_obj_set_style_text_color(title, lv_color_make(255, 255, 255), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t * list = lv_obj_create(bg);
    lv_obj_set_size(list, 220, 240);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(list, lv_color_make(25, 28, 33), 0);

    lv_obj_t * back_detector = lv_obj_create(bg);
    lv_obj_set_size(back_detector, 0, 0);
    lv_obj_add_event_cb(back_detector, global_navigation_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(button_group, back_detector);

    DIR *dir = opendir(SONGS_DIRECTORY);
    struct dirent *entry;
    int items_found = 0;

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                lv_obj_t * btn = lv_btn_create(list);
                lv_obj_set_width(btn, lv_pct(100));
                lv_obj_add_event_cb(btn, global_navigation_handler, LV_EVENT_KEY, NULL);

                lv_obj_t * lbl = lv_label_create(btn);
                lv_label_set_text(lbl, entry->d_name);

                lv_group_add_obj(button_group, btn);
                if(items_found == 0) lv_group_focus_obj(btn);
                items_found++;
            }
        }
        closedir(dir);
    }

    if (items_found == 0) {
        lv_obj_t * err_lbl = lv_label_create(list);
        lv_label_set_text(err_lbl, "No songs found.\nPlace files in:\nDesktop/fakemicroSD/songs");
        lv_obj_set_style_text_color(err_lbl, lv_color_make(200, 50, 50), 0);
        lv_group_focus_obj(back_detector);
    }
}

void init_pocket_reader_ui(void) {
    // Connect into the default focus engine group created by hal.c
    button_group = lv_group_get_default();
    draw_home_page();
}

#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "ereader_engine.h"

// External engine hooks declared inside ereader_engine.c
int   ereader_open_book(const char * file_path);
void  ereader_close_book(void);
char* ereader_get_page_text(void);
int   ereader_next_page(void);
int   ereader_prev_page(void);
int   ereader_get_current_page_number(void);

typedef enum {
    STATE_HOME,
    STATE_BOOKSHELF,
    STATE_READING_VIEW,  // <-- ADD JUST THIS LINE HERE
    STATE_MP3_LIST
} AppState;


// Layout pointer labels
static lv_obj_t * book_text_label;
static lv_obj_t * page_footer_label;

// Forward declaration of the new page screen renderer
void draw_ereader_view_page(void);


#define SDCARD_MOUNT_PATH "/Users/beaubritain/Desktop/fakemicroSD"
#define BOOKS_DIRECTORY   SDCARD_MOUNT_PATH "/books"
#define SONGS_DIRECTORY   SDCARD_MOUNT_PATH "/songs"

static AppState current_state = STATE_HOME;
static lv_group_t * button_group;

void draw_home_page(void);
void draw_bookshelf_page(void);
void draw_songs_page(void);

static void clear_screen(void) {
    lv_obj_clean(lv_screen_active());
    lv_group_remove_all_objs(button_group);
}

static void handle_reader_input(uint32_t key)
{
    int old_page = current_page;

    if (key == LV_KEY_RIGHT) {
        ereader_next_page();
    }
    else if (key == LV_KEY_LEFT) {
        ereader_prev_page();
    }
    else if (key == ' ') {
        ereader_save_bookmark(current_book_path);
        lv_label_set_text(page_footer_label, "Bookmark Saved!");
        return;
    }

    // 🚨 only refresh UI if page actually changed
    if (current_page != old_page) {

        lv_label_set_text(book_text_label, ereader_get_page_text());

        int page = ereader_get_current_page_number();
        int pct  = ereader_get_progress_percent();

        char footer_buf[64];
        snprintf(footer_buf, sizeof(footer_buf),
                 "Page %d (%d%%)",
                 page,
                 pct);

        lv_label_set_text(page_footer_label, footer_buf);
    }
}

static void handle_menu_select(lv_obj_t * target)
{
    int action_id = (intptr_t)lv_obj_get_user_data(target);

    if(current_state == STATE_HOME) {
        clear_screen();
        if(action_id == 1) draw_bookshelf_page();
        if(action_id == 2) draw_songs_page();
    }
    else if(current_state == STATE_BOOKSHELF) {
        lv_obj_t * label = lv_obj_get_child(target, 0);
        const char * filename = lv_label_get_text(label);

        char full_path[512];
        snprintf(full_path, sizeof(full_path),
                 "%s/%s", BOOKS_DIRECTORY, filename);

        if (ereader_open_book(full_path)) {
            clear_screen();
            draw_ereader_view_page();
        }
    }
}

static void handle_menu_navigation(uint32_t key)
{
    if (key == LV_KEY_LEFT || key == LV_KEY_UP)
        lv_group_focus_prev(button_group);

    if (key == LV_KEY_RIGHT || key == LV_KEY_DOWN)
        lv_group_focus_next(button_group);
}

static void global_navigation_handler(lv_event_t * e) {
    uint32_t key = lv_event_get_key(e);
    lv_obj_t * target = lv_event_get_target(e);

    // BACK ACTION LAYER ("B" Key or Escape)
    if(key == 'b' || key == 'B' || key == LV_KEY_ESC) {
        if(current_state == STATE_BOOKSHELF || current_state == STATE_MP3_LIST) {
            clear_screen();
            draw_home_page();
        }
        else if(current_state == STATE_READING_VIEW) {
            ereader_close_book(); // Safely unlock your text file pointer stream
            clear_screen();
            draw_bookshelf_page(); // Pop back out to the library shelf list
        }
        return;
    }

    // LIST AND LAUNCHER TRACKING SCHEME (Menus and App Pickers)
    if (current_state != STATE_READING_VIEW) {
        if(key == ' ' || key == LV_KEY_ENTER) {
            handle_menu_select(target);
            return;
        }

        handle_menu_navigation(key);
    }

    // ACTIVE READING CANVAS ENGINE CONTROLS (Page Turning Loops)
    else if (current_state == STATE_READING_VIEW) {
        handle_reader_input(key);
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


void draw_ereader_view_page(void) {
    current_state = STATE_READING_VIEW;

    // Elegant ivory bone paper background
    lv_obj_t * bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 240, 320);
    lv_obj_set_style_bg_color(bg, lv_color_make(250, 248, 245), 0);
    lv_obj_set_style_radius(bg, 0, 0);

    // Create an invisible focal box so keyboard key presses filter here safely
    lv_obj_t * key_catcher = lv_obj_create(bg);
    lv_obj_set_size(key_catcher, 0, 0);
    lv_obj_add_event_cb(key_catcher, global_navigation_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(button_group, key_catcher);
    lv_group_focus_obj(key_catcher);

    // Dynamic Word-Wrapping Text Box Display
    book_text_label = lv_label_create(bg);

    lv_obj_set_style_text_font(book_text_label, &lv_font_montserrat_14, 0); // Tweaks font to a highly readable, compact size

    lv_obj_set_size(book_text_label, 210, 255);
    lv_label_set_long_mode(book_text_label, LV_LABEL_LONG_WRAP); // Word-wrapper lines
    lv_obj_set_style_text_color(book_text_label, lv_color_make(25, 25, 25), 0);
    lv_obj_align(book_text_label, LV_ALIGN_TOP_MID, 0, 15);

    // Initial textbook content extraction pull from our new file engine
    lv_label_set_text(book_text_label, ereader_get_page_text());

    // Clean page metadata tracker footer layout bar
    page_footer_label = lv_label_create(bg);
    lv_obj_set_style_text_color(page_footer_label, lv_color_make(120, 120, 120), 0);
    lv_obj_align(page_footer_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    int page = ereader_get_current_page_number();
    int pct  = ereader_get_progress_percent();

    char footer_buf[64];

    snprintf(footer_buf, sizeof(footer_buf),
            "Page %d (%d%%)", page, pct);

    lv_label_set_text(page_footer_label, footer_buf);
}

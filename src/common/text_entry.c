#include "text_entry.h"

#include "lvgl/lvgl.h"

#include <stdio.h>
#include <string.h>


#define TEXT_BUFFER_SIZE 64


/*
 * Character set for the selector.
 *
 * Space is included.
 */
static const char character_table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789"
" -_.";


#define CHARACTER_COUNT \
(sizeof(character_table) - 1)



/*
 * UI objects
 */
static lv_obj_t *text_entry_popup = NULL;

static lv_obj_t *title_label = NULL;

static lv_obj_t *text_label = NULL;

static lv_obj_t *character_label = NULL;



/*
 * State
 */
static char current_text[TEXT_BUFFER_SIZE];

static int selected_character = 0;

static TextEntryCallback callback = NULL;

static int is_open = 0;



/*
 * Draw current text
 */
static void update_text_display(void)
{
    if(!text_label)
        return;


    char buffer[TEXT_BUFFER_SIZE + 4];


    snprintf(
        buffer,
        sizeof(buffer),
        "%s_",
        current_text
    );


    lv_label_set_text(
        text_label,
        buffer
    );
}



/*
 * Draw current character
 */
static void update_character_display(void)
{
    if(!character_label)
        return;


    char buffer[32];


    snprintf(
        buffer,
        sizeof(buffer),
        "Add: [%c]",
        character_table[selected_character]
    );


    lv_label_set_text(
        character_label,
        buffer
    );
}



/*
 * Create popup
 */
static void create_text_entry_popup(
    const char *title
)
{
    text_entry_popup =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_set_size(
        text_entry_popup,
        210,
        170
    );


    lv_obj_center(
        text_entry_popup
    );


    lv_obj_clear_flag(
        text_entry_popup,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_style_bg_color(
        text_entry_popup,
        lv_color_make(
            220,
            220,
            220
        ),
        0
    );



    title_label =
        lv_label_create(
            text_entry_popup
        );


    lv_label_set_text(
        title_label,
        title
    );


    lv_obj_align(
        title_label,
        LV_ALIGN_TOP_MID,
        0,
        10
    );


    lv_obj_set_style_text_color(
        title_label,
        lv_color_make(
            40,
            90,
            180
        ),
        0
    );



    text_label =
        lv_label_create(
            text_entry_popup
        );


    lv_obj_align(
        text_label,
        LV_ALIGN_TOP_LEFT,
        20,
        45
    );



    character_label =
        lv_label_create(
            text_entry_popup
        );


    lv_obj_align(
        character_label,
        LV_ALIGN_TOP_LEFT,
        20,
        80
    );



    lv_obj_t *help =
        lv_label_create(
            text_entry_popup
        );


    lv_label_set_text(
        help,
        "SPACE add\n"
        "UP delete\n"
        "S save\n"
        "B cancel"
    );


    lv_obj_align(
        help,
        LV_ALIGN_TOP_LEFT,
        20,
        110
    );


    update_text_display();

    update_character_display();
}



/*
 * Close popup
 */
void text_entry_close(void)
{
    if(text_entry_popup)
    {
        lv_obj_delete(
            text_entry_popup
        );

        text_entry_popup = NULL;
    }


    title_label = NULL;
    text_label = NULL;
    character_label = NULL;


    is_open = 0;
}



/*
 * Open text entry
 */
void text_entry_open(
    const char *title,
    const char *initial_text,
    TextEntryCallback finished_callback
)
{
    memset(
        current_text,
        0,
        sizeof(current_text)
    );


    if(initial_text)
    {
        strncpy(
            current_text,
            initial_text,
            TEXT_BUFFER_SIZE - 1
        );
    }


    selected_character = 0;

    callback = finished_callback;

    is_open = 1;


    create_text_entry_popup(
        title
    );
}



/*
 * Check if active
 */
int text_entry_is_open(void)
{
    return is_open;
}



/*
 * Add selected character
 */
static void add_character(void)
{
    int len =
        strlen(current_text);


    if(len >= TEXT_BUFFER_SIZE - 1)
        return;


    current_text[len] =
        character_table[selected_character];


    current_text[len + 1] =
        '\0';


    update_text_display();
}



/*
 * Remove last character
 */
static void delete_character(void)
{
    int len =
        strlen(current_text);


    if(len == 0)
        return;


    current_text[len - 1] =
        '\0';


    update_text_display();
}



/*
 * Insert space
 */
static void insert_space(void)
{
    int len =
        strlen(current_text);


    if(len >= TEXT_BUFFER_SIZE - 1)
        return;


    current_text[len] =
        ' ';


    current_text[len + 1] =
        '\0';


    update_text_display();
}



/*
 * Save result
 */
static void save_text(void)
{
    if(callback)
    {
        callback(
            current_text
        );
    }


    text_entry_close();
}



/*
 * Keyboard handling
 */
void text_entry_handle_key(
    uint32_t key
)
{
    if(!is_open)
        return;



    if(key == LV_KEY_LEFT)
    {
        selected_character--;


        if(selected_character < 0)
        {
            selected_character =
                CHARACTER_COUNT - 1;
        }


        update_character_display();
    }



    else if(key == LV_KEY_RIGHT)
    {
        selected_character++;


        if(selected_character >= CHARACTER_COUNT)
        {
            selected_character = 0;
        }


        update_character_display();
    }



    else if(key == ' ')
    {
        add_character();
    }



    else if(key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC)
    {
        delete_character();
    }



    else if(key == LV_KEY_DOWN)
    {
        insert_space();
    }



    else if(key == 's' ||
            key == 'S')
    {
        save_text();
    }



    /*else if(key == 'b' ||
            key == 'B' ||
            key == LV_KEY_ESC)
    {
        text_entry_close();
    }*/
}

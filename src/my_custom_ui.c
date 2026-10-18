
#include "lvgl/lvgl.h"

#include "home/home.h"
#include "bookshelf/bookshelf.h"
#include "reader_view.h"
#include "audio/audio.h"
#include "common/text_entry.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// =====================================================
// Application state
// =====================================================

typedef enum
{
    APP_HOME,
    APP_BOOKSHELF,
    APP_READER,
    APP_AUDIO
} AppState;

static AppState current_app = APP_HOME;

static lv_group_t *button_group;
static lv_obj_t *key_receiver;

// =====================================================
// Forward declarations
// =====================================================

static void ui_key_handler(lv_event_t *e);
static void create_key_receiver(void);

static void show_lock_screen(void);
static void show_passcode_screen(void);
static void update_passcode_display(void);
static void handle_lock_key(uint32_t key);
static void unlock_screen(void);

static void lock_screen_sleep_callback(lv_timer_t *timer);
static void reset_lock_sleep_timer(void);

// =====================================================
// Lock screen configuration
// =====================================================

#define POWER_BUTTON_KEY 'p'

#define LOCK_SCREEN_SLEEP_TIMEOUT_MS 3000

#define PASSCODE_FILE \
    "/Users/beaubritain/Documents/c++ projects/Pocket_Reader_MP3/passcode.txt"

typedef enum
{
    LOCKED_SCREEN,
    ENTERING_PASSCODE,
    CREATING_PASSCODE,
    CONFIRMING_PASSCODE
} LockState;

static bool screen_locked = false;
static bool display_sleeping = false;

static LockState lock_state = LOCKED_SCREEN;

static char passcode[5] = "";
static char entered_code[5] = "";
static char new_code[5] = "";

static int digit_position = 0;
static int selected_digit = 0;

static lv_obj_t *lock_overlay = NULL;
static lv_obj_t *lock_title = NULL;
static lv_obj_t *lock_digits = NULL;
static lv_obj_t *lock_hint = NULL;

static lv_timer_t *lock_sleep_timer = NULL;

// =====================================================
// Passcode storage
// =====================================================

static void save_passcode_to_disk(const char *code)
{
    FILE *file = fopen(PASSCODE_FILE, "w");

    if(file)
    {
        fprintf(file, "%s", code);
        fclose(file);

        printf("Passcode saved.\n");
    }
    else
    {
        printf("Failed to save passcode.\n");
    }
}

static void load_passcode_from_disk(void)
{
    FILE *file = fopen(PASSCODE_FILE, "r");

    passcode[0] = '\0';

    if(file)
    {
        if(fgets(passcode, sizeof(passcode), file))
        {
            passcode[strcspn(passcode, "\r\n")] = '\0';
        }

        fclose(file);
    }

    if(strlen(passcode) != 4)
    {
        passcode[0] = '\0';
    }
}

static bool validate_passcode(const char *input)
{
    return strcmp(input, passcode) == 0;
}

// =====================================================
// Lock-screen sleep timer
// =====================================================

static void lock_screen_sleep_callback(lv_timer_t *timer)
{
    (void)timer;

    /*
     * This timer runs only once. LVGL automatically
     * deletes it after the callback finishes, so clear
     * our pointer to avoid trying to delete it again.
     */
    lock_sleep_timer = NULL;

    if(!screen_locked)
        return;

    display_sleeping = true;

    if(lock_title)
        lv_obj_add_flag(lock_title, LV_OBJ_FLAG_HIDDEN);

    if(lock_digits)
        lv_obj_add_flag(lock_digits, LV_OBJ_FLAG_HIDDEN);

    if(lock_hint)
        lv_obj_add_flag(lock_hint, LV_OBJ_FLAG_HIDDEN);

    printf("Lock screen entered sleep mode.\n");

    /*
     * ESP32: turn off the display backlight here.
     * Do not stop audio.
     */
}

static void reset_lock_sleep_timer(void)
{
    if(!screen_locked)
        return;

    if(lock_sleep_timer)
    {
        lv_timer_delete(lock_sleep_timer);
        lock_sleep_timer = NULL;
    }

    lock_sleep_timer = lv_timer_create(
        lock_screen_sleep_callback,
        LOCK_SCREEN_SLEEP_TIMEOUT_MS,
        NULL
    );

    lv_timer_set_repeat_count(lock_sleep_timer, 1);
}

// =====================================================
// Lock-screen UI
// =====================================================

static void create_lock_overlay(void)
{
    if(lock_overlay)
        return;

    /*
     * Use the top layer so the current app page remains
     * alive underneath the lock screen.
     */
    lock_overlay = lv_obj_create(lv_layer_top());

    lv_obj_remove_style_all(lock_overlay);

    lv_obj_set_size(
        lock_overlay,
        LV_PCT(100),
        LV_PCT(100)
    );

    lv_obj_set_pos(lock_overlay, 0, 0);

    lv_obj_set_style_bg_color(
        lock_overlay,
        lv_color_black(),
        0
    );

    lv_obj_set_style_bg_opa(
        lock_overlay,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        lock_overlay,
        0,
        0
    );

    lv_obj_set_style_radius(
        lock_overlay,
        0,
        0
    );

    lv_obj_clear_flag(
        lock_overlay,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /*
     * Title
     */
    lock_title = lv_label_create(lock_overlay);

    lv_obj_set_style_text_color(
        lock_title,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_align(
        lock_title,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_width(
        lock_title,
        LV_PCT(100)
    );

    lv_obj_align(
        lock_title,
        LV_ALIGN_CENTER,
        0,
        -55
    );

    /*
     * PIN digits
     */
    lock_digits = lv_label_create(lock_overlay);

    lv_obj_set_style_text_color(
        lock_digits,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_font(
        lock_digits,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_set_style_text_align(
        lock_digits,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_width(
        lock_digits,
        LV_PCT(100)
    );

    lv_obj_align(
        lock_digits,
        LV_ALIGN_CENTER,
        0,
        0
    );

    /*
     * Instructions
     */
    lock_hint = lv_label_create(lock_overlay);

    lv_obj_set_style_text_color(
        lock_hint,
        lv_color_hex(0xAAAAAA),
        0
    );

    lv_obj_set_style_text_align(
        lock_hint,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_width(
        lock_hint,
        LV_PCT(100)
    );

    lv_label_set_long_mode(
        lock_hint,
        LV_LABEL_LONG_WRAP
    );

    lv_obj_align(
        lock_hint,
        LV_ALIGN_CENTER,
        0,
        65
    );
}

static void show_lock_screen(void)
{
    create_lock_overlay();

    screen_locked = true;
    lock_state = LOCKED_SCREEN;

    digit_position = 0;
    selected_digit = 0;

    entered_code[0] = '\0';
    new_code[0] = '\0';

    display_sleeping = false;

    lv_obj_clear_flag(lock_title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lock_digits, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lock_hint, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(
        lock_title,
        "Screen Locked"
    );

    lv_label_set_text(
        lock_digits,
        ""
    );

    lv_label_set_text(
        lock_hint,
        "Press P to unlock"
    );

    reset_lock_sleep_timer();
}

static void show_passcode_screen(void)
{
    create_lock_overlay();

    display_sleeping = false;

    lv_obj_clear_flag(lock_title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lock_digits, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lock_hint, LV_OBJ_FLAG_HIDDEN);

    digit_position = 0;
    selected_digit = 0;

    entered_code[0] = '\0';
    new_code[0] = '\0';

    if(passcode[0] == '\0')
    {
        lock_state = CREATING_PASSCODE;

        lv_label_set_text(
            lock_title,
            "Create a 4-digit PIN"
        );
    }
    else
    {
        lock_state = ENTERING_PASSCODE;

        lv_label_set_text(
            lock_title,
            "Enter your PIN"
        );
    }

    lv_label_set_text(
        lock_hint,
        "Up/Down: change digit\nSpace: select\nB: previous digit"
    );

    update_passcode_display();

    reset_lock_sleep_timer();
}

static void update_passcode_display(void)
{
    char display[32] = "";
    int i;

    for(i = 0; i < 4; i++)
    {
        char digit[8];

        if(i < digit_position)
        {
            snprintf(
                digit,
                sizeof(digit),
                " %c  ",
                entered_code[i]
            );
        }
        else if(i == digit_position)
        {
            snprintf(
                digit,
                sizeof(digit),
                "[%d] ",
                selected_digit
            );
        }
        else
        {
            snprintf(
                digit,
                sizeof(digit),
                " _  "
            );
        }

        strncat(
            display,
            digit,
            sizeof(display) - strlen(display) - 1
        );
    }

    lv_label_set_text(
        lock_digits,
        display
    );
}


void ui_focus_keyboard(void)
{
    if(key_receiver)
    {
        lv_group_focus_obj(key_receiver);
    }
}

// =====================================================
// Unlock
// =====================================================

static void unlock_screen(void)
{
    screen_locked = false;
    display_sleeping = false;

    if(lock_sleep_timer)
    {
        lv_timer_delete(lock_sleep_timer);
        lock_sleep_timer = NULL;
    }

    if(lock_overlay)
    {
        lv_obj_del(lock_overlay);

        lock_overlay = NULL;
        lock_title = NULL;
        lock_digits = NULL;
        lock_hint = NULL;
    }

    /*
     * Restore keyboard focus. The underlying page
     * was never destroyed or reopened.
     */
    ui_focus_keyboard();

    printf("Screen unlocked.\n");
}

// =====================================================
// Lock-screen keyboard handling
// =====================================================

static void handle_lock_key(uint32_t key)
{
    /*
     * When asleep, only P wakes the PIN prompt.
     */
    if(display_sleeping)
    {
        if(key == POWER_BUTTON_KEY)
        {
            show_passcode_screen();
        }

        return;
    }

    /*
     * P while locked returns to the lock screen.
     */
    if(key == POWER_BUTTON_KEY)
    {
        show_lock_screen();
        return;
    }

    /*
     * Button activity resets the sleep timer.
     */
    reset_lock_sleep_timer();

    /*
     * Up/Right increments the selected digit.
     */
    if(key == LV_KEY_UP || key == LV_KEY_RIGHT)
    {
        selected_digit = (selected_digit + 1) % 10;

        update_passcode_display();
        return;
    }

    /*
     * Down/Left decrements the selected digit.
     */
    if(key == LV_KEY_DOWN || key == LV_KEY_LEFT)
    {
        selected_digit = (selected_digit + 9) % 10;

        update_passcode_display();
        return;
    }

    /*
     * B/Escape moves back one digit.
     */
    if(key == 'b' ||
       key == 'B' ||
       key == LV_KEY_ESC)
    {
        if(digit_position > 0)
        {
            digit_position--;

            entered_code[digit_position] = '\0';

            update_passcode_display();
        }

        return;
    }

    /*
     * Space confirms the selected digit.
     */
    if(key != ' ')
        return;

    entered_code[digit_position] =
        (char)('0' + selected_digit);

    digit_position++;

    if(digit_position < 4)
    {
        selected_digit = 0;

        update_passcode_display();
        return;
    }

    entered_code[4] = '\0';

    /*
     * Create a new PIN.
     */
    if(lock_state == CREATING_PASSCODE)
    {
        strcpy(new_code, entered_code);

        lock_state = CONFIRMING_PASSCODE;

        digit_position = 0;
        selected_digit = 0;

        entered_code[0] = '\0';

        lv_label_set_text(
            lock_title,
            "Confirm your PIN"
        );

        update_passcode_display();
        return;
    }

    /*
     * Confirm the newly created PIN.
     */
    if(lock_state == CONFIRMING_PASSCODE)
    {
        if(strcmp(entered_code, new_code) == 0)
        {
            strcpy(passcode, new_code);

            save_passcode_to_disk(passcode);

            unlock_screen();
        }
        else
        {
            lv_label_set_text(
                lock_title,
                "PINs did not match"
            );

            digit_position = 0;
            selected_digit = 0;

            entered_code[0] = '\0';
            new_code[0] = '\0';

            lock_state = CREATING_PASSCODE;

            lv_label_set_text(
                lock_hint,
                "Create a new PIN\nUp/Down: change digit"
            );

            update_passcode_display();
        }

        return;
    }

    /*
     * Validate an existing PIN.
     */
    if(lock_state == ENTERING_PASSCODE)
    {
        if(validate_passcode(entered_code))
        {
            unlock_screen();
        }
        else
        {
            lv_label_set_text(
                lock_title,
                "Incorrect PIN"
            );

            digit_position = 0;
            selected_digit = 0;

            entered_code[0] = '\0';

            lv_label_set_text(
                lock_hint,
                "Try again\nUp/Down: change digit\nSpace: select"
            );

            update_passcode_display();
        }
    }
}

// =====================================================
// App switching
// =====================================================

void ui_open_bookshelf(void)
{
    current_app = APP_BOOKSHELF;

    bookshelf_load_books();
    bookshelf_open();

    lv_group_focus_obj(key_receiver);
}

void ui_open_audio(void)
{
    static int initialized = 0;

    current_app = APP_AUDIO;

    if(!initialized)
    {
        audio_init();
        initialized = 1;
    }

    audio_open();

    lv_group_focus_obj(key_receiver);
}

void open_home(void)
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
    uint32_t key = lv_event_get_key(e);

    /*
     * Locked mode receives all keyboard input first.
     */
    if(screen_locked)
    {
        handle_lock_key(key);
        return;
    }

    /*
     * Press P to lock from any app page.
     */
    if(key == POWER_BUTTON_KEY)
    {
        show_lock_screen();
        return;
    }

    printf("UI KEY: %d\n", key);

    /*
     * Text entry owns the keyboard while open.
     */
    if(text_entry_is_open())
    {
        text_entry_handle_key(key);
        return;
    }

    switch(current_app)
    {
        case APP_HOME:
        {
            home_handle_key(key);
            break;
        }

        case APP_BOOKSHELF:
        {
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
        }

        case APP_READER:
        {
            if((key == 'b' ||
                key == 'B' ||
                key == LV_KEY_ESC) &&
               !reader_has_popup())
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

        case APP_AUDIO:
        {
            audio_handle_key(key);
            break;
        }
    }
}


// =====================================================
// Persistent keyboard object
// =====================================================

static void create_key_receiver(void)
{
    key_receiver = lv_obj_create(lv_layer_top());

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


// =====================================================
// Startup
// =====================================================

void init_pocket_reader_ui(void)
{
    button_group = lv_group_get_default();

    create_key_receiver();
    load_passcode_from_disk();

    open_home();

    /*
     * Start in locked mode on initial application startup.
     */
    show_lock_screen();

    ui_focus_keyboard();
}

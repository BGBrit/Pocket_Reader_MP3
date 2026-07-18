#include "bookshelf.h"

#include "../lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../assets/wallpaper_test.h"
#include "../common/scroll_list.h"
#include "../my_custom_ui.h"
#define MAX_BOOKS 256

#define BOOKS_DIRECTORY \
"/Users/beaubritain/Desktop/fakemicroSD/books"

#define OFFSETS_DIRECTORY "/Users/beaubritain/Desktop/fakemicroSD/offsets"

#define BOOK_DB_FILE \
"./books.db"

static EReaderBook books[MAX_BOOKS];

static int book_count = 0;

static int selected_book = 0;


static lv_group_t *button_group;

static int file_exists(const char *path)
{
    struct stat buffer;

    return (
        stat(path, &buffer) == 0
    );
}

void bookshelf_save_books(void)
{
    FILE *fp = fopen(BOOK_DB_FILE, "w");

    if(!fp)
        return;

    for(int i = 0; i < book_count; i++)
    {
        fprintf(
            fp,
            "%s|%d\n",
            books[i].path,
            books[i].bookmark_page
        );
    }

    fclose(fp);
}

void bookshelf_load_books(void)
{
    FILE *fp = fopen(BOOK_DB_FILE, "r");

    EReaderBook saved[MAX_BOOKS];
    int saved_count = 0;


    if(fp)
    {
        char line[1024];

        while(fgets(line, sizeof(line), fp))
        {
            char *sep = strchr(line, '|');

            if(!sep)
                continue;


            *sep = '\0';


            memset(
                &saved[saved_count],
                0,
                sizeof(EReaderBook)
            );


            strncpy(
                saved[saved_count].path,
                line,
                sizeof(saved[saved_count].path)-1
            );


            saved[saved_count].bookmark_page =
                atoi(sep + 1);


            saved_count++;


            if(saved_count >= MAX_BOOKS)
                break;
        }

        fclose(fp);
    }


    book_count = 0;


    DIR *dir = opendir(BOOKS_DIRECTORY);

    if(!dir)
        return;


    struct dirent *entry;


    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_name[0] == '.')
            continue;

        const char *ext = strrchr(entry->d_name, '.');

        if(ext == NULL || strcmp(ext, ".txt") != 0)
            continue;

        if(book_count >= MAX_BOOKS)
            break;


        char full_path[512];


        snprintf(
            full_path,
            sizeof(full_path),
            "%s/%s",
            BOOKS_DIRECTORY,
            entry->d_name
        );


        memset(
            &books[book_count],
            0,
            sizeof(EReaderBook)
        );


        strncpy(
            books[book_count].path,
            full_path,
            sizeof(books[book_count].path)-1
        );


        /*
         * Restore bookmark
         */
        for(int i = 0; i < saved_count; i++)
        {
            if(strcmp(saved[i].path, full_path) == 0)
            {
                books[book_count].bookmark_page =
                    saved[i].bookmark_page;

                break;
            }
        }


        const char *filename =
            strrchr(
                books[book_count].path,
                '/'
            );

        if(filename)
            filename++;
        else
            filename = books[book_count].path;


        snprintf(
            books[book_count].offset_path,
            sizeof(books[book_count].offset_path),
            "%s/%s.offsets",
            OFFSETS_DIRECTORY,
            filename
        );


        printf(
            "Loaded book %d: %s\n",
            book_count,
            books[book_count].path
        );


        book_count++;
    }


    closedir(dir);


    if(selected_book >= book_count)
        selected_book = 0;


    printf(
        "Books loaded: %d\n",
        book_count
    );
}

static const char *bookshelf_get_name(int index)
{
    static char name[256];

    const char *file =
        strrchr(
            books[index].path,
            '/'
        );

    if(file)
        file++;
    else
        file = books[index].path;


    snprintf(
        name,
        sizeof(name),
        "%s",
        file
    );


    char *ext =
        strrchr(name,'.');

    if(ext)
        *ext = '\0';


    return name;
}


void bookshelf_open(void)
{
    #ifdef _WIN32
        _mkdir(OFFSETS_DIRECTORY);
    #else
        mkdir(OFFSETS_DIRECTORY, 0755);
    #endif

    button_group =
        lv_group_get_default();


    lv_obj_clean(
        lv_screen_active()
    );

    /*
    * Background image
    */
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

    lv_obj_clear_flag(
        bg,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_t *title =
        lv_label_create(
            lv_screen_active()
        );


    lv_label_set_text(
        title,
        "BOOKSHELF"
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        20
    );


    scroll_list_create("BOOKSHELF");

    scroll_list_set_count(book_count);

    scroll_list_set_text_callback(
        bookshelf_get_name
    );

    scroll_list_set_selected(
        selected_book
    );


    ui_focus_keyboard();

    if(book_count > 0)
    {
        printf("Selected book index: %d\n", selected_book);
        lv_group_focus_obj(
            lv_group_get_obj_by_index(
                button_group,
                0
            )
        );
    }
}



void bookshelf_handle_key(uint32_t key)
{
    printf("BOOKSHELF KEY: %d\n", key);
    if(key == LV_KEY_RIGHT)
    {
        selected_book++;

        if(selected_book >= book_count)
            selected_book = 0;

        bookshelf_open();
    }


    else if(key == LV_KEY_LEFT)
    {
        selected_book--;

        if(selected_book < 0)
            selected_book = book_count - 1;

        bookshelf_open();
    }
    else if(key == ' ')
    {
        EReaderBook *book =
            bookshelf_get_selected();


        if(book)
        {
            printf("Opening: %s\n",
                   book->path);

            // reader_open(book) later
        }
    }
}



EReaderBook *bookshelf_get_selected(void)
{
    if(book_count == 0)
        return NULL;


    return &books[selected_book];
}

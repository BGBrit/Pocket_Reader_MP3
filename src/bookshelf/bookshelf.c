#include "bookshelf.h"

#include "../lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>


#define MAX_BOOKS 256

#define BOOKS_DIRECTORY \
"/Users/beaubritain/Desktop/fakemicroSD/books"

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


        snprintf(
            books[book_count].offset_path,
            sizeof(books[book_count].offset_path),
            "%s.offsets",
            full_path
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

void bookshelf_open(void)
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
        "BOOKSHELF"
    );


    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        20
    );


    for(int i = 0; i < book_count; i++)
    {

        lv_obj_t *btn =
            lv_btn_create(
                lv_screen_active()
            );


        lv_obj_set_size(
            btn,
            210,
            35
        );


        lv_obj_align(
            btn,
            LV_ALIGN_TOP_MID,
            0,
            60 + (i * 40)
        );


        lv_obj_t *label =
            lv_label_create(btn);


        const char *name =
            strrchr(
                books[i].path,
                '/'
            );


        if(name)
            name++;
        else
            name = books[i].path;


        if(i == selected_book)
        {
            char buffer[256];

            snprintf(
                buffer,
                sizeof(buffer),
                "> %s",
                name
            );

            lv_label_set_text(
                label,
                buffer
            );
        }
        else
        {
            lv_label_set_text(
                label,
                name
            );
        }


        lv_obj_center(label);


        lv_group_add_obj(
            button_group,
            btn
        );
    }


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

#include "bookshelf.h"

#include "../lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>


#define MAX_BOOKS 256

#define BOOKS_DIRECTORY \
"/Users/beaubritain/Desktop/fakemicroSD/books"



static EReaderBook books[MAX_BOOKS];

static int book_count = 0;

static int selected_book = 0;


static lv_group_t *button_group;



static void load_books(void)
{
    book_count = 0;


    DIR *dir =
        opendir(BOOKS_DIRECTORY);


    if(!dir)
        return;


    struct dirent *entry;


    while((entry = readdir(dir))
          != NULL)
    {

        if(entry->d_name[0] == '.')
            continue;


        if(book_count >= MAX_BOOKS)
            break;


        snprintf(
            books[book_count].path,
            sizeof(books[book_count].path),
            "%s/%s",
            BOOKS_DIRECTORY,
            entry->d_name
        );


        books[book_count].bookmark_page = 0;
        books[book_count].bookmark_offset = 0;


        book_count++;
    }


    closedir(dir);
}



void bookshelf_open(void)
{
    button_group =
        lv_group_get_default();


    lv_obj_clean(
        lv_screen_active()
    );


    load_books();


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

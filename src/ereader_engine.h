#ifndef EREADER_ENGINE_H
#define EREADER_ENGINE_H

#include <stdio.h>

#define PAGE_SIZE 450

typedef struct
{
    char path[512];

    FILE *file;

    int current_page;

    long *page_bookmarks;
    int page_bookmarks_capacity;

    char page_buffer[PAGE_SIZE + 1];

} EReaderBook;

// Function declarations
void ereader_init_book(EReaderBook *book);

int ereader_open_book(EReaderBook *book,const char *file_path);

char *ereader_get_page_text(EReaderBook *book);

int ereader_next_page(EReaderBook *book);
int ereader_prev_page(EReaderBook *book);

int ereader_get_current_page_number(EReaderBook *book);

int ereader_get_progress_percent(EReaderBook *book);

void ereader_save_bookmark(EReaderBook *book);

void  ereader_close_book(void);

// Global variables (declared as extern)
extern char current_book_path[512];
extern int current_page;

#endif // EREADER_ENGINE_H

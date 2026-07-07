#ifndef EREADER_ENGINE_H
#define EREADER_ENGINE_H

#include <stddef.h>

#define PAGE_SIZE 450


#ifdef __cplusplus
extern "C" {
#endif


// ======================
// Book information
// ======================

typedef struct {

    char path[512];

    // Saved position
    int bookmark_page;
    long bookmark_offset;

} EReaderBook;



// ======================
// Render result
// ======================

typedef struct {

    char text[PAGE_SIZE + 1];

    // UI passes this back to next_page()
    size_t bytes_used;

} PageResult;



// ======================
// Initialization
// ======================

void ereader_init_book(EReaderBook *book);



// ======================
// Book loading
// ======================

int ereader_open_book(EReaderBook *book);

void ereader_close_book(void);



// ======================
// Rendering
// ======================
// Only function UI calls to display text

PageResult ereader_get_page(void);



// ======================
// Navigation
// ======================
// These DO NOT render.
// They only update engine position.

void ereader_next_page(size_t bytes_used);

void ereader_prev_page(void);



// ======================
// Bookmark
// ======================

void ereader_save_bookmark(EReaderBook *book);



// ======================
// Persistent bookshelf
// ======================

void ereader_load_books(
    EReaderBook *books,
    int *count,
    int max
);


void ereader_save_books(
    EReaderBook *books,
    int count
);



// ======================
// Progress
// ======================

int ereader_get_progress_percent(EReaderBook *book);



#ifdef __cplusplus
}
#endif


#endif

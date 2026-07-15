#ifndef EREADER_ENGINE_H
#define EREADER_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define PAGE_SIZE 450


#ifdef __cplusplus
extern "C" {
#endif


// ======================
// Book information
// ======================

typedef struct
{
    char path[512];

    char offset_path[512];

    int bookmark_page;

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
void ereader_jump_to_page(int page);
int ereader_get_total_pages(void);

void ereader_init_book(EReaderBook *book);

typedef bool (*PageFitsCallback)(const char *text);

void ereader_set_page_fits_callback(PageFitsCallback callback);

typedef void (*OffsetProgressCallback)(int percent);

void ereader_set_offset_progress_callback(
    OffsetProgressCallback callback
);

// ======================
// Book loading
// ======================

int ereader_open_book(EReaderBook *book);

void ereader_close_book(void);

int ereader_build_offsets(EReaderBook *book);

int ereader_load_offsets(EReaderBook *book);

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

void ereader_next_page(void);

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

int ereader_get_current_page_number(void);

#ifdef __cplusplus
}
#endif


#endif

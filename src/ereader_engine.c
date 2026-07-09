#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ereader_engine.h"

#define PAGE_SIZE 450
#define MAX_PAGES 10000

// ======================
// Runtime State
// ======================
static int current_page = 0;
static long current_offset = 0;
static FILE *book_file = NULL;

// offset history for fast prev/seek
static long page_offsets[MAX_PAGES];
static int page_count = 0;

// ======================
// Page Result Builder
// ======================
static PageResult empty_page(void)
{
    PageResult p = {0};
    return p;
}

static PageFitsCallback page_fits_callback = NULL;


void ereader_set_page_fits_callback(PageFitsCallback callback)
{
    page_fits_callback = callback;
}

// ======================
// UTF8 / formatting helpers
// ======================
static int sanitize_character(const char *src, uint32_t i,
                              char *dest, uint32_t *widx)
{
    if (!src || src[i] == '\0')
        return 0;

    if ((unsigned char)src[i] != 0xE2)
        return 0;

    unsigned char b1 = (unsigned char)src[i + 1];
    unsigned char b2 = (unsigned char)src[i + 2];

    if (b1 != 0x80)
        return 0;

    if (b2 == 0x98 || b2 == 0x99) {
        dest[(*widx)++] = '\'';
        return 3;
    }

    if (b2 == 0x9C || b2 == 0x9D) {
        dest[(*widx)++] = '"';
        return 3;
    }

    if (b2 == 0x94) {
        dest[(*widx)++] = '-';
        return 3;
    }

    return 0;
}

static void handle_newline(const char *raw,
                           uint32_t *r,
                           char *out,
                           uint32_t *w,
                           uint32_t *line_count)
{
    int i = *r + 1;
    int nl = 1;

    while (raw[i] == '\n' || raw[i] == '\r') {
        if (raw[i] == '\n') nl++;
        i++;
    }

    if (nl >= 2) {
        out[(*w)++] = '\n';
        if (*w < PAGE_SIZE)
            out[(*w)++] = '\n';

        *r = i - 1;
        *line_count = 0;
    } else {
        if (*line_count < 38) {
            out[(*w)++] = '\n';
            *line_count = 0;
        } else {
            if (*w > 0 && out[*w - 1] != ' ')
                out[(*w)++] = ' ';
        }
    }
}

// ======================
// CORE PAGE BUILDER
// ======================

static PageResult build_page(const char *raw, size_t bytes_read)
{
    PageResult result = {0};

    uint32_t r = 0;
    uint32_t w = 0;
    uint32_t used = 0;


    while(raw[r] && r < bytes_read)
    {
        uint32_t word_start = r;

        char word[128];
        uint32_t word_len = 0;


        /*
         * Skip spaces/newlines before word
         */
        while(raw[r] == ' ' ||
              raw[r] == '\n' ||
              raw[r] == '\r')
        {
            if(w < PAGE_SIZE - 1)
            {
                result.text[w++] = raw[r];
            }

            r++;
            used++;
        }


        if(!raw[r])
            break;



        word_start = r;


        /*
         * Read word WITHOUT committing offset yet
         */
        while(raw[r] &&
              raw[r] != ' ' &&
              raw[r] != '\n' &&
              raw[r] != '\r')
        {
            if(word_len < sizeof(word)-1)
                word[word_len++] = raw[r];

            r++;
        }

        word[word_len] = '\0';



        /*
         * Test word
         */
        char test[PAGE_SIZE];

        memcpy(
            test,
            result.text,
            w
        );

        memcpy(
            &test[w],
            word,
            word_len
        );

        test[w + word_len] = '\0';



        if(page_fits_callback &&
           !page_fits_callback(test))
        {
            /*
             * Do not consume this word.
             */
            r = word_start;
            break;
        }



        /*
         * Commit word
         */
        memcpy(
            &result.text[w],
            word,
            word_len
        );

        w += word_len;
        used += word_len;



        if(w >= PAGE_SIZE - 2)
            break;
    }


    result.text[w] = '\0';
    result.bytes_used = used;


    printf(
        "PAGE OUTPUT chars=%d bytes=%d\n",
        w,
        used
    );


    return result;
}

// ======================
// FILE RENDER PIPELINE
// ======================
static PageResult render_page_at_offset(long offset)
{
    PageResult result = {0};

    if (!book_file)
        return result;

    fseek(book_file, offset, SEEK_SET);

    char raw[PAGE_SIZE * 2];
    size_t read = fread(raw, 1, sizeof(raw) - 1, book_file);
    raw[read] = '\0';

    return build_page(raw, read);
}

// ======================
// PUBLIC API
// ======================
void ereader_init_book(EReaderBook *book)
{
    memset(book, 0, sizeof(EReaderBook));
}

int ereader_open_book(EReaderBook *book)
{
    book_file = fopen(book->path, "r");
    if (!book_file)
        return 0;

    current_page = book->bookmark_page;
    current_offset = book->bookmark_offset;

    page_count = 0;
    page_offsets[0] = 0;

    long offset = 0;


    for(int i = 0; i < current_page; i++)
    {
        PageResult page =
            render_page_at_offset(offset);


        offset += page.bytes_used;


        if(page_count < MAX_PAGES - 1)
        {
            page_count++;
            page_offsets[page_count] = offset;
        }
    }


    /*
    * Restore current location
    */
    current_offset =
        page_offsets[current_page];

    fseek(book_file, current_offset, SEEK_SET);

    return 1;
}

void ereader_close_book(void)
{
    if (book_file)
        fclose(book_file);

    book_file = NULL;

    current_page = 0;
    current_offset = 0;
    page_count = 0;
}

// ======================
// RENDER (ONLY PLACE UI CALLS THIS)
// ======================
PageResult ereader_get_page(void)
{
    if (!book_file)
        return empty_page();

    PageResult page = render_page_at_offset(current_offset);

    return page;
}

// ======================
// NAVIGATION (NO RENDERING HERE)
// ======================
void ereader_next_page(size_t bytes_used)
{
    if (!book_file)
        return;

    current_page++;
    current_offset += bytes_used;

    if (page_count < MAX_PAGES - 1) {
        page_count++;
        page_offsets[page_count] = current_offset;
    }
}

void ereader_prev_page(void)
{
    if (current_page <= 0)
        return;

    current_page--;

    if (current_page < page_count) {
        current_offset = page_offsets[current_page];
    }
}

// ======================
// BOOKMARK
// ======================
void ereader_save_bookmark(EReaderBook *book)
{
    book->bookmark_page = current_page;
    book->bookmark_offset = current_offset;
}

// ======================
// PROGRESS
// ======================
int ereader_get_progress_percent(EReaderBook *book)
{
    if (!book_file)
        return 0;

    fseek(book_file, 0, SEEK_END);
    long size = ftell(book_file);

    if (size <= 0)
        return 0;

    return (int)((current_offset * 100) / size);
}

int ereader_get_current_page_number(void)
{
    return current_page;
}

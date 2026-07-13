#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ereader_engine.h"
#include <sys/stat.h>
#include "lvgl.h"
#define PAGE_SIZE 450
#define MAX_PAGES 50000

// ======================
// Runtime State
// ======================
static int current_page = 0;
static long current_offset = 0;
static FILE *book_file = NULL;

// offset history for fast prev/seek
static long page_offsets[MAX_PAGES];
static int page_count = 0;

static PageResult build_page(const char *raw, size_t bytes_read);
static PageResult render_page_at_offset(long offset);

static lv_obj_t * offset_progress_bar = NULL;
static lv_obj_t * offset_progress_label = NULL;
static OffsetProgressCallback progress_callback = NULL;

void ereader_set_offset_progress_callback(
    OffsetProgressCallback callback)
{
    progress_callback = callback;
}

// ======================
// Page Result Builder
// ======================
static PageResult empty_page(void)
{
    PageResult p = {0};
    return p;
}

static PageFitsCallback page_fits_callback = NULL;

static int file_exists(const char *path)
{
    FILE *fp = fopen(path, "r");

    if(fp)
    {
        fclose(fp);
        return 1;
    }

    return 0;
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

int ereader_build_offsets(EReaderBook *book)
{

    FILE *build_file = fopen(book->path, "r");

    if(!build_file)
        return 0;

    fseek(build_file, 0, SEEK_END);
    long total_bytes = ftell(build_file);
    rewind(build_file);

    FILE *offset_file = fopen(book->offset_path, "wb");

    if(!offset_file)
    {
        fclose(build_file);
        return 0;
    }


    book_file = build_file;


    long offset = 0;
    int pages_written = 0;


    while(1)
    {
        if (pages_written < 10)
        {
            printf("BUILD OFFSET=%ld\n", offset);
        }

        PageResult page =
            render_page_at_offset(offset);

        if (pages_written < 10)
        {
            printf(
                "RENDER RETURN bytes=%zu chars=%zu\n",
                page.bytes_used,
                strlen(page.text)
            );
        }


        if(page.bytes_used == 0)
            break;


        fwrite(
            &offset,
            sizeof(long),
            1,
            offset_file
        );


        pages_written++;

        offset += page.bytes_used;
        static int last_percent = -1;

        int percent = (int)((offset * 100) / total_bytes);

        if(percent > 100)
            percent = 100;

        if(percent != last_percent)
        {
            last_percent = percent;

            if(progress_callback)
                progress_callback(percent);
        }
    }

    printf(
        "Built %d pages of offsets\n",
        pages_written
    );
    if(progress_callback)
        progress_callback(100);

    printf("before fflush\n");
    fflush(offset_file);

    printf("before fclose offset\n");
    fclose(offset_file);

    printf("before fclose book\n");
    fclose(build_file);

    printf("before clear book_file\n");

    book_file = NULL;

    printf("returning from build\n");

    return 1;
}

int ereader_load_offsets(EReaderBook *book)
{
    FILE *fp =
        fopen(book->offset_path, "rb");


    if(!fp)
        return 0;


    page_count = 0;


    while(page_count < MAX_PAGES &&
          fread(
              &page_offsets[page_count],
              sizeof(long),
              1,
              fp
          ) == 1)
    {
        page_count++;
    }


    fclose(fp);


    printf(
        "Loaded %d offsets\n",
        page_count
    );


    return page_count > 0;
}
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

    while(r < bytes_read && raw[r] != '\0')
    {
        /*
         * Save where this token begins.
         */
        uint32_t token_start = r;

        /*
         * Read leading whitespace.
         */
        char whitespace[64];
        uint32_t ws_len = 0;

        while(r < bytes_read &&
             (raw[r] == ' ' ||
              raw[r] == '\n' ||
              raw[r] == '\r'))
        {
            if(ws_len < sizeof(whitespace)-1)
                whitespace[ws_len++] = raw[r];

            r++;
        }

        if(r >= bytes_read || raw[r] == '\0')
            break;


        /*
         * Read word.
         */
        char word[128];
        uint32_t word_len = 0;

        while(r < bytes_read &&
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
         * Build candidate page.
         */
        char test[PAGE_SIZE + 1];
        uint32_t test_len = 0;

        memcpy(test, result.text, w);
        test_len = w;

        if(test_len + ws_len < PAGE_SIZE)
        {
            memcpy(
                &test[test_len],
                whitespace,
                ws_len
            );
            test_len += ws_len;
        }

        if(test_len + word_len < PAGE_SIZE)
        {
            memcpy(
                &test[test_len],
                word,
                word_len
            );
            test_len += word_len;
        }

        test[test_len] = '\0';


        /*
         * Does entire token fit?
         */
        if(page_fits_callback &&
           !page_fits_callback(test))
        {
            /*
             * Entire token belongs to next page.
             */
            r = token_start;
            break;
        }


        /*
         * Commit whitespace.
         */
        if(ws_len)
        {
            memcpy(
                &result.text[w],
                whitespace,
                ws_len
            );

            w += ws_len;
        }


        /*
         * Commit word.
         */
        memcpy(
            &result.text[w],
            word,
            word_len
        );

        w += word_len;


        if(w >= PAGE_SIZE - 2)
            break;
    }


    result.text[w] = '\0';

    /*
     * r is the exact file position for the next page.
     */
    result.bytes_used = r;

    return result;
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
    book_file =
        fopen(book->path, "r");


    if(!book_file)
        return 0;


    /*
     * Create offsets if this book has never been opened
     */
    if(!file_exists(book->offset_path))
    {
        printf(
            "Building offsets for %s\n",
            book->path
        );


        fclose(book_file);
        book_file = NULL;

        if(!ereader_build_offsets(book))
        {
            printf("Offset build failed\n");
            return 0;
        }
        /*
         * Re-open book after building
         */
        book_file =
            fopen(book->path, "r");


        if(!book_file)
            return 0;
    }


    /*
     * Load offset table
     */
    if(!ereader_load_offsets(book))
    {
        printf("Could not load offsets\n");

        fclose(book_file);
        book_file = NULL;

        return 0;
    }


    /*
     * Restore bookmark
     */
    current_page =
        book->bookmark_page;


    if(current_page >= page_count)
        current_page = 0;


    current_offset =
        page_offsets[current_page];


    fseek(
        book_file,
        current_offset,
        SEEK_SET
    );


    printf(
        "Opened page=%d offset=%ld\n",
        current_page,
        current_offset
    );


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
void ereader_next_page(void)
{
    if(current_page >= page_count - 1)
        return;


    current_page++;


    current_offset =
        page_offsets[current_page];
}

void ereader_prev_page(void)
{
    if(current_page <= 0)
        return;


    current_page--;


    current_offset =
        page_offsets[current_page];
}

// ======================
// BOOKMARK
// ======================
void ereader_save_bookmark(EReaderBook *book)
{
    book->bookmark_page = current_page;
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

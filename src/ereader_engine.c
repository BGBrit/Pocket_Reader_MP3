#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ereader_engine.h"

// Define the global variables
char current_book_path[512] = ""; // Initialize to an empty string

#define PAGE_SIZE 450 // Max characters to display on a 240x320 portrait screen at once
static long *page_bookmarks = NULL;
static int page_bookmarks_capacity = 0;

static FILE * book_file = NULL;
static char current_page_buffer[PAGE_SIZE + 1];

static size_t read_raw_block(char *raw_buffer) {
    size_t bytes_read = fread(raw_buffer, 1, (PAGE_SIZE * 2) - 1, book_file);
    raw_buffer[bytes_read] = '\0';
    return bytes_read;
}

void ereader_init_book(EReaderBook *book)
{
    if (!book)
        return;

    memset(book, 0, sizeof(EReaderBook));
}

static int sanitize_character(const char *src, uint32_t i,
                              char *dest, uint32_t *widx)
{
    // HARD SAFETY CHECK: prevent buffer over-read
    if (!src)
        return 0;

    if (src[i] == '\0')
        return 0;

    // ensure we can safely read i+2
    if ((unsigned char)src[i] != 0xE2)
        return 0;

    // guard next bytes BEFORE accessing them
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

static void handle_newline(const char *raw_buffer,
                           uint32_t *read_idx,
                           char *out,
                           uint32_t *write_idx,
                           uint32_t *line_char_count) {

    int next_idx = *read_idx + 1;
    int consecutive_newlines = 1;

    while (raw_buffer[next_idx] == '\n' || raw_buffer[next_idx] == '\r') {
        if (raw_buffer[next_idx] == '\n') consecutive_newlines++;
        next_idx++;
    }

    if (consecutive_newlines >= 2) {
        out[(*write_idx)++] = '\n';
        if (*write_idx < PAGE_SIZE)
            out[(*write_idx)++] = '\n';

        *read_idx = next_idx - 1;
        *line_char_count = 0;
    } else {
        if (*line_char_count < 38) {
            out[(*write_idx)++] = '\n';
            *line_char_count = 0;
        } else {
            if (*write_idx > 0 && out[*write_idx - 1] != ' ') {
                out[(*write_idx)++] = ' ';
            }
        }
    }
}

static size_t build_page(char *raw_buffer, size_t *bytes_used_out) {

    uint32_t write_idx = 0;
    uint32_t read_idx = 0;
    uint32_t line_char_count = 0;
    uint32_t file_bytes_used = 0;

    while (raw_buffer[read_idx] != '\0' && write_idx < PAGE_SIZE) {

        int consumed = sanitize_character(
            raw_buffer, read_idx, current_page_buffer, &write_idx);

        if (consumed > 0) {
            read_idx += consumed;
            file_bytes_used += consumed;
            line_char_count++;
            continue;
        }

        if (raw_buffer[read_idx] == '\r') {
            read_idx++;
            continue;
        }

        if (raw_buffer[read_idx] == '\n') {
            handle_newline(raw_buffer, &read_idx,
                           current_page_buffer,
                           &write_idx,
                           &line_char_count);
            file_bytes_used++;
            read_idx++;
            continue;
        }

        current_page_buffer[write_idx++] = raw_buffer[read_idx++];
        file_bytes_used++;
        line_char_count++;
    }

    current_page_buffer[write_idx] = '\0';
    *bytes_used_out = file_bytes_used;
    return write_idx;
}

static FILE *open_bookmark_file_rw(void) {
    FILE *f = fopen("bookmark.txt", "r+");
    if (!f) f = fopen("bookmark.txt", "w");
    return f;
}

static FILE *open_bookmark_file_r(void) {
    return fopen("bookmark.txt", "r");
}

static void seek_to_current_page(EReaderBook *book)
{
    if (!book_file) return;

    if (page_bookmarks[book->current_page] == 0 && book->current_page != 0)
        return; // prevents fake seek

    fseek(book_file, page_bookmarks[book->current_page], SEEK_SET);
}
void ereader_save_bookmark(EReaderBook *book)
{
    FILE *file = open_bookmark_file_r();
    FILE *tmp = fopen("bookmark.tmp", "w");

    if (!tmp)
        return;

    char line[1024];
    int found = 0;

    long offset = page_bookmarks[book->current_page];

    if (file) {

        while (fgets(line, sizeof(line), file)) {

            line[strcspn(line, "\n")] = '\0';

            char *p1 = strchr(line, '|');
            char *p2 = p1 ? strchr(p1 + 1, '|') : NULL;

            if (!p1 || !p2) {
                fprintf(tmp, "%s\n", line);
                continue;
            }

            *p1 = '\0';
            *p2 = '\0';

            char *path = line;
            char *page_str = p1 + 1;
            char *offset_str = p2 + 1;

            // 🔥 DEBUG GOES HERE
            printf("COMPARE CHECK\n");
            printf("book path = '%s'\n", book->path);
            printf("file path = '%s'\n", path);
            printf("len book = %zu\n", strlen(book->path));
            printf("len file = %zu\n", strlen(path));

            if (strcmp(path, book->path) == 0) {

                fprintf(tmp, "%s|%d|%ld\n",
                        book->path,
                        book->current_page,
                        offset);

                found = 1;
            }
            else {

                fprintf(tmp, "%s|%s|%s\n",
                        path,
                        page_str,
                        offset_str);
            }
        }

        fclose(file);
    }

    if (!found) {
        fprintf(tmp, "%s|%d|%ld\n",
                book->path,
                book->current_page,
                offset);
    }

    fclose(tmp);

    remove("bookmark.txt");
    rename("bookmark.tmp", "bookmark.txt");
}

static void seek_to_page_number(int target_page, EReaderBook *book)
{
    book->current_page = 0;

    for (int i = 0; i < target_page; i++) {
        ereader_get_page_text(book);
        book->current_page++;
    }
}


void ereader_load_bookmark(EReaderBook *book)
{
    printf("LOADING BOOKMARK FOR: %s\n", book->path);
    FILE *file = open_bookmark_file_r();
    if (!file) {
        printf("No bookmarks found.\n");
        return;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file)) {

        line[strcspn(line, "\n")] = '\0';

        char *p1 = strchr(line, '|');
        if (!p1) continue;

        char *p2 = strchr(p1 + 1, '|');
        if (!p2) continue;

        *p1 = '\0';
        *p2 = '\0';

        char *path = line;
        int page = atoi(p1 + 1);
        long offset = atol(p2 + 1);

        if (strcmp(path, book->path) == 0) {

            book->current_page = page;

            if (book_file) {
                fseek(book_file, offset, SEEK_SET);
            }

            printf("Bookmark loaded: %s page=%d offset=%ld\n",
                   book->path,
                   book->current_page,
                   offset);

            break;
        }
    }

    fclose(file);
}

// ===============================
// BOOK FILE HELPERS (NEW WRAP ONLY)
// ===============================
static FILE *open_book_file(const char *file_path) {
    return fopen(file_path, "r");
}

static void set_current_book_path(const char *file_path) {
    strncpy(current_book_path, file_path, sizeof(current_book_path) - 1);
    current_book_path[sizeof(current_book_path) - 1] = '\0';
}

static void reset_reader_state_after_open(void) {
    memset(current_page_buffer, 0, sizeof(current_page_buffer));
}


int ereader_open_book(EReaderBook *book, const char *file_path)
{
    strncpy(book->path, file_path, sizeof(book->path) - 1);
    book->path[sizeof(book->path) - 1] = '\0';
    page_bookmarks_capacity = 100;
    page_bookmarks = malloc(sizeof(long) * page_bookmarks_capacity);
    page_bookmarks[0] = 0;
    printf("Trying to open: %s\n", book->path);
    book_file = open_book_file(book->path);

    if (!book_file) {
        printf("Error: Could not open book file.\n");
        return 0;
    }

    set_current_book_path(book->path);

    ereader_load_bookmark(book);

    if (book->current_page > 0) {
        seek_to_page_number(book->current_page, book);
    }

    // ONLY runtime reset (single source of truth)
    memset(current_page_buffer, 0, sizeof(current_page_buffer));

    printf("Book opened: %s at page %d\n", book->path, book->current_page);

    return 1;
}

// Closes the file safely when backing out to the shelf menu
void ereader_close_book(void) {
    if (book_file) {
        fclose(book_file);
        book_file = NULL;
    }
}

static void ensure_page_capacity(int needed_page)
{
    if (needed_page < page_bookmarks_capacity)
        return;

    while (needed_page >= page_bookmarks_capacity)
        page_bookmarks_capacity *= 2;

    page_bookmarks = realloc(page_bookmarks,
                              sizeof(long) * page_bookmarks_capacity);
}

char *ereader_get_page_text(EReaderBook *book) {

    if (!book_file)
        return "Error: No book file loaded.";

    printf("PAGE=%d OFFSET=%ld\n",
       book->current_page,
       page_bookmarks[book->current_page]);

    seek_to_current_page(book);

    char raw_buffer[PAGE_SIZE * 2];
    read_raw_block(raw_buffer);

    size_t bytes_used = 0;

    size_t used = build_page(raw_buffer, &bytes_used);
    size_t cutoff = used;
    ensure_page_capacity(book->current_page + 1);
    page_bookmarks[book->current_page + 1] =
    page_bookmarks[book->current_page] + bytes_used;

    return current_page_buffer;
}

static int can_go_next_page(EReaderBook *book)
{
    if (!book_file)
        return 0;

    long current_pos = page_bookmarks[book->current_page];

    // get file size
    fseek(book_file, 0, SEEK_END);
    long file_size = ftell(book_file);

    // restore position (IMPORTANT)
    fseek(book_file, current_pos, SEEK_SET);

    // allow next page only if we are not at end
    return current_pos < file_size;
}

static int can_go_prev_page(EReaderBook *book)
{
    return book->current_page > 0;
}

int ereader_next_page(EReaderBook *book)
{
    if (!can_go_next_page(book))
        return book->current_page + 1;

    book->current_page++;
    return book->current_page + 1;
}

int ereader_prev_page(EReaderBook *book)
{
    if (!can_go_prev_page(book))
        return book->current_page + 1;

    book->current_page--;
    return book->current_page + 1;
}

// Fetches the active page number index for display labels
int ereader_get_current_page_number(EReaderBook *book) {
    return book->current_page + 1;
}
int ereader_get_progress_percent(EReaderBook *book)
{
    if (!book_file)
        return 0;

    long current_pos = page_bookmarks[book->current_page];

    fseek(book_file, 0, SEEK_END);
    long file_size = ftell(book_file);

    if (file_size <= 0)
        return 0;

    return (int)((current_pos * 100) / file_size);
}

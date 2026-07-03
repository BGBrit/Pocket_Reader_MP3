#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ereader_engine.h"

// Define the global variables
char current_book_path[512] = ""; // Initialize to an empty string
int current_page = 0;

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

static void seek_to_current_page(void)
{
    if (!book_file) return;

    if (page_bookmarks[current_page] == 0 && current_page != 0)
        return; // prevents fake seek

    fseek(book_file, page_bookmarks[current_page], SEEK_SET);
}

void ereader_save_bookmark(const char *file_path) {
    FILE *bookmark_file = open_bookmark_file_rw();

    char saved_file_path[256];
    int saved_page;
    long pos = 0;
    int found = 0;

    while (fscanf(bookmark_file, "%s %d", saved_file_path, &saved_page) == 2) {
        if (strcmp(saved_file_path, file_path) == 0) {
            found = 1;
            break;
        }
        pos = ftell(bookmark_file);
    }

    if (found) {
        fseek(bookmark_file, pos, SEEK_SET);
        fprintf(bookmark_file, "%s %d\n", file_path, current_page);
    } else {
        fseek(bookmark_file, 0, SEEK_END);
        fprintf(bookmark_file, "%s %d\n", file_path, current_page);
    }

    fclose(bookmark_file);
}

void ereader_load_bookmark(const char *file_path) {
    FILE *bookmark_file = open_bookmark_file_r();
    if (!bookmark_file) {
        printf("No bookmarks found.\n");
        return;
    }

    char saved_file_path[256];
    int saved_page;

    while (fscanf(bookmark_file, "%s %d", saved_file_path, &saved_page) == 2) {
        if (strcmp(saved_file_path, file_path) == 0) {
            current_page = saved_page;
            printf("Bookmark loaded for book: %s at page %d\n",
                   file_path, current_page);
            break;
        }
    }

    fclose(bookmark_file);
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

static void seek_to_page_number(int target_page)
{
    current_page = 0;

    for (int i = 0; i < target_page; i++) {
        ereader_get_page_text(); // advances bookmarks internally
        current_page++;
    }
}

int ereader_open_book(const char *file_path)
{
    page_bookmarks_capacity = 100;
    page_bookmarks = malloc(sizeof(long) * page_bookmarks_capacity);
    page_bookmarks[0] = 0;
    book_file = open_book_file(file_path);

    if (!book_file) {
        printf("Error: Could not open book file.\n");
        return 0;
    }

    set_current_book_path(file_path);

    ereader_load_bookmark(file_path);

    if (current_page > 0) {
        seek_to_page_number(current_page);
    }

    // ONLY runtime reset (single source of truth)
    memset(current_page_buffer, 0, sizeof(current_page_buffer));

    printf("Book opened: %s at page %d\n", file_path, current_page);

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

char *ereader_get_page_text(void) {

    if (!book_file)
        return "Error: No book file loaded.";

    printf("PAGE=%d OFFSET=%ld\n",
       current_page,
       page_bookmarks[current_page]);

    seek_to_current_page();

    char raw_buffer[PAGE_SIZE * 2];
    read_raw_block(raw_buffer);

    size_t bytes_used = 0;

    size_t used = build_page(raw_buffer, &bytes_used);
    size_t cutoff = used;
    ensure_page_capacity(current_page + 1);
    page_bookmarks[current_page + 1] =
    page_bookmarks[current_page] + bytes_used;

    return current_page_buffer;
}


// Flips forward one page
int ereader_next_page(void)
{
    if (!book_file) return current_page + 1;

    if (page_bookmarks[current_page + 1] == 0)
        return current_page + 1; // cannot move yet

    current_page++;
    return current_page + 1;
}

// Flips backward one page
int ereader_prev_page(void) {
    if (current_page > 0) {
        current_page--;
    }
    return current_page + 1;
}

// Fetches the active page number index for display labels
int ereader_get_current_page_number(void) {
    return current_page + 1;
}
int ereader_get_progress_percent(void)
{
    if (!book_file)
        return 0;

    long current_pos = page_bookmarks[current_page];

    fseek(book_file, 0, SEEK_END);
    long file_size = ftell(book_file);

    if (file_size <= 0)
        return 0;

    return (int)((current_pos * 100) / file_size);
}

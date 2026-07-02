#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ereader_engine.h"

// Define the global variables
char current_book_path[512] = ""; // Initialize to an empty string
int current_page = 0;

#define PAGE_SIZE 450 // Max characters to display on a 240x320 portrait screen at once
#define MAX_PAGES 1000

static FILE * book_file = NULL;
static long page_bookmarks[MAX_PAGES]; // Holds the exact file character position for every page start
static int max_pages_visited = 0;
static char current_page_buffer[PAGE_SIZE + 1];

static size_t read_raw_block(char *raw_buffer) {
    size_t bytes_read = fread(raw_buffer, 1, (PAGE_SIZE * 2) - 1, book_file);
    raw_buffer[bytes_read] = '\0';
    return bytes_read;
}

static int sanitize_character(const char *src, uint32_t i,
                              char *dest, uint32_t *widx)
{
    if ((unsigned char)src[i] == 0xE2 &&
        (unsigned char)src[i + 1] == 0x80)
    {
        unsigned char t = (unsigned char)src[i + 2];

        if (t == 0x98 || t == 0x99) {
            dest[(*widx)++] = '\'';
            return 3;
        }
        if (t == 0x9C || t == 0x9D) {
            dest[(*widx)++] = '"';
            return 3;
        }
        if (t == 0x94) {
            dest[(*widx)++] = '-';
            return 3;
        }
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

static size_t build_page(char *raw_buffer) {

    uint32_t write_idx = 0;
    uint32_t read_idx = 0;
    uint32_t line_char_count = 0;

    while (raw_buffer[read_idx] != '\0' && write_idx < PAGE_SIZE) {

        int consumed = sanitize_character(
            raw_buffer, read_idx, current_page_buffer, &write_idx);

        if (consumed > 0) {
            read_idx += consumed;
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
            read_idx++;
            continue;
        }

        current_page_buffer[write_idx++] = raw_buffer[read_idx++];
        line_char_count++;
    }

    current_page_buffer[write_idx] = '\0';
    return write_idx;
}

static size_t trim_page(size_t used) {

    size_t cutoff = used;

    if (used >= PAGE_SIZE - 5) {
        for (int i = used - 1; i > (int)used - 40; i--) {
            if (current_page_buffer[i] == ' ' ||
                current_page_buffer[i] == '\n') {

                cutoff = i + 1;
                current_page_buffer[cutoff] = '\0';
                break;
            }
        }
    }

    return cutoff;
}

static void update_bookmark_tracking(char *raw_buffer, size_t cutoff) {

    int actual_file_bytes_used = 0;
    int parsed_bytes_counted = 0;
    int line_char_count = 0;

    while (raw_buffer[actual_file_bytes_used] != '\0'
           && parsed_bytes_counted < (int)cutoff) {

        char dummy_dest;
        uint32_t dummy_write_idx = 0;

        int consumed = sanitize_character(
            raw_buffer,
            actual_file_bytes_used,
            &dummy_dest,
            &dummy_write_idx);

        if (consumed > 0) {
            actual_file_bytes_used += consumed;
            parsed_bytes_counted++;
            line_char_count++;
            continue;
        }

        if (raw_buffer[actual_file_bytes_used] == '\r') {
            actual_file_bytes_used++;
            continue;
        }

        if (raw_buffer[actual_file_bytes_used] == '\n') {
            int next_idx = actual_file_bytes_used + 1;
            int consecutive_newlines = 1;

            while (raw_buffer[next_idx] == '\n' ||
                   raw_buffer[next_idx] == '\r') {
                if (raw_buffer[next_idx] == '\n')
                    consecutive_newlines++;
                next_idx++;
            }

            if (consecutive_newlines >= 2) {
                parsed_bytes_counted += 2;
                actual_file_bytes_used = next_idx - 1;
                line_char_count = 0;
            } else {
                if (line_char_count < 38) {
                    parsed_bytes_counted++;
                    line_char_count = 0;
                } else {
                    parsed_bytes_counted++;
                }
            }
        } else {
            parsed_bytes_counted++;
            line_char_count++;
        }

        actual_file_bytes_used++;
    }

    if (current_page + 1 > max_pages_visited &&
        current_page + 1 < MAX_PAGES) {

        page_bookmarks[current_page + 1] =
            page_bookmarks[current_page] + actual_file_bytes_used;

        max_pages_visited++;
    }
}

static FILE *open_bookmark_file_rw(void) {
    FILE *f = fopen("bookmark.txt", "r+");
    if (!f) f = fopen("bookmark.txt", "w");
    return f;
}

static FILE *open_bookmark_file_r(void) {
    return fopen("bookmark.txt", "r");
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
    max_pages_visited = current_page;
    memset(current_page_buffer, 0, sizeof(current_page_buffer));
}

static void restore_book_position(void)
{
    if (current_page > 0 && current_page < MAX_PAGES) {
        fseek(book_file, page_bookmarks[current_page], SEEK_SET);
    } else {
        current_page = 0;
    }
}

static void initialize_reader_state(void)
{
    max_pages_visited = current_page;
    memset(current_page_buffer, 0, sizeof(current_page_buffer));
}
int ereader_open_book(const char *file_path)
{
    book_file = open_book_file(file_path);

    if (!book_file) {
        printf("Error: Could not open book file.\n");
        return 0;
    }

    set_current_book_path(file_path);

    ereader_load_bookmark(file_path);

    restore_book_position();
    initialize_reader_state();

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


static size_t read_raw_block_at_current_page(char *raw_buffer)
{
    if (!book_file)
        return 0;

    fseek(book_file, page_bookmarks[current_page], SEEK_SET);

    size_t bytes_read =
        fread(raw_buffer, 1, (PAGE_SIZE * 2) - 1, book_file);

    raw_buffer[bytes_read] = '\0';

    return bytes_read;
}

char *ereader_get_page_text(void) {

    if (!book_file)
        return "Error: No book file loaded.";

    fseek(book_file, page_bookmarks[current_page], SEEK_SET);

    char raw_buffer[PAGE_SIZE * 2];
    read_raw_block_at_current_page(raw_buffer);

    size_t used = build_page(raw_buffer);
    size_t cutoff = trim_page(used);

    update_bookmark_tracking(raw_buffer, cutoff);

    return current_page_buffer;
}


// Flips forward one page
int ereader_next_page(void) {
    if (!book_file) return current_page + 1;

    // Check if there is still unread text left in the file archive
    fseek(book_file, 0, SEEK_END);
    long end_of_file = ftell(book_file);

    if (page_bookmarks[current_page + 1] < end_of_file && page_bookmarks[current_page + 1] != 0) {
        current_page++;
    }
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

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

void ereader_save_bookmark(const char *file_path) {
    FILE *bookmark_file = fopen("bookmark.txt", "r+");
    if (!bookmark_file) {
        bookmark_file = fopen("bookmark.txt", "w");
    }

    char saved_file_path[256];
    int saved_page;
    long pos = 0;
    int found = 0;

    // Check if the bookmark for this book already exists
    while (fscanf(bookmark_file, "%s %d", saved_file_path, &saved_page) == 2) {
        if (strcmp(saved_file_path, file_path) == 0) {
            found = 1;
            break;
        }
        pos = ftell(bookmark_file);
    }

    if (found) {
        // Update the existing bookmark
        fseek(bookmark_file, pos, SEEK_SET);
        fprintf(bookmark_file, "%s %d\n", file_path, current_page);
    } else {
        // Add a new bookmark
        fseek(bookmark_file, 0, SEEK_END);
        fprintf(bookmark_file, "%s %d\n", file_path, current_page);
    }

    fclose(bookmark_file);
}

void ereader_load_bookmark(const char *file_path) {
    FILE *bookmark_file = fopen("bookmark.txt", "r");
    if (!bookmark_file) {
        printf("No bookmarks found.\n");
        return;
    }

    char saved_file_path[256];
    int saved_page;

    // Search for the bookmark for this book
    while (fscanf(bookmark_file, "%s %d", saved_file_path, &saved_page) == 2) {
        if (strcmp(saved_file_path, file_path) == 0) {
            current_page = saved_page;
            printf("Bookmark loaded for book: %s at page %d\n", file_path, current_page);
            break;
        }
    }

    fclose(bookmark_file);
}
int ereader_open_book(const char *file_path) {
    book_file = fopen(file_path, "r");
    if (!book_file) {
        printf("Error: Could not open book file.\n");
        return 0; // Return 0 to indicate failure
    }

    // Set the current book path
    strncpy(current_book_path, file_path, sizeof(current_book_path) - 1);
    current_book_path[sizeof(current_book_path) - 1] = '\0'; // Ensure null termination

    // Load bookmark for this book
    ereader_load_bookmark(file_path);

    // Seek to the correct position in the file based on the bookmark
    if (current_page > 0 && current_page < MAX_PAGES) {
        fseek(book_file, page_bookmarks[current_page], SEEK_SET);
    } else {
        current_page = 0; // Default to the first page if no valid bookmark exists
    }

    max_pages_visited = current_page;
    memset(current_page_buffer, 0, sizeof(current_page_buffer));

    printf("Book opened: %s at page %d\n", file_path, current_page);
    return 1; // Return 1 to indicate success
}
// Closes the file safely when backing out to the shelf menu
void ereader_close_book(void) {
    if (book_file) {
        fclose(book_file);
        book_file = NULL;
    }
}
// Safely detects UTF-8 smart characters and swaps them into standard ascii tokens
static int sanitize_character(const char * src, uint32_t read_idx, char * dest, uint32_t * write_idx) {
    if ((unsigned char)src[read_idx] == 0xE2 && (unsigned char)src[read_idx + 1] == 0x80) {
        unsigned char third_byte = (unsigned char)src[read_idx + 2];

        if (third_byte == 0x98 || third_byte == 0x99) { // Smart single quote / apostrophe
            dest[(*write_idx)++] = '\'';
            return 3; // Consumed 3 raw bytes from the file stream
        }
        else if (third_byte == 0x9C || third_byte == 0x9D) { // Curly double quotes
            dest[(*write_idx)++] = '"';
            return 3;
        }
        else if (third_byte == 0x94) { // Em-dash (—) swap to standard dash
            dest[(*write_idx)++] = '-';
            return 3;
        }
    }
    return 0; // Not a special character, zero bytes handled
}

char * ereader_get_page_text(void) {
    if (!book_file) return "Error: No book file loaded.";

    fseek(book_file, page_bookmarks[current_page], SEEK_SET);

    char raw_buffer[PAGE_SIZE * 2];
    size_t bytes_read = fread(raw_buffer, 1, sizeof(raw_buffer) - 1, book_file);
    raw_buffer[bytes_read] = '\0';

    uint32_t write_idx = 0;
    uint32_t read_idx = 0;
    uint32_t line_char_count = 0;

    while (raw_buffer[read_idx] != '\0' && write_idx < PAGE_SIZE) {

        // 1. CALL THE REFACTORED SANITIZER METHOD
        int bytes_sanitized = sanitize_character(raw_buffer, read_idx, current_page_buffer, &write_idx);
        if (bytes_sanitized > 0) {
            read_idx += bytes_sanitized;
            line_char_count++;
            continue;
        }

        if (raw_buffer[read_idx] == '\r') {
            read_idx++;
            continue;
        }

        if (raw_buffer[read_idx] == '\n') {
            int next_idx = read_idx + 1;
            int consecutive_newlines = 1;
            while (raw_buffer[next_idx] == '\n' || raw_buffer[next_idx] == '\r') {
                if (raw_buffer[next_idx] == '\n') consecutive_newlines++;
                next_idx++;
            }

            if (consecutive_newlines >= 2) {
                current_page_buffer[write_idx++] = '\n';
                if (write_idx < PAGE_SIZE) current_page_buffer[write_idx++] = '\n';
                read_idx = next_idx - 1;
                line_char_count = 0;
            }
            else {
                if (line_char_count < 38) {
                    current_page_buffer[write_idx++] = '\n';
                    line_char_count = 0;
                }
                else {
                    if (write_idx > 0 && current_page_buffer[write_idx - 1] != ' ') {
                        current_page_buffer[write_idx++] = ' ';
                    }
                }
            }
        }
        else {
            current_page_buffer[write_idx++] = raw_buffer[read_idx];
            line_char_count++;
        }
        read_idx++;
    }
    current_page_buffer[write_idx] = '\0';

    uint32_t characters_that_fit = write_idx;
    if (write_idx >= PAGE_SIZE - 5) {
        for (int i = write_idx - 1; i > (int)write_idx - 40; i--) {
            if (current_page_buffer[i] == ' ' || current_page_buffer[i] == '\n') {
                characters_that_fit = i + 1;
                current_page_buffer[characters_that_fit] = '\0';
                break;
            }
        }
    }

    // 4. MAP THE DIGITAL PAGE BOOKMARK OFFSET VALUE ACCURATELY TO THE REAL FILE SEEK POSITION
    int actual_file_bytes_used = 0;
    int parsed_bytes_counted = 0;
    line_char_count = 0;

    while (raw_buffer[actual_file_bytes_used] != '\0' && parsed_bytes_counted < (int)characters_that_fit) {

        // Pass a dummy destination character just to track the bookmark byte offsets safely
        char dummy_dest;
        uint32_t dummy_write_idx = 0;
        int bytes_sanitized = sanitize_character(raw_buffer, actual_file_bytes_used, &dummy_dest, &dummy_write_idx);
        if (bytes_sanitized > 0) {
            actual_file_bytes_used += bytes_sanitized;
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
            while (raw_buffer[next_idx] == '\n' || raw_buffer[next_idx] == '\r') {
                if (raw_buffer[next_idx] == '\n') consecutive_newlines++;
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

    if (current_page + 1 > max_pages_visited && current_page + 1 < MAX_PAGES) {
        page_bookmarks[current_page + 1] = page_bookmarks[current_page] + actual_file_bytes_used;
        max_pages_visited++;
    }

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

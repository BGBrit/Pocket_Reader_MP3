#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 450 // Max characters to display on a 240x320 portrait screen at once
#define MAX_PAGES 1000

static FILE * book_file = NULL;
static long page_bookmarks[MAX_PAGES]; // Holds the exact file character position for every page start
static int current_page = 0;
static int max_pages_visited = 0;
static char current_page_buffer[PAGE_SIZE + 1];

// Opens the chosen book file and resets bookmarks to the beginning
int ereader_open_book(const char * file_path) {
    if (book_file) {
        fclose(book_file);
    }

    book_file = fopen(file_path, "r");
    if (!book_file) {
        return 0; // Error opening file
    }

    // Reset page logs
    memset(page_bookmarks, 0, sizeof(page_bookmarks));
    page_bookmarks[0] = 0; // Page 1 starts at character index 0
    current_page = 0;
    max_pages_visited = 0;

    return 1; // Success
}

// Closes the file safely when backing out to the shelf menu
void ereader_close_book(void) {
    if (book_file) {
        fclose(book_file);
        book_file = NULL;
    }
}

// Reads a clean chunk of text based on our current page position
// Reads a clean chunk of text and cuts off perfectly at a whole word boundary
// Reads a clean chunk of text, strips massive spacing, and cuts off at a whole word
char * ereader_get_page_text(void) {
    if (!book_file) return "Error: No book file loaded.";

    fseek(book_file, page_bookmarks[current_page], SEEK_SET);

    // 1. Read a larger block of text data from the drive to account for paragraph compression changes
    char raw_buffer[PAGE_SIZE * 2];
    size_t bytes_read = fread(raw_buffer, 1, sizeof(raw_buffer) - 1, book_file);
    raw_buffer[bytes_read] = '\0';

    // 2. RUN INTEL INTELLIGENT PARAGRAPH COMPRESSION & LINE-UNWRAPPING
    uint32_t write_idx = 0;
    uint32_t read_idx = 0;
    uint32_t line_char_count = 0; // Tracks characters since the last intentional break

    while (raw_buffer[read_idx] != '\0' && write_idx < PAGE_SIZE) {
        // Strip Windows carriage return carriage flags completely to evaluate a pure \n stream
        if (raw_buffer[read_idx] == '\r') {
            read_idx++;
            continue;
        }

        // Check if we encountered an active line break marker
        if (raw_buffer[read_idx] == '\n') {
            // Count how many consecutive newlines follow (checking past hidden \r values)
            int next_idx = read_idx + 1;
            int consecutive_newlines = 1;
            while (raw_buffer[next_idx] == '\n' || raw_buffer[next_idx] == '\r') {
                if (raw_buffer[next_idx] == '\n') consecutive_newlines++;
                next_idx++;
            }

            if (consecutive_newlines >= 2) {
                // TRUE PARAGRAPH BOUNDARY DETECTED: Print exactly one empty spacing line
                current_page_buffer[write_idx++] = '\n';
                if (write_idx < PAGE_SIZE) current_page_buffer[write_idx++] = '\n';
                read_idx = next_idx - 1; // Advance the pointer past the spacer blocks
                line_char_count = 0;
            }
            else {
                // SINGLE LINE BREAK ENCOUNTERED (Evaluate context constraints)
                // If the previous line was short, it's a Table of Contents list or Title header—PRESERVE IT!
                if (line_char_count < 38) {
                    current_page_buffer[write_idx++] = '\n';
                    line_char_count = 0;
                }
                else {
                    // It is a hard-wrapped middle sentence fragment line—STRIP AND REPLACE WITH SPACE!
                    // Ensure we don't accidentally print double spaces if one is already present
                    if (write_idx > 0 && current_page_buffer[write_idx - 1] != ' ') {
                        current_page_buffer[write_idx++] = ' ';
                    }
                }
            }
        }
        else {
            // Normal character processing
            current_page_buffer[write_idx++] = raw_buffer[read_idx];
            line_char_count++;
        }
        read_idx++;
    }
    current_page_buffer[write_idx] = '\0';

    // 3. SCAN BACKWARD TO FIND A PERFECT WORD BOUNDARY BREAK
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
                    parsed_bytes_counted++; // Preserved newline count increment
                    line_char_count = 0;
                } else {
                    parsed_bytes_counted++; // Space placeholder evaluation swap
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

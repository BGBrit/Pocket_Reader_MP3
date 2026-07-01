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
char * ereader_get_page_text(void) {
    if (!book_file) return "Error: No book file loaded.";

    // Jump straight to the bookmark position saved for this specific page index
    fseek(book_file, page_bookmarks[current_page], SEEK_SET);

    // Read a raw block of letters from your computer's hard drive text stream
    size_t bytes_read = fread(current_page_buffer, 1, PAGE_SIZE, book_file);
    current_page_buffer[bytes_read] = '\0'; // Clean end to the text string

    // SMART FIX: Scan backward from the end of our buffer to find a clean word break
    uint32_t characters_that_fit = bytes_read;
    if (bytes_read == PAGE_SIZE) {
        // Look for the last space, newline, or punctuation mark within the last 40 characters
        for (int i = PAGE_SIZE - 1; i > PAGE_SIZE - 40; i--) {
            if (current_page_buffer[i] == ' ' || current_page_buffer[i] == '\n' || current_page_buffer[i] == '-') {
                characters_that_fit = i + 1; // Cut off exactly after this word break
                current_page_buffer[characters_that_fit] = '\0';
                break;
            }
        }
    }

    // If we are opening a brand new page forward, map its start marker for the back button
    if (current_page + 1 > max_pages_visited && current_page + 1 < MAX_PAGES) {
        page_bookmarks[current_page + 1] = page_bookmarks[current_page] + characters_that_fit;
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

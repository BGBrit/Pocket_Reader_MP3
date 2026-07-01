#ifndef EREADER_ENGINE_H
#define EREADER_ENGINE_H

// Function declarations
int   ereader_open_book(const char *file_path);
void  ereader_close_book(void);
char* ereader_get_page_text(void);
int   ereader_next_page(void);
int   ereader_prev_page(void);
int   ereader_get_current_page_number(void);
void  ereader_save_bookmark(const char *file_path);

// Global variables (declared as extern)
extern char current_book_path[512];
extern int current_page;

#endif // EREADER_ENGINE_H

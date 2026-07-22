#ifndef TEXT_ENTRY_H
#define TEXT_ENTRY_H


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Called when the user presses S to save.
 *
 * The returned string is the final entered text.
 */
typedef void (*TextEntryCallback)(
    const char *text
);



/*
 * Open text entry popup.
 *
 * title:
 *      Popup title ("Rename Playlist", "Create Playlist")
 *
 * initial_text:
 *      Existing text for editing, or "" for new entry
 *
 * callback:
 *      Function called when user saves
 */
void text_entry_open(
    const char *title,
    const char *initial_text,
    TextEntryCallback callback
);



/*
 * Close popup without saving
 */
void text_entry_close(void);



/*
 * Returns 1 while text entry popup is active
 */
int text_entry_is_open(void);



/*
 * Send keyboard input to text entry
 */
void text_entry_handle_key(
    uint32_t key
);



#ifdef __cplusplus
}
#endif


#endif

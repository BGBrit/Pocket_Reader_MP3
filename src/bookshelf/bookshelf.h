#ifndef BOOKSHELF_H
#define BOOKSHELF_H

#include "../ereader_engine.h"
#include <stdint.h>

void bookshelf_open(void);

void bookshelf_handle_key(uint32_t key);

EReaderBook *bookshelf_get_selected(void);

#endif

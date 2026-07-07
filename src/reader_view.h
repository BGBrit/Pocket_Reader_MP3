#ifndef READER_VIEW_H
#define READER_VIEW_H

#include <stdint.h>
#include "ereader_engine.h"

void reader_open(EReaderBook *book);

void reader_handle_key(uint32_t key);

void reader_close(void);

#endif

# Pocket Reader Architecture

This document describes the high-level architecture of the Pocket Reader project and the responsibilities of each module.

The primary design goal is **separation of responsibility**:

- UI manages navigation
- Bookshelf manages the library
- Reader manages presentation
- Engine manages page generation
- Storage manages persistence

---

# Overall Architecture

```
main()
│
▼
my_custom_ui.c
│
├──────────────► bookshelf.c
│                   │
│                   ▼
│              reader_view.c
│                   │
│                   ▼
│             ereader_engine.c
│                   │
│                   ▼
│             Text (.txt) files
│
└──────────────► Future Apps
                    (MP3, Notes, etc.)
```

Each layer should only know about the layer directly beneath it.

---

# Module Responsibilities

## 1. my_custom_ui.c

### Responsibility

Application launcher.

This file should **never** know anything about books, pages, bookmarks, or rendering text.

Its only job is switching between applications.

### Current Flow

```
HOME
    ↓
SPACE
    ↓
Bookshelf
```

### Public Functions

```c
init_pocket_reader_ui()

draw_home_page()

handle_home_key()
```

---

## 2. bookshelf.c

### Responsibility

Owns the library.

This module is responsible for:

- Discovering books
- Loading/saving bookmarks
- Displaying the bookshelf
- Selecting books

It owns all library state.

### Private State

```c
books[]

book_count

selected_book
```

No other module should modify these.

### Public Functions

```c
bookshelf_load_books()

bookshelf_save_books()

bookshelf_open()

bookshelf_handle_key()

bookshelf_get_selected()
```

---

### bookshelf_load_books()

Reads:

```
books.db
```

Scans:

```
/books
```

Merges both sources into:

```
books[]
```

Behavior:

- Restores existing bookmarks
- Adds newly copied books
- Removes deleted books automatically

---

### bookshelf_save_books()

Writes the current in-memory library to:

```
books.db
```

No UI logic should exist here.

---

### bookshelf_open()

Draws:

```
BOOKSHELF

> Sherlock

  Pilgrim

  John Wesley
```

This function should **never** reload books.

It only displays the current library.

---

### bookshelf_handle_key()

Handles:

```
LEFT
RIGHT
SPACE
```

Example:

```
RIGHT
    ↓
selected_book++
    ↓
bookshelf_open()
```

SPACE:

```
SPACE
    ↓
reader_open(selected_book)
```

---

# 3. reader_view.c

### Responsibility

Presentation layer.

This module owns the reader UI.

It knows nothing about:

- File parsing
- UTF-8
- Storage
- Book discovery

### Private State

```c
active_book

reader_text

reader_footer
```

### Public Functions

```c
reader_open()

reader_handle_key()

reader_close()
```

---

### reader_open()

```
Book
    ↓
ereader_open_book()
    ↓
Build UI
    ↓
reader_refresh()
```

---

### reader_refresh()

Requests a page from the engine:

```
ereader_get_page()
```

Displays:

- Page text
- Current page
- Reading progress

---

### reader_handle_key()

RIGHT

```
ereader_next_page()

↓

reader_refresh()
```

LEFT

```
ereader_prev_page()

↓

reader_refresh()
```

SPACE

```
ereader_save_bookmark()

↓

bookshelf_save_books()
```

---

# 4. ereader_engine.c

### Responsibility

Core reading engine.

The engine knows absolutely nothing about LVGL.

It only knows:

```
Book

↓

Pages
```

---

### Runtime State

```c
current_page

current_offset

book_file

page_offsets[]

page_count
```

These are intentionally hidden.

---

### Public API

```c
ereader_open_book()

ereader_close_book()

ereader_get_page()

ereader_next_page()

ereader_prev_page()

ereader_save_bookmark()

ereader_get_progress_percent()

ereader_get_current_page_number()
```

---

### Opening a Book

```
bookmark_page

bookmark_offset

↓

Restore runtime

↓

Rebuild page offset cache

↓

Ready to read
```

The page offset cache is rebuilt once when opening the book so previous-page navigation works immediately.

---

### Reading

```
Offset

↓

Read bytes

↓

Sanitize UTF-8

↓

Build page

↓

Return PageResult
```

---

### Navigation

Forward

```
current_offset += bytes_used
```

Backward

```
current_offset = page_offsets[current_page]
```

---

### Bookmark

```
current_page

↓

bookmark_page
```

```
current_offset

↓

bookmark_offset
```

The engine updates the active book.

The bookshelf persists the updated library.

---

# Storage

The application uses two storage systems.

---

## Book Files

```
/books/

Sherlock.txt

Pilgrims_Progress.txt

JohnWesley.txt
```

These are read-only.

---

## Library Database

```
books.db
```

Stores:

```
Book Path

Bookmark Page

Bookmark Offset
```

Example:

```
/Users/.../Sherlock.txt|25|18432
/Users/.../Pilgrims_Progress.txt|4|3125
```

---

# Runtime Flow

```
Application Start
        │
        ▼
      HOME
        │
     SPACE
        │
        ▼
bookshelf_load_books()
        │
        ▼
     books[]
        │
        ▼
bookshelf_open()
        │
     SPACE
        │
        ▼
 reader_open(book)
        │
        ▼
ereader_open_book()
        │
        ▼
reader_refresh()
        │
        ▼
     Display Page
```

---

# Bookmark Flow

```
Reading
    │
 SPACE
    │
    ▼
ereader_save_bookmark()
    │
    ▼
Update active_book
    │
    ▼
bookshelf_save_books()
    │
    ▼
books.db
```

---

# Design Principles

The project follows a few important rules.

### 1. Single Responsibility

Each module should have one purpose.

- UI → navigation
- Bookshelf → library
- Reader → presentation
- Engine → reading

---

### 2. Engine Never Touches UI

The engine should never include LVGL.

It returns data only.

---

### 3. Reader Never Parses Files

The reader displays pages.

The engine generates pages.

---

### 4. Bookshelf Owns the Library

Only the bookshelf should manage:

- books[]
- selected_book
- books.db

---

### 5. Runtime vs Persistent State

Runtime:

```
current_page

current_offset

page_offsets[]
```

Persistent:

```
bookmark_page

bookmark_offset

books.db
```

The runtime cache is rebuilt when opening a book.

---

# Future Expansion

The architecture is intentionally modular.

```
HOME

├── E-Reader
│      │
│      ▼
│  Bookshelf
│      │
│      ▼
│   Reader
│
├── MP3 Player
│
├── Notes
│
└── Settings
```

Each application should be self-contained and only expose a small public API to the launcher.

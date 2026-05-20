# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(sysctl -n hw.ncpu)
```

The system has Qt5 (via Anaconda). The CMakeLists supports both Qt5 and Qt6 — new source files must be added to `EXTRA_SOURCES` (used by both branches).

## Architecture

A Qt Widgets desktop app for college students to track courses, extracurricular activities, and awards. Uses SQLite via `QSqlTableModel`.

### Directory layout

| Directory | Purpose |
|---|---|
| `src/models/` | Data layer — `DatabaseManager` (DB singleton), `User` (current-session singleton) |
| `src/views/` | UI classes — `MainWindow`, `LoginDialog`, `RegisterDialog`, `AddCourseDialog` |
| `src/controllers/` | Reserved, currently empty |
| `ui/` | Qt Designer file — `mainwindow.ui` |
| `src/main.cpp` | Entry point: init DB → show LoginDialog → show MainWindow |

### Data flow

1. `main.cpp` initializes `DatabaseManager` singleton, opens SQLite
2. User logs in via `LoginDialog` → `DatabaseManager::loginUser()` returns user id → `User::getInstance().login(id)` loads full profile from DB
3. `MainWindow` uses `QSqlTableModel` with `setFilter("user_id = ...")` for per-user data isolation
4. All data tables (`courses`, `experiences`, `awards`) have a `user_id` FK column

### Key classes

- **`DatabaseManager`** (`src/models/DatabaseMannager.h`) — Meyers singleton. Creates/migrates tables, all CRUD. `migrateTables()` runs `ALTER TABLE ADD COLUMN` for backward-compatible schema changes.
- **`User`** (`src/models/User.h`) — Meyers singleton holding the current logged-in user's `id`, `username`, `grade`, `gender`, `major`. Call `User::getInstance().getId()` anywhere you need the current user's id.
- **`MainWindow`** (`src/views/mainwindow.h`) — `QStackedWidget` with sidebar navigation. Manages three `QSqlTableModel*` (courseModel, expModel, awardModel). Auto-connected slots follow Qt naming (`on_addCourseBtn_clicked`).

### Database tables

| Table | Key columns (beyond id + user_id) |
|---|---|
| `users` | username, password, grade, gender, major |
| `courses` | name, credit, score, semester |
| `experiences` | title, type, date, content |
| `awards` | name, level, date |

### Patterns

- **File naming**: macOS APFS is case-insensitive but case-preserving. CMakeLists.txt paths must match the exact case used when the file was created.
- **Include paths**: `src/models/` and `src/views/` are on the include path, so headers are included by filename alone (`#include "User.h"`, not `#include "../models/User.h"`).
- **Singleton**: Both singletons use the Meyers pattern (static local in `getInstance()`, deleted copy/assignment, private constructor).
- **UI signal/slot**: Buttons use Qt's automatic `connectSlotsByName` — declare slots as `on_widgetName_signal()` and they connect without explicit `connect()` calls.

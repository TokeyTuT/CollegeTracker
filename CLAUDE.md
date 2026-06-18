# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Environment

- **Qt**: 6.11.0 (official installer) at `/Users/andydelaptop/Qt/6.11.0/macos`
- **CMake**: 4.3.3 (Homebrew) at `/opt/homebrew/bin/cmake`
- **Compiler**: Apple Clang (Xcode)
- **Platform**: macOS 26 (Darwin 25.5.0) ARM64
- Qt bin directory is in PATH via `~/.zshrc`
- **VSCode**: `.vscode/` contains tasks/launch/IntelliSense config; install recommended extensions

## Build

### Command line

```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel $(sysctl -n hw.ncpu)
```

### VSCode

- **Build**: `⇧⌘B` (or Terminal → Run Build Task) runs the default "CMake: Build" task
- **Debug**: `F5` — launches the app under LLDB, builds first if needed
- **Compile commands**: `build/compile_commands.json` is symlinked to project root for IntelliSense

CMake configure is wired as task "CMake: Configure" — use `⇧⌘P` → Tasks: Run Task → CMake: Configure after changing CMakeLists.txt.

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

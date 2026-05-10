# FutureCode SIS

A university **student information system**, written in C++17 with
[raylib](https://www.raylib.com/). FutureCode Technology University's
student portal — designed to feel like a real-world academic SIS.

The whole interface is in **Bulgarian** by default, with a one-click
**English** toggle.

## Highlights

- **Login by faculty number**, with password (or fallback username for the
  pre-seeded teacher). The login screen has the same dual-pane layout you'll
  see on the real SIS — branding panel on the left, sign-in form on the
  right, with a "What's new?" modal and Google sign-in button (mocked in the
  demo).
- **Tabbed student dashboard** with six tabs:
  1. **Оценки (Grades)** — GPA tile, ECTS earned tile, subjects passed,
     current semester, plus a chronological grade list with subject color
     stripes and a colored grade pill on each row.
  2. **Учебен план (Curriculum)** — full course catalog grouped by semester
     showing course code, name, credits, weekly hours, type
     (compulsory/elective), lecturer, and your latest grade.
  3. **График (Schedule)** — proper weekly grid (Mon–Fri × 8:00–18:00) with
     class blocks colored by subject, showing code, kind (lecture / lab /
     seminar), time, and room. Today's column is highlighted.
  4. **Изпитна сесия (Exam session)** — upcoming/past/all filter chips, list
     of exams with date pill (with relative countdown), subject, time, room,
     lecturer, and session-type pill (regular / makeup) color-coded by
     urgency (red ≤3 days, amber ≤7).
  5. **Документи (Documents)** — an **online document request form** with
     four document types (academic transcript, enrollment certificate,
     semester certificate, diploma duplicate), purpose textarea, and a
     submission history with status pills (pending / processing / ready /
     collected).
  6. **Профил (Profile)** — personal info, academic info (program, year,
     semester, status pill), language switch (BG/EN), theme switch
     (light/dark) — all persisted per-user.
- **Bulgarian-grade scale** (2–6) with localized labels:
  Отличен / Мн. добър / Добър / Среден / Слаб.
- **Cyrillic-aware font loader** — searches DejaVu Sans, then Segoe UI, then
  Arial; loads the full Bulgarian Cyrillic glyph range plus typographic
  punctuation.
- **Persistent file-based store** with auto-migration: if you upgrade across
  schema versions, the old data file is backed up to `.bak` and a fresh seed
  is written.
- **Notifications** in the top-right corner, theme toggle (light/dark with
  per-user persistence), animated transitions between screens.
- **Teacher (admin) workflow** preserved: dashboard with student search,
  sort, add students, manage courses, statistics screen with bar/line
  charts, drill-down into a single student.

## Build

You will need **raylib 5.0+** (5.5+ recommended) and a C++17 compiler.

### Linux

```sh
sudo apt install fonts-dejavu libraylib-dev   # Debian/Ubuntu
make
./grademaster
```

If your distribution doesn't ship raylib in the package manager, build it
from source: <https://github.com/raysan5/raylib>.

### macOS

```sh
brew install raylib
make
./grademaster
```

### Windows (MSVC)

Open the project in Visual Studio with raylib installed (e.g. via vcpkg).
The Makefile is also compatible with `mingw32-make` if you prefer.

The font search includes `C:/Windows/Fonts/segoeui.ttf` so Cyrillic should
render out of the box on Windows. On macOS/Linux you'll want DejaVu Sans
installed.

## Default credentials

The first run seeds the database with realistic sample data. Try logging in
with any of these:

| Login (faculty number / username) | Password   | Role / Program |
|----------------------------------|------------|----------------|
| `admin`                          | `admin`    | Lecturer (Янко Янакиев) |
| `F23001` (or `denis`)            | `student`  | Денис Паспалов — Информатика |
| `F23002` (or `stoyan`)           | `student`  | Стоян Колев — Информатика |
| `F23003` (or `ivan`)             | `student`  | Иван Димитров — Информатика |
| `F23004` (or `maria`)            | `student`  | Мария Георгиева — Софтуерно инженерство |
| `F23005` (or `petar`)            | `student`  | Петър Стоянов — Софтуерно инженерство |
| `F23006` (or `elena`)            | `student`  | Елена Тодорова — Софтуерно инженерство |
| `F23007` (or `nikolay`)          | `student`  | Николай Христов — Информационни системи |
| `F23008` (or `viktoria`)         | `student`  | Виктория Иванова — Информационни системи |

You can also create new accounts via **Създай профил** on the login screen.

## Sample academic content

The seeded curriculum is a realistic first-year CS bachelor's:

| Sem | Code   | Course (BG)                                | Credits | h/wk |
|-----|--------|--------------------------------------------|--------:|-----:|
| 1   | MAT101 | Висша математика I                         |       7 |    6 |
| 1   | INF101 | Програмиране I                             |       7 |    6 |
| 1   | INF102 | Дискретна математика                       |       5 |    4 |
| 1   | INF103 | Въведение в компютърните системи           |       5 |    4 |
| 1   | LAN101 | Английски език за IT (избираема)           |       3 |    2 |
| 2   | MAT102 | Висша математика II                        |       6 |    5 |
| 2   | INF201 | Програмиране II — ООП                      |       7 |    6 |
| 2   | INF202 | Алгоритми и структури от данни             |       7 |    6 |
| 2   | INF203 | Бази от данни                              |       6 |    5 |
| 2   | INF204 | Уеб технологии (избираема)                 |       4 |    3 |

The schedule and exam-session data are seeded for the second semester (the
"current" semester for sample students), with exam dates spread relative to
the day you first launch the app.

## Keyboard shortcuts (student dashboard)

- **1** — Оценки
- **2** — Учебен план
- **3** — График
- **4** — Изпитна сесия
- **5** — Документи
- **6** — Профил
- **Esc** — close any open modal

## File layout

```
src/
  main.cpp              window + main loop
  app.{h,cpp}           top-level controller, transitions, header overlay
  theme.{h,cpp}         palette + font loader (Cyrillic-aware)
  i18n.h                BG/EN translation table (header-only)
  models.{h,cpp}        plain-data structs and small helpers
  data_store.{h,cpp}    in-memory data + flat-file persistence
  ui.{h,cpp}            all reusable widgets (buttons, inputs, charts, …)
  screen.h              screen base class
  screen_auth.{h,cpp}   login + signup
  screen_student.{h,cpp}  tabbed student dashboard + teacher's student view
  screen_teacher.{h,cpp}  teacher dashboard + modals
  screen_stats.{h,cpp}    statistics screen
  easing.h              header-only easing helpers (Approach, OutCubic, OutBack)

resources/
  README.txt            instructions for adding fonts
  DejaVuSans.ttf        (optional — drop in here)
  DejaVuSans-Bold.ttf   (optional)

data/
  grademaster.dat       created on first run; flat text format
```

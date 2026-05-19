<div align="center">
FutureCode SIS
A modern student information system written in C++17 + raylib
A complete university student portal — login, grades, schedule, exams, document
requests, statistics, all running natively at 60 FPS with custom-drawn UI.
Show Image
Show Image
Show Image
Show Image
Show Image
</div>

Table of contents

What is this
Highlights
Screenshots
Quick start
Login credentials
Keyboard shortcuts
Features in depth
Project structure
Tech stack
Architecture
Contributing
Troubleshooting
License


What is this
FutureCode SIS is a desktop application that mimics a real university
student portal. Students can browse grades, follow the weekly schedule, track
their exam session, request transcripts and more. Teachers (admins) can manage
students, subjects, grades, and the schedule itself.
The whole interface is in Bulgarian by default with a one-click toggle to
English — every UI string flows through an in-app translation table.
Everything you see is drawn from primitives — there are no PNG or JPG
assets. Icons, charts, the university crest, the avatar circles — all drawn
with raylib's primitives so they tint with the active theme and stay crisp at
any zoom level.
Highlights

Six-tab student dashboard — Grades, Curriculum, Schedule, Exams,
Documents, Profile
Animated GPA tile with sparkline, trend arrow, and number that counts up
Live current-time indicator on the schedule grid
Mini calendar widget that highlights upcoming exam dates
Admin can add schedule entries through a modal: pick subject, day, time,
type, room, lecturer
Smart insights auto-generated when grades trend up or down by ≥0.5
points
Export transcript to a formatted text file with Ctrl+E
Cyrillic-first UI with proper UTF-8 handling for Bulgarian names
Dark / light themes with persistent per-user preference
Floating help overlay (F1) listing every keyboard shortcut
Toast notifications with stacking and auto-dismiss
Cross-platform — single Makefile builds on Linux, macOS, and
Windows (MinGW)

Screenshots
+----------------------------------------------------------------+
|  FC  FutureCode                Здравейте,                       |
|      Student Information       Денис Паспалов                   |
|      System                    Факултетен №  F23001        🌙 🔔 |
+----------------------------------------------------------------+
|  Grades  Curriculum  Schedule  Exams  Documents  Profile        |
|  ────                                                           |
+----------------------------------------------------------------+
|  ┌────────┐ ┌──────────┐ ┌───────────┐ ┌────────────────────┐  |
|  │ GPA    │ │ Credits  │ │ Subjects  │ │ Semester           │  |
|  │ 5.42 ↑ │ │ 45 / 60  │ │ 8 / 10    │ │ 2                  │  |
|  │ ╱╲ ╱╲  │ │ 75% ECTS │ │           │ │ 2025/2026          │  |
|  └────────┘ └──────────┘ └───────────┘ └────────────────────┘  |
|                                                                 |
|  Последни оценки                                                |
|  ┌─────────────────────────────────────────────────────────┐   |
|  │ ▌ MAT102  Висша математика II                       [6] │   |
|  │   проф. д-р ... · 2025-09-12                            │   |
|  └─────────────────────────────────────────────────────────┘   |
|                                                            ?    |
+----------------------------------------------------------------+
(ASCII sketch — run the app to see the real thing rendered with vector icons,
animations, soft shadows, and the university navy/burgundy/gold palette.)
Quick start
1. Install raylib 5.5+
Ubuntu / Debian:
shsudo apt install libraylib-dev g++ make
macOS (Homebrew):
shbrew install raylib
Windows (MSYS2 / MinGW64):
shpacman -S mingw-w64-x86_64-raylib mingw-w64-x86_64-gcc mingw-w64-x86_64-make
Or build raylib from source: https://github.com/raysan5/raylib
2. Build and run
shgit clone <your-repo-url> grademaster
cd grademaster
make
./grademaster
The data/ folder is created automatically on first launch with seeded sample
data — 1 teacher, 8 students, 15 subjects, ~20 schedule entries, 8 exams, and
hundreds of grades.
3. Optional — drop in a Cyrillic font
The app ships without bundled fonts (DejaVuSans is GPL-licensed). On first run
it falls back to Segoe UI / Arial if available, or to raylib's default. For
the best visual result, drop a TTF into resources/:
shcp /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf resources/
cp /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf resources/
See resources/README.txt for details.
Login credentials
All passwords for seeded students are student.
Students
Faculty №UsernameFull nameProgramF23001denisДенис ПаспаловИнформатикаF23002stoyanСтоян КолевИнформатикаF23003ivanИван ДимитровИнформатикаF23004mariaМария ГеоргиеваСофтуерно инженерствоF23005petarПетър СтояновСофтуерно инженерствоF23006elenaЕлена ТодороваСофтуерно инженерствоF23007nikolayНиколай ХристовИнформационни системиF23008viktoriaВиктория ИвановаИнформационни системи
You can log in with either the faculty number or the username — both
work as the login field.
Teacher (admin)
LoginPasswordadminadmin
Logs into the teacher dashboard with student management, subjects, grades,
schedule editing, and the statistics screen.

Recommended demo account: F23001 / student (Денис Паспалов) — has
the most seeded grades and pre-existing document requests, so the GPA
sparkline + trend arrow look great.

Keyboard shortcuts
KeyAction1 – 6Switch to tab 1–6Ctrl+EExport current student transcriptF1Toggle keyboard shortcuts overlayEscClose any modal or overlayEnterSubmit login formMouse wheelScroll any list with overflow
The floating ? button bottom-right opens the same help overlay as F1.
Features in depth
Student dashboard
Grades tab
Four stat tiles across the top:

GPA — animated counter from 0 to current average; sparkline of the last
8 grades along the bottom; up/down arrow showing whether the recent half is
better than the older half
Credits earned — 45 / 60 plus an ECTS percentage
Subjects passed — count of subjects with average ≥ 3.0
Semester + academic year — e.g. 2 · 2025/2026

Below that, a scrollable list of recent grades with subject color stripe,
course code, course name, lecturer, date, and a colored grade pill.
Curriculum tab
All subjects grouped by semester. Each row shows code, name, lecturer,
credits, hours per week, type pill (compulsory/elective), and the latest
grade. Activity-style subjects (clubs, workshops with credits=0) are
hidden from this view — they only appear on the schedule.
Schedule tab

Today's classes panel — pill cards for each class today; the one
happening right now gets a pulsing border and a "Сега"/"Now" badge; past
classes fade to 40% opacity
Weekly grid — Mon-Fri × 8:00–18:00 with alternating row tints, today's
column highlighted, and a live red current-time indicator that moves
through the grid in real time
Activity blocks marked with a star icon ⭑

Exams tab
Two-column layout:

Left — filter pills (Upcoming / Past / All) and a scrollable card list.
Each card has a date pill colored by urgency (red ≤ 3 days, amber ≤ 7,
blue otherwise), subject info, time, room, type pill, and a countdown
Right — mini month calendar with today highlighted and exam dates
marked as colored dots; below it, a session summary with counts and an
"Until next exam" callout

Documents tab
Submit requests for: academic transcript, enrollment certificate, semester
certificate, diploma duplicate. Form has chip-style picker and a textarea
that's whitespace-trimmed before validation. Past requests below show
status pills (Pending / Processing / Ready / Collected).
Profile tab

Avatar circle with UTF-8-aware initials and a color hashed from the name
Big name (20 pt) + faculty number (16 pt primary blue)
Personal data (username, faculty) and academic data (program, year,
semester, academic year) cards
Language and theme segmented controls (persisted per user)
Export Transcript button → writes data/transcript_F23001.txt with
full breakdown and credit-weighted GPA
Status pill ("Активен")
Logout button

Teacher (admin) dashboard
Top-bar buttons: Add Student, Subjects, График (Schedule),
Statistics, Logout.

Student list — searchable, sortable (name / avg desc / avg asc /
program), with Add Grade and View shortcuts on each row
Add Student modal — full name, username, password, program
Subjects modal — add/remove subjects with the 8-color palette picker
Schedule modal — chip picker for subject (auto-fills lecturer +
semester), day pills Mon-Fri, type pills Lecture/Lab/Seminar, HH:MM time
inputs with validation, plus an existing-entries list with delete buttons
Add Grade modal — picks the student, the subject (activities hidden),
grade value 2–6, optional comment
Student detail screen — drill-down with summary, smart insights, trend
chart, full grade list with delete buttons

Statistics screen
Four stat tiles + three charts:

Bar chart of grade distribution (5 buckets, 2–6)
Bar chart of per-subject class averages (activities excluded)
Line chart of monthly grade averages (last 8 months)

Filter chips at the top let you see all programs combined or just one at a
time.
Project structure
grademaster/
├── README.md              ← this file
├── Makefile               ← cross-platform build (Linux/macOS/Windows-MinGW)
├── .gitignore             ← excludes build/data/IDE artifacts
├── .gitattributes         ← LF line-endings
├── .editorconfig          ← 4-space indent, UTF-8
├── resources/
│   └── README.txt         ← font installation guide
├── data/                  ← runtime data (created on first launch, gitignored)
└── src/
    ├── main.cpp           ← raylib window setup
    ├── app.{h,cpp}        ← top-level controller, transitions, header
    ├── screen.h           ← Screen base class + ScreenId enum
    ├── screen_auth.{h,cpp}     ← login + signup screens
    ├── screen_student.{h,cpp}  ← 6-tab student dashboard + detail view
    ├── screen_teacher.{h,cpp}  ← teacher dashboard + management modals
    ├── screen_stats.{h,cpp}    ← statistics screen
    ├── data_store.{h,cpp}      ← in-memory data + file persistence
    ├── models.{h,cpp}          ← User/Subject/Grade types + helpers
    ├── theme.{h,cpp}           ← palette + Cyrillic font loader
    ├── ui.{h,cpp}              ← all widgets + vector icons + crest
    ├── easing.h                ← header-only animation helpers
    └── i18n.h                  ← header-only BG/EN translation table
Total: ~7,900 lines of C++17 across 25 files.
Tech stack
LayerChoiceLanguageC++17 (works fine with C++20)Graphicsraylib 5.5+ (single static library)UI frameworkCustom — built on raylib primitivesText renderingraylib's LoadFontEx with codepoint setPersistencePlain text key-value file under data/BuildHand-rolled Makefile, no CMake neededExternal depsJust raylib — no JSON parser, no DB, no UI toolkit
Architecture
Update / Draw loop
The standard raylib loop runs at 60 FPS. Every frame:

App::Update(dt) — input handling, animation tweens, screen transitions
App::Draw() — particles, current screen, header widgets, toasts

Each Screen (Login, Student dashboard, Teacher dashboard, Statistics)
implements OnEnter / OnShow / OnHide / Update / Draw. The App class
manages transitions and routes Update/Draw calls to the active screen.
Animation philosophy
Easing::Approach(current, target, halfLifeSeconds, dt) is used everywhere
for frame-rate-independent smoothing. Combined with OutCubic and OutBack
for entry animations, the result feels snappy without being twitchy. There
are no fixed-frame animations — everything is time-based.
Persistence
DataStore::Load() reads data/grademaster.dat, a sectioned text file:
# FutureCode SIS data file v4. Edit by hand at your own risk.
[USERS]
1|teacher|admin|...
[SUBJECTS]
1|MAT101|Висша математика I|...
[GRADES]
...
Every mutating operation (add grade, submit request, update user) calls
Save(). Schema version is checked on load — older versions get backed up
to .bak and reseeded with sample data, so upgrades never lose history.
Internationalization
I18n::T("login.welcome_back") looks up the current language and returns the
correct string. Adding a new language means filling out one extra column in
i18n.h. The default is BG; English is a one-click toggle in the top-right.
Contributing
This is primarily a learning / portfolio project, but contributions are
welcome. Please:

Fork and create a feature branch: git checkout -b feat/your-feature
Use Conventional Commits style:

feat: for new functionality
fix: for bug fixes
refactor: for code restructure with no behavior change
docs: for documentation
chore: for build/tooling


Keep commits focused — one logical change per commit
Make sure make still builds cleanly with no warnings
Open a pull request

Troubleshooting
Cyrillic text shows as boxes / question marks
The fallback font on your system doesn't include Cyrillic glyphs. Drop a
DejaVuSans TTF into resources/ (see resources/README.txt).
raylib.h: No such file or directory
Install raylib using your package manager (see Quick start)
or build from source. On Windows, make sure pkg-config can find raylib
(pkg-config --cflags raylib should print include paths).
Black window on launch (macOS)
You're likely on Apple Silicon and raylib was built for x86. Reinstall with
brew install raylib or rebuild from source for arm64.
Login fails with correct credentials
Delete data/grademaster.dat and data/grademaster.dat.bak to force a fresh
seed. The seeded teacher is always admin/admin.
Schedule tab shows no entries
The seed entries are tagged with semester=2. Make sure your test user has
currentSemester=2 (all seeded students do). Or use the admin's "Schedule"
modal to add entries for other semesters.
Build fails with -fsanitize=address on macOS
Remove sanitizer flags from the Makefile (make SANITIZE=) — Apple Silicon
has limited ASan support depending on Xcode version.
License
This project is released under the MIT License — see LICENSE for full
text. In short: do whatever you want with the code, just keep the copyright
notice. The bundled raylib library is released under the
zlib/libpng license.
If you drop in DejaVuSans or another font, do not commit the TTF — it has
its own license (DejaVu is GPL with font exception, fine to use but
distributable separately).

<div align="center">
Built with C++17, raylib, and a lot of vector icons drawn from primitives.
If this helped you learn something, ⭐ the repo!
</div>

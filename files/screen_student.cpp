#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "screen_student.h"
#include "app.h"
#include "theme.h"
#include "easing.h"
#include "i18n.h"
#include "models.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <set>
#include <map>

namespace {
    inline Font Fnt() { return ThemeMgr::GetFont(); }
    inline Font Bld() { return ThemeMgr::GetFontBold(); }
    inline Font Dis() { return ThemeMgr::GetFontDisplay(); }

    // Format minutes-from-midnight as "HH:MM"
    std::string fmtTime(int minutes) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes / 60, minutes % 60);
        return buf;
    }

    // Compute Julian-day number for sorting / day-difference calculations
    long jdn(int y, int m, int d) {
        long a = (14 - m) / 12;
        long yy = y + 4800 - a;
        long mm = m + 12 * a - 3;
        return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
    }
    int daysBetween(int ay, int am, int ad, int by, int bm, int bd) {
        return (int)(jdn(by, bm, bd) - jdn(ay, am, ad));
    }

    // String form of class kind ("Лекция" / "Lab" etc.)
    const char* kindLabel(ClassKind k) {
        switch (k) {
            case ClassKind::LECTURE: return I18n::T("schedule.lecture");
            case ClassKind::LAB:     return I18n::T("schedule.lab");
            case ClassKind::SEMINAR: return I18n::T("schedule.seminar");
        }
        return "";
    }

    const char* docKindLabel(DocKind k) {
        switch (k) {
            case DocKind::TRANSCRIPT:    return I18n::T("doc.transcript");
            case DocKind::ENROLLMENT:    return I18n::T("doc.enrollment");
            case DocKind::SEMESTER_CERT: return I18n::T("doc.semester_cert");
            case DocKind::DIPLOMA_DUP:   return I18n::T("doc.diploma");
        }
        return "";
    }

    // Status pill colors
    Color docStatusColor(DocStatus s) {
        const Theme& th = ThemeMgr::Current();
        switch (s) {
            case DocStatus::PENDING:    return th.warning;
            case DocStatus::PROCESSING: return th.primary;
            case DocStatus::READY:      return th.success;
            case DocStatus::COLLECTED:  return th.textMuted;
        }
        return th.textMuted;
    }

    const char* docStatusLabel(DocStatus s) {
        switch (s) {
            case DocStatus::PENDING:    return I18n::T("doc.status.pending");
            case DocStatus::PROCESSING: return I18n::T("doc.status.processing");
            case DocStatus::READY:      return I18n::T("doc.status.ready");
            case DocStatus::COLLECTED:  return I18n::T("doc.status.collected");
        }
        return "";
    }

    // ============================================================
    // Export transcript: writes a plain-text academic transcript to
    // data/transcript_<facultynumber>.txt. Returns the path on
    // success, or an empty string on failure.
    // ============================================================
    std::string ExportTranscript(DataStore& data, const User& u) {
        std::filesystem::path outDir = "data";
        try { std::filesystem::create_directories(outDir); } catch (...) {}
        std::string fn = "transcript_" + (u.facultyNumber.empty() ? u.username : u.facultyNumber) + ".txt";
        std::filesystem::path path = outDir / fn;
        std::ofstream out(path);
        if (!out.is_open()) return "";

        // Header
        out << "================================================================\n";
        out << "  FutureCode - Технологичен университет\n";
        out << "  АКАДЕМИЧНА СПРАВКА (TRANSCRIPT OF RECORDS)\n";
        out << "================================================================\n\n";

        out << "Студент: " << u.fullName << "\n";
        out << "Факултетен номер: " << u.facultyNumber << "\n";
        out << "Факултет: " << u.faculty << "\n";
        out << "Специалност: " << u.className << "\n";
        out << "Курс: " << u.year << "    Семестър: " << u.currentSemester << "\n";

        int ny, nm, nd; Models::TodayYMD(ny, nm, nd);
        out << "Учебна година: " << Models::AcademicYear(ny, nm) << "\n";
        out << "Дата: " << Models::FormatDate(ny, nm, nd) << "\n\n";

        // Group grades by subject
        out << "----------------------------------------------------------------\n";
        out << " Код    | Дисциплина                          |  Кр. | Оценка\n";
        out << "----------------------------------------------------------------\n";

        auto& subjects = data.Subjects();
        auto& grades   = data.Grades();
        int totalCredits = 0, earnedCredits = 0;
        float weighted = 0; int weightedDenom = 0;

        for (const auto& s : subjects) {
            if (s.semester > u.currentSemester) continue;
            float avg = Models::StudentSubjectAverage(u.id, s.id, grades);
            char line[256];
            if (avg < 0) {
                std::snprintf(line, sizeof(line), " %-6s | %-36s | %4d | %s\n",
                              s.code.c_str(), s.name.c_str(), s.credits,
                              "—       ");
            } else {
                std::snprintf(line, sizeof(line), " %-6s | %-36s | %4d | %4.2f   \n",
                              s.code.c_str(), s.name.c_str(), s.credits, avg);
                if (avg >= 3.0f) earnedCredits += s.credits;
                weighted += avg * s.credits;
                weightedDenom += s.credits;
            }
            out << line;
            totalCredits += s.credits;
        }
        out << "----------------------------------------------------------------\n\n";

        out << "Общо кредити по програма : " << totalCredits << "\n";
        out << "Спечелени кредити         : " << earnedCredits << "\n";
        if (weightedDenom > 0) {
            out << "Среден успех (по кредити) : "
                << (weighted / (float)weightedDenom) << "\n";
        }
        float ovr = Models::StudentOverallAverage(u.id, grades);
        if (ovr >= 0) {
            out << "Среден успех (общ)        : " << ovr << "\n";
        }

        out << "\n================================================================\n";
        out << "Този документ е генериран автоматично от Student SIS.\n";
        out << "================================================================\n";
        return path.string();
    }

    // Returns 0..6 dayOfWeek for a given date (0 = Mon)
    int dayOfWeek(int y, int m, int d) {
        std::tm tm{};
        tm.tm_year = y - 1900; tm.tm_mon = m - 1; tm.tm_mday = d;
        tm.tm_hour = 12;
        std::time_t t = std::mktime(&tm);
        std::tm lt{};
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        // tm_wday: 0=Sunday..6=Saturday. Convert to Mon-based.
        int w = lt.tm_wday;
        return (w + 6) % 7;  // 0 = Mon
    }
}

// ========================================================================
//  StudentDashboard
// ========================================================================

void StudentDashboard::OnEnter(App& app) {
    (void)app;
    logoutBtn.kind = UI::Button::GHOST;
    logoutBtn.label = I18n::T("common.logout");

    profileLogoutBtn.kind = UI::Button::DANGER;
    profileLogoutBtn.label = I18n::T("common.logout");

    docPurpose.placeholder = I18n::T("doc.purpose.placeholder");
    docPurpose.maxLength = 200;
    submitDocBtn.kind = UI::Button::PRIMARY;
    submitDocBtn.label = I18n::T("doc.submit");

    gpaCircle.colorByValue = true;
    gpaCircle.thickness = 10.0f;
}

void StudentDashboard::OnShow(App& app) {
    enterT = 0.0f;
    activeTab = StudentTab::GRADES;
    docSubmitted = false;
    docPurpose.value.clear();

    User* u = app.CurrentUser();
    profileLangChoice  = (I18n::Current() == Lang::EN) ? 1 : 0;
    profileThemeChoice = (u && u->darkMode) ? 1 : 0;

    // Initialize the GPA circle target now so it animates in.
    if (u) {
        float avg = Models::StudentOverallAverage(u->id, app.Data().Grades());
        if (avg < 0) avg = 0;
        gpaCircle.SetTarget(avg / 6.0f);
    }
}

void StudentDashboard::Update(App& app, float dt, Vector2 mouse) {
    enterT = Easing::Approach(enterT, 1.0f, 0.30f, dt);
    tabSwitchT = Easing::Approach(tabSwitchT, 1.0f, 0.18f, dt);
    gpaCircle.Update(dt);

    int sw = app.W();
    // Top-bar logout - sits to the LEFT of the global theme/notif controls
    Vector2 lm = UI::MeasureText(Bld(), logoutBtn.label.c_str(), 13.0f);
    logoutBtn.bounds = { (float)sw - lm.x - 380, 30, lm.x + 44, 38 };
    logoutBtn.Update(dt, mouse);
    if (logoutBtn.Clicked(mouse)) { app.Logout(); return; }

    // Smooth scroll
    int idx = (int)activeTab;
    scrollY[idx] = Easing::Approach(scrollY[idx], scrollTarget[idx], 0.15f, dt);
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) scrollTarget[idx] -= wheel * 60.0f;
    if (scrollTarget[idx] < 0) scrollTarget[idx] = 0;

    // Tab switching by keyboard 1-6
    auto switchTo = [&](StudentTab t){
        if (activeTab != t) { activeTab = t; tabSwitchT = 0.0f; }
    };
    if (IsKeyPressed(KEY_ONE))   switchTo(StudentTab::GRADES);
    if (IsKeyPressed(KEY_TWO))   switchTo(StudentTab::CURRICULUM);
    if (IsKeyPressed(KEY_THREE)) switchTo(StudentTab::SCHEDULE);
    if (IsKeyPressed(KEY_FOUR))  switchTo(StudentTab::EXAMS);
    if (IsKeyPressed(KEY_FIVE))  switchTo(StudentTab::DOCUMENTS);
    if (IsKeyPressed(KEY_SIX))   switchTo(StudentTab::PROFILE);
    if (IsKeyPressed(KEY_F1))    showHelp = !showHelp;
    if (IsKeyPressed(KEY_ESCAPE) && showHelp) showHelp = false;
    helpT = Easing::Approach(helpT, showHelp ? 1.0f : 0.0f, 0.10f, dt);

    // Ctrl+E - export transcript to data/transcript_<fn>.txt
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrl && IsKeyPressed(KEY_E)) {
        if (User* uu = app.CurrentUser()) {
            std::string p = ExportTranscript(app.Data(), *uu);
            if (!p.empty()) {
                std::string msg = (I18n::Current() == Lang::BG
                    ? "Справката е експортирана: "
                    : "Transcript exported to: ") + p;
                app.Toast(msg, ThemeMgr::Current().success);
            } else {
                app.Toast(I18n::Current() == Lang::BG
                              ? "Грешка при експорт"
                              : "Export failed",
                          ThemeMgr::Current().error);
            }
        }
    }

    // Click on tabs - bounds were set during last draw
    for (int i = 0; i < 6; ++i) {
        if (CheckCollisionPointRec(mouse, tabBounds[i])) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                switchTo((StudentTab)i);
        }
    }

    // Per-tab interactions
    User* u = app.CurrentUser();
    if (!u) return;

    if (activeTab == StudentTab::DOCUMENTS) {
        docPurpose.Update(dt, mouse);
        if (submitDocBtn.Clicked(mouse)) {
            if (docPurpose.value.empty()) {
                app.Toast(I18n::T("doc.empty_purpose"), ThemeMgr::Current().warning);
            } else {
                app.Data().SubmitDocumentRequest(u->id, selectedDocKind, docPurpose.value);
                docPurpose.value.clear();
                docSubmitted = true;
                app.Toast(I18n::T("doc.submitted"), ThemeMgr::Current().success);
            }
        }
        submitDocBtn.Update(dt, mouse);
    }

    if (activeTab == StudentTab::PROFILE) {
        profileLogoutBtn.Update(dt, mouse);
        if (profileLogoutBtn.Clicked(mouse)) { app.Logout(); return; }
    }
}

// ------------------------------------------------------------------------
//  Top bar + tab strip
// ------------------------------------------------------------------------

void StudentDashboard::DrawTopBar(App& app, Rectangle r) {
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();

    // University crest (left)
    UI::DrawUniversityCrest({ r.x + 44, r.y + r.height / 2 }, 26.0f, th.primary, WHITE);
    UI::DrawTextLeft(Bld(), I18n::T("app.university"),
                     { r.x + 90, r.y + 22 }, 18.0f, th.textPrimary);
    UI::DrawTextLeft(Fnt(), I18n::T("app.subtitle"),
                     { r.x + 90, r.y + 48 }, 13.0f, th.textMuted);

    // Greeting + name + faculty number (center) - now significantly bigger
    if (u) {
        // Line 1: greeting (small, muted)
        const char* greeting = I18n::T("student.greeting");
        Vector2 gm = UI::MeasureText(Fnt(), greeting, 13.0f);

        // Line 2: full name (big, bold, primary)
        Vector2 nm = UI::MeasureText(Bld(), u->fullName.c_str(), 22.0f);

        // Line 3: faculty number block (medium-large, with label)
        std::string fnLine = std::string(I18n::T("student.faculty_number")) +
                             "  " + u->facultyNumber;
        Vector2 fm = UI::MeasureText(Bld(), fnLine.c_str(), 15.0f);

        float maxW = nm.x;
        if (fm.x > maxW) maxW = fm.x;
        float blockX = r.x + r.width / 2 - maxW / 2;

        UI::DrawTextLeft(Fnt(), greeting,
                         { blockX, r.y + 12 }, 13.0f, th.textMuted);
        UI::DrawTextLeft(Bld(), u->fullName.c_str(),
                         { blockX, r.y + 30 }, 22.0f, th.textPrimary);
        UI::DrawTextLeft(Bld(), fnLine.c_str(),
                         { blockX, r.y + 62 }, 15.0f, th.primary);
        (void)gm;
    }

    // Logout (right) - draw as a pill with icon + text
    {
        Rectangle b = logoutBtn.bounds;
        bool hov = CheckCollisionPointRec(GetMousePosition(), b);
        Color bg = hov ? ColorAlpha(th.error, 0.10f) : th.surfaceMuted;
        DrawRectangleRounded(b, 0.5f, 6, bg);
        UI::IconLogout({ b.x + 16, b.y + b.height * 0.5f }, 9.0f,
                       hov ? th.error : th.textSecondary, 1.8f);
        Vector2 lm = UI::MeasureText(Bld(), logoutBtn.label.c_str(), 13.0f);
        DrawTextEx(Bld(), logoutBtn.label.c_str(),
                   { b.x + b.width - lm.x - 14, b.y + b.height * 0.5f - 8 },
                   13.0f, 1.0f, hov ? th.error : th.textSecondary);
    }
}

void StudentDashboard::DrawTabStrip(App& app, Rectangle r, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    const Theme& th = ThemeMgr::Current();
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)r.height, th.surface);
    DrawRectangle((int)r.x, (int)(r.y + r.height - 1), (int)r.width, 1, th.border);

    const char* keys[] = {
        "tab.grades", "tab.curriculum", "tab.schedule",
        "tab.exams", "tab.documents", "tab.profile"
    };

    // Fixed left padding, evenly spaced labels
    float x = r.x + 44;
    float gap = 36;
    for (int i = 0; i < 6; ++i) {
        const char* label = I18n::T(keys[i]);
        Vector2 m = UI::MeasureText(Bld(), label, 15.0f);
        Rectangle b = { x, r.y, m.x + 18, r.height };
        tabBounds[i] = b;

        bool active = (int)activeTab == i;
        bool hov = CheckCollisionPointRec(GetMousePosition(), b);
        Color c = active ? th.primary : (hov ? th.textPrimary : th.textSecondary);
        UI::DrawTextCenter(Bld(), label, b, 15.0f, c, 0.5f);

        if (active) {
            // Animate underline towards this tab
            float targetX = b.x + 8;
            float targetW = b.width - 16;
            if (activeUnderlineW == 0.0f) {
                activeUnderlineX = targetX;
                activeUnderlineW = targetW;
            } else {
                activeUnderlineX = Easing::Approach(activeUnderlineX, targetX, 0.10f, dt);
                activeUnderlineW = Easing::Approach(activeUnderlineW, targetW, 0.10f, dt);
            }
        }

        x += b.width + gap;
    }

    // Animated underline
    DrawRectangle((int)activeUnderlineX, (int)(r.y + r.height - 3),
                  (int)activeUnderlineW, 3, th.primary);
}

// ------------------------------------------------------------------------
//  TAB: Grades
// ------------------------------------------------------------------------

void StudentDashboard::DrawTabGrades(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();
    if (!u) return;

    // ---- Stats strip: GPA / credits earned / semester avg / passed count ----
    auto& subjects = app.Data().Subjects();
    auto& grades   = app.Data().Grades();

    float overall = Models::StudentOverallAverage(u->id, grades);
    int   passed = 0, totalSubj = 0;
    int   creditsEarned = 0, creditsTotal = 0;
    std::map<int, std::pair<float,int>> subjAcc;  // subjId -> {sum, count}
    for (const auto& g : grades) {
        if (g.studentId != u->id) continue;
        auto& a = subjAcc[g.subjectId];
        a.first += g.value; a.second += 1;
    }
    // Walk all subjects so credits include subjects with no grade yet
    for (const auto& s : subjects) {
        if (s.semester > u->currentSemester) continue;
        creditsTotal += s.credits;
        totalSubj++;
        auto it = subjAcc.find(s.id);
        if (it != subjAcc.end()) {
            float avg = it->second.first / it->second.second;
            if (avg >= 3.0f) {
                passed++;
                creditsEarned += s.credits;
            }
        }
    }

    Rectangle strip = { area.x + 24, area.y + 24, area.width - 48, 130 };

    auto drawTile = [&](Rectangle r, const char* title, const char* big, Color accent) {
        UI::DrawSoftShadow(r, 0.10f, 16.0f, 0.06f);
        DrawRectangleRounded(r, 0.10f, 8, th.surface);
        UI::StrokeRoundedRect(r, 0.10f, 8, 1.0f, th.border);
        // Side stripe in accent color
        DrawRectangleRounded({ r.x, r.y, 4, r.height }, 1.0f, 4, accent);
        UI::DrawTextLeft(Fnt(), title, { r.x + 18, r.y + 16 }, 13.0f, th.textMuted);
        UI::DrawTextLeft(Dis(), big, { r.x + 18, r.y + 38 }, 34.0f, th.textPrimary);
    };

    float tileW = (strip.width - 36) / 4;
    Rectangle tile1 = { strip.x,                    strip.y, tileW, strip.height };
    Rectangle tile2 = { strip.x + tileW + 12,       strip.y, tileW, strip.height };
    Rectangle tile3 = { strip.x + (tileW + 12) * 2, strip.y, tileW, strip.height };
    Rectangle tile4 = { strip.x + (tileW + 12) * 3, strip.y, tileW, strip.height };

    // ---- GPA tile (unique: sparkline + trend arrow + animated value) ----
    {
        UI::DrawSoftShadow(tile1, 0.10f, 16.0f, 0.06f);
        DrawRectangleRounded(tile1, 0.10f, 8, th.surface);
        UI::StrokeRoundedRect(tile1, 0.10f, 8, 1.0f, th.border);
        DrawRectangleRounded({ tile1.x, tile1.y, 4, tile1.height }, 1.0f, 4, th.primary);
        UI::DrawTextLeft(Fnt(), I18n::T("student.gpa"),
                         { tile1.x + 18, tile1.y + 16 }, 13.0f, th.textMuted);

        // Animated number using gpaCircle.animValue (which is 0..1 mapping to 0..6)
        char gb[16];
        if (overall >= 0)
            std::snprintf(gb, sizeof(gb), "%.2f", gpaCircle.animValue * 6.0f);
        else
            std::snprintf(gb, sizeof(gb), "—");
        UI::DrawTextLeft(Dis(), gb, { tile1.x + 18, tile1.y + 38 }, 34.0f, th.textPrimary);

        // Trend arrow (delta vs grade right before the most recent one)
        std::vector<float> series = Models::RecentGradeSeries(u->id, grades, 8);
        if (series.size() >= 3) {
            // Recent half average vs older half
            size_t mid = series.size() / 2;
            float older = 0, newer = 0;
            for (size_t i = 0; i < mid; ++i) older += series[i];
            for (size_t i = mid; i < series.size(); ++i) newer += series[i];
            older /= (float)mid;
            newer /= (float)(series.size() - mid);
            float delta = newer - older;
            if (std::fabs(delta) >= 0.10f) {
                Color tc = (delta > 0) ? th.success : th.error;
                Vector2 ic = { tile1.x + 84, tile1.y + 50 };
                if (delta > 0) UI::IconArrowUp(ic, 9.0f, tc, 2.0f);
                else           UI::IconArrowDown(ic, 9.0f, tc, 2.0f);
                char tb[16];
                std::snprintf(tb, sizeof(tb), "%+.2f", delta);
                UI::DrawTextLeft(Bld(), tb, { ic.x + 12, tile1.y + 44 }, 12.0f, tc);
            }
        }

        // Sparkline at bottom of tile
        if (series.size() >= 2) {
            Rectangle spark = {
                tile1.x + 18, tile1.y + tile1.height - 36,
                tile1.width - 36, 22
            };
            UI::DrawSparkline(spark, series, th.primary, 1.6f, true, 2.0f, 6.0f);
        }
    }

    char ceBuf[32];
    std::snprintf(ceBuf, sizeof(ceBuf), "%d / %d", creditsEarned, creditsTotal);
    drawTile(tile2, I18n::T("student.credits_earned"), ceBuf, th.success);

    // Add small ECTS note below the big number
    {
        char nb[32];
        int pct = creditsTotal > 0 ? (creditsEarned * 100 / creditsTotal) : 0;
        std::snprintf(nb, sizeof(nb), "%d%% ECTS", pct);
        UI::DrawTextLeft(Fnt(), nb, { tile2.x + 18, tile2.y + tile2.height - 24 }, 11.0f, th.textMuted);
    }

    char psBuf[16];
    std::snprintf(psBuf, sizeof(psBuf), "%d / %d", passed, totalSubj);
    drawTile(tile3, I18n::Current() == Lang::BG ? "Положени дисциплини" : "Subjects passed",
             psBuf, th.accent);

    // Tile 4: Show semester + year + academic year string
    {
        UI::DrawSoftShadow(tile4, 0.10f, 16.0f, 0.06f);
        DrawRectangleRounded(tile4, 0.10f, 8, th.surface);
        UI::StrokeRoundedRect(tile4, 0.10f, 8, 1.0f, th.border);
        DrawRectangleRounded({ tile4.x, tile4.y, 4, tile4.height }, 1.0f, 4, th.secondary);
        UI::DrawTextLeft(Fnt(), I18n::T("student.semester"),
                         { tile4.x + 18, tile4.y + 16 }, 13.0f, th.textMuted);
        char sb[16];
        std::snprintf(sb, sizeof(sb), "%d", u->currentSemester);
        UI::DrawTextLeft(Dis(), sb, { tile4.x + 18, tile4.y + 38 }, 34.0f, th.textPrimary);

        int nowY, nowM, nowD; Models::TodayYMD(nowY, nowM, nowD);
        std::string ay = Models::AcademicYear(nowY, nowM);
        UI::DrawTextLeft(Fnt(), ay.c_str(),
                         { tile4.x + 18, tile4.y + tile4.height - 24 }, 11.0f, th.textMuted);
    }

    // ---- Grades list ----
    Rectangle listArea = { area.x + 24, strip.y + strip.height + 16,
                            area.width - 48, area.height - strip.height - 64 };
    UI::DrawSoftShadow(listArea, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(listArea, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(listArea, 0.04f, 8, 1.0f, th.border);

    UI::DrawTextLeft(Bld(), I18n::T("student.recent_grades"),
                     { listArea.x + 22, listArea.y + 18 }, 18.0f, th.textPrimary);

    // Collect grades for this student, latest first
    std::vector<const Grade*> mine;
    for (const auto& g : grades) if (g.studentId == u->id) mine.push_back(&g);
    std::sort(mine.begin(), mine.end(),
              [](const Grade* a, const Grade* b){ return a->createdAt > b->createdAt; });

    Rectangle scroll = { listArea.x + 12, listArea.y + 50,
                         listArea.width - 24, listArea.height - 64 };
    BeginScissorMode((int)scroll.x, (int)scroll.y, (int)scroll.width, (int)scroll.height);
    float py = scroll.y - scrollY[(int)StudentTab::GRADES];

    if (mine.empty()) {
        UI::DrawTextCenter(Fnt(), I18n::T("student.no_grades"),
                           { scroll.x, scroll.y + 30, scroll.width, 24 },
                           14.0f, th.textMuted, 0.5f);
    }

    for (const Grade* gp : mine) {
        Rectangle row = { scroll.x + 4, py, scroll.width - 8, 72 };
        // Hover state
        bool hov = CheckCollisionPointRec(GetMousePosition(), row);
        Color rowBg = hov ? th.surfaceAlt : th.surfaceMuted;
        DrawRectangleRounded(row, 0.10f, 6, rowBg);

        Subject* s = app.Data().FindSubject(gp->subjectId);
        Color subjC = s ? UI::SubjectColor(s->colorIndex) : th.primary;
        // Color stripe
        DrawRectangleRounded({ row.x, row.y, 4, row.height }, 1.0f, 4, subjC);

        // Code + name
        std::string code = s ? s->code : "—";
        std::string name = s ? s->name : "—";
        UI::DrawTextLeft(Bld(), code.c_str(), { row.x + 22, row.y + 12 }, 13.0f, th.textMuted);
        UI::DrawTextLeft(Bld(), name.c_str(), { row.x + 22, row.y + 30 }, 16.0f, th.textPrimary);

        // Lecturer + date (right side of subject)
        std::string detail = (s ? s->lecturer : std::string()) + "   ·   " +
                             Models::FormatDate(gp->year, gp->month, gp->day);
        UI::DrawTextLeft(Fnt(), detail.c_str(),
                         { row.x + 22, row.y + 52 }, 12.0f, th.textMuted);

        // Grade value box on the right
        Rectangle gb = { row.x + row.width - 92, row.y + 14, 72, 44 };
        Color gc = ThemeMgr::GradeColor((float)gp->value);
        DrawRectangleRounded(gb, 0.30f, 6, ColorAlpha(gc, 0.18f));
        UI::StrokeRoundedRect(gb, 0.30f, 6, 1.0f, gc);
        char gv[8]; std::snprintf(gv, sizeof(gv), "%d", gp->value);
        UI::DrawTextCenter(Bld(), gv, gb, 26.0f, gc, 0.5f);

        py += 80;
    }
    // Update scroll bound
    float used = py + scrollY[(int)StudentTab::GRADES] - scroll.y;
    if (used < scroll.height) scrollTarget[(int)StudentTab::GRADES] = 0;
    else if (scrollTarget[(int)StudentTab::GRADES] > used - scroll.height)
        scrollTarget[(int)StudentTab::GRADES] = used - scroll.height;

    EndScissorMode();
}

// ------------------------------------------------------------------------
//  TAB: Curriculum
// ------------------------------------------------------------------------

void StudentDashboard::DrawTabCurriculum(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt;
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();
    if (!u) return;

    // Group subjects by semester
    auto& subjects = app.Data().Subjects();
    auto& grades   = app.Data().Grades();
    std::map<int, std::vector<const Subject*>> bySem;
    for (const auto& s : subjects) bySem[s.semester].push_back(&s);

    Rectangle scroll = { area.x + 24, area.y + 24,
                         area.width - 48, area.height - 48 };
    BeginScissorMode((int)scroll.x, (int)scroll.y, (int)scroll.width, (int)scroll.height);
    float py = scroll.y - scrollY[(int)StudentTab::CURRICULUM];

    // Column geometry (absolute positions inside the scroll viewport).
    // We allocate a fixed right-side block for credits/hours/type/grade so
    // long course names + lecturer text don't crash into them.
    const float colCodeX  = 18;
    const float colNameX  = 90;
    const float colNameW  = scroll.width - 90 - 380;     // course name area
    const float colCreditsX = scroll.width - 360;
    const float colHoursX   = scroll.width - 280;
    const float colTypeX    = scroll.width - 190;
    const float colGradeX   = scroll.width - 88;

    for (auto& kv : bySem) {
        int sem = kv.first;
        auto& list = kv.second;

        // Section header card with book icon
        Rectangle hdr = { scroll.x, py, scroll.width, 48 };
        DrawRectangleRounded(hdr, 0.20f, 6, th.surfaceMuted);
        UI::IconBook({ hdr.x + 22, hdr.y + 24 }, 10.0f, th.primary);
        char hb[64];
        std::snprintf(hb, sizeof(hb), "%s %d  ·  %s %d",
                      I18n::T("curriculum.semester"), sem,
                      I18n::T("student.year"), (sem + 1) / 2);
        UI::DrawTextLeft(Bld(), hb, { hdr.x + 42, hdr.y + 14 }, 14.0f, th.textPrimary);

        // Right side: total credits (bold) + count
        int sumCr = 0; for (auto* s : list) sumCr += s->credits;
        char crBuf[40];
        std::snprintf(crBuf, sizeof(crBuf), "%d %s · %d %s",
                      sumCr, I18n::T("curriculum.credits"),
                      (int)list.size(),
                      I18n::Current() == Lang::BG ? "дисциплини" : "courses");
        UI::DrawTextRight(Fnt(), crBuf,
                          { hdr.x, hdr.y + 16, hdr.width - 18, 16 }, 12.0f, th.textMuted);

        py += 56;

        // Column headers
        Rectangle ch = { scroll.x, py, scroll.width, 22 };
        UI::DrawTextLeft(Fnt(), I18n::T("curriculum.code"),     { ch.x + colCodeX,    ch.y }, 11.0f, th.textMuted);
        UI::DrawTextLeft(Fnt(), I18n::T("curriculum.course"),   { ch.x + colNameX,    ch.y }, 11.0f, th.textMuted);
        UI::DrawTextCenter(Fnt(), I18n::T("curriculum.credits"),
                           { ch.x + colCreditsX - 24, ch.y, 60, 16 }, 11.0f, th.textMuted, 0.5f);
        UI::DrawTextCenter(Fnt(), I18n::T("curriculum.hours"),
                           { ch.x + colHoursX - 24, ch.y, 60, 16 }, 11.0f, th.textMuted, 0.5f);
        UI::DrawTextCenter(Fnt(), I18n::T("curriculum.type"),
                           { ch.x + colTypeX - 30, ch.y, 76, 16 }, 11.0f, th.textMuted, 0.5f);
        UI::DrawTextCenter(Fnt(), I18n::T("curriculum.grade"),
                           { ch.x + colGradeX - 26, ch.y, 60, 16 }, 11.0f, th.textMuted, 0.5f);
        py += 26;

        // Sort by code so display is consistent
        std::sort(list.begin(), list.end(),
                  [](const Subject* a, const Subject* b){ return a->code < b->code; });

        for (auto* s : list) {
            Rectangle row = { scroll.x, py, scroll.width, 60 };
            bool hov = CheckCollisionPointRec(mouse, row);
            DrawRectangleRounded(row, 0.06f, 4, hov ? th.surfaceAlt : th.surface);
            UI::StrokeRoundedRect(row, 0.06f, 4, 1.0f, hov ? th.borderStrong : th.border);

            Color subjC = UI::SubjectColor(s->colorIndex);
            DrawRectangleRounded({ row.x, row.y, 4, row.height }, 1.0f, 4, subjC);

            // Code (bold colored) + name + lecturer
            UI::DrawTextLeft(Bld(), s->code.c_str(),
                             { row.x + colCodeX, row.y + 12 }, 12.0f, subjC);

            // Truncate name to fit colNameW
            std::string truncName = UI::Ellipsize(Bld(), s->name, colNameW - 6, 14.0f, 1.0f);
            UI::DrawTextLeft(Bld(), truncName.c_str(),
                             { row.x + colNameX, row.y + 10 }, 14.0f, th.textPrimary);
            std::string truncLec = UI::Ellipsize(Fnt(), s->lecturer, colNameW - 6, 11.0f, 1.0f);
            UI::DrawTextLeft(Fnt(), truncLec.c_str(),
                             { row.x + colNameX, row.y + 32 }, 11.0f, th.textMuted);

            // Credits
            char crb[8]; std::snprintf(crb, sizeof(crb), "%d", s->credits);
            UI::DrawTextCenter(Bld(), crb,
                               { row.x + colCreditsX - 24, row.y + 22, 60, 18 },
                               16.0f, th.textPrimary, 0.5f);
            // Hours
            char hrb[16]; std::snprintf(hrb, sizeof(hrb), "%d", s->hoursPerWeek);
            UI::DrawTextCenter(Fnt(), hrb,
                               { row.x + colHoursX - 24, row.y + 24, 60, 16 },
                               14.0f, th.textSecondary, 0.5f);
            // Type pill
            const char* tlbl = s->elective
                ? I18n::T("curriculum.elective")
                : I18n::T("curriculum.compulsory");
            Color tc = s->elective ? th.accent : th.success;
            Rectangle tp = { row.x + colTypeX - 38, row.y + 22, 90, 22 };
            DrawRectangleRounded(tp, 0.5f, 6, ColorAlpha(tc, 0.12f));
            UI::DrawTextCenter(Fnt(), tlbl, tp, 10.0f, tc, 0.5f);

            // Grade status: find latest grade for this subject
            int latestVal = -1;
            long long latestTs = 0;
            for (const auto& g : grades) {
                if (g.studentId == u->id && g.subjectId == s->id && g.createdAt > latestTs) {
                    latestVal = g.value; latestTs = g.createdAt;
                }
            }
            Rectangle gb = { row.x + colGradeX - 22, row.y + 14, 44, 32 };
            if (latestVal > 0) {
                Color gc = ThemeMgr::GradeColor((float)latestVal);
                DrawRectangleRounded(gb, 0.30f, 6, ColorAlpha(gc, 0.15f));
                UI::StrokeRoundedRect(gb, 0.30f, 6, 1.0f, gc);
                char gv[8]; std::snprintf(gv, sizeof(gv), "%d", latestVal);
                UI::DrawTextCenter(Bld(), gv, gb, 18.0f, gc, 0.5f);
            } else {
                DrawRectangleRounded(gb, 0.30f, 6, th.surfaceMuted);
                UI::DrawTextCenter(Fnt(), "—", gb, 14.0f, th.textMuted, 0.5f);
            }

            py += 68;
        }
        py += 12;
    }

    float used = py + scrollY[(int)StudentTab::CURRICULUM] - scroll.y;
    if (used < scroll.height) scrollTarget[(int)StudentTab::CURRICULUM] = 0;
    else if (scrollTarget[(int)StudentTab::CURRICULUM] > used - scroll.height)
        scrollTarget[(int)StudentTab::CURRICULUM] = used - scroll.height;

    EndScissorMode();
}

// ------------------------------------------------------------------------
//  TAB: Schedule (Today's classes panel + weekly grid)
// ------------------------------------------------------------------------

void StudentDashboard::DrawTabSchedule(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();
    if (!u) return;

    Rectangle pad = { area.x + 24, area.y + 24, area.width - 48, area.height - 48 };

    // ---- Title row ----
    UI::DrawTextLeft(Dis(), I18n::T("schedule.title"),
                     { pad.x, pad.y }, 22.0f, th.textPrimary);
    int nowY, nowM, nowD, nowHr, nowMn;
    Models::NowYMDHM(nowY, nowM, nowD, nowHr, nowMn);
    int todayDow = Models::DayOfWeek(nowY, nowM, nowD);
    char subBuf[96];
    std::snprintf(subBuf, sizeof(subBuf), "%s %s  ·  %s %d  ·  %s %d",
                  I18n::Current() == Lang::BG ? "Учебна година" : "Academic year",
                  Models::AcademicYear(nowY, nowM).c_str(),
                  I18n::T("curriculum.semester"), u->currentSemester,
                  I18n::T("student.year"), u->year);
    UI::DrawTextRight(Fnt(), subBuf,
                      { pad.x, pad.y + 6, pad.width, 16 }, 12.0f, th.textMuted);

    // ---- Today's classes strip (above grid) ----
    auto& schedule = app.Data().Schedule();
    std::vector<const ScheduleEntry*> todays;
    for (const auto& e : schedule) {
        if (e.semester != u->currentSemester) continue;
        if (!e.program.empty() && e.program != u->className) continue;
        if (e.dayOfWeek == todayDow) todays.push_back(&e);
    }
    std::sort(todays.begin(), todays.end(),
              [](const ScheduleEntry* a, const ScheduleEntry* b){
                  return a->startMinutes < b->startMinutes;
              });

    Rectangle todayCard = { pad.x, pad.y + 44, pad.width, 110 };
    UI::DrawSoftShadow(todayCard, 0.06f, 14.0f, 0.05f);
    DrawRectangleRounded(todayCard, 0.06f, 8, th.surface);
    UI::StrokeRoundedRect(todayCard, 0.06f, 8, 1.0f, th.border);

    // Calendar icon + heading
    UI::IconCalendar({ todayCard.x + 24, todayCard.y + 22 }, 12.0f, th.primary, 2.0f);
    std::string todayHeading;
    if (todayDow >= 0 && todayDow < 5) {
        todayHeading = std::string(I18n::Current() == Lang::BG ? "Днес · " : "Today · ") +
                       I18n::DayLong(todayDow);
    } else {
        todayHeading = I18n::Current() == Lang::BG ? "Уикенд" : "Weekend";
    }
    UI::DrawTextLeft(Bld(), todayHeading.c_str(),
                     { todayCard.x + 44, todayCard.y + 14 }, 14.0f, th.textPrimary);

    char dateBuf[40];
    std::snprintf(dateBuf, sizeof(dateBuf), "%d %s %d", nowD,
                  I18n::MonthShort(nowM), nowY);
    UI::DrawTextRight(Fnt(), dateBuf,
                      { todayCard.x, todayCard.y + 14, todayCard.width - 24, 14 },
                      12.0f, th.textMuted);

    if (todays.empty()) {
        // Empty state with icon
        UI::IconCalendar({ todayCard.x + todayCard.width / 2, todayCard.y + 70 }, 16.0f,
                         th.textMuted, 1.5f);
        UI::DrawTextCenter(Fnt(), I18n::T("schedule.no_classes"),
                           { todayCard.x, todayCard.y + 86, todayCard.width, 16 },
                           12.0f, th.textMuted, 0.5f);
    } else {
        // Pill-shaped class list
        float px = todayCard.x + 20;
        for (const ScheduleEntry* e : todays) {
            Subject* s = app.Data().FindSubject(e->subjectId);
            Color sc = s ? UI::SubjectColor(s->colorIndex) : th.primary;

            // Determine if this class is "now", "next", or "past"
            int curMin = nowHr * 60 + nowMn;
            bool isNow = (curMin >= e->startMinutes && curMin < e->endMinutes);
            bool isPast = (curMin >= e->endMinutes);

            float pw = 168;
            Rectangle pill = { px, todayCard.y + 46, pw, 50 };
            float alpha = isPast ? 0.4f : 1.0f;
            DrawRectangleRounded(pill, 0.18f, 6, ColorAlpha(sc, isNow ? 0.22f : 0.10f));
            DrawRectangleRounded({ pill.x, pill.y, 3, pill.height }, 1.0f, 4,
                                 ColorAlpha(sc, alpha));
            if (isNow) {
                // Pulsing border for "now"
                float pulse = 0.5f + 0.5f * std::sin((float)GetTime() * 3.0f);
                UI::StrokeRoundedRect(pill, 0.18f, 6, 1.5f, ColorAlpha(sc, 0.4f + 0.4f * pulse));
            }

            std::string code = s ? s->code : "?";
            UI::DrawTextLeft(Bld(), code.c_str(),
                             { pill.x + 12, pill.y + 6 }, 12.0f,
                             ColorAlpha(th.textPrimary, alpha));
            std::string when = fmtTime(e->startMinutes) + " · " + e->room;
            UI::DrawTextLeft(Fnt(), when.c_str(),
                             { pill.x + 12, pill.y + 22 }, 10.0f,
                             ColorAlpha(th.textSecondary, alpha));
            std::string kt = kindLabel(e->kind);
            UI::DrawTextLeft(Fnt(), kt.c_str(),
                             { pill.x + 12, pill.y + 36 }, 10.0f,
                             ColorAlpha(th.textMuted, alpha));

            if (isNow) {
                // "Now" badge
                Rectangle nb = { pill.x + pill.width - 50, pill.y + 6, 42, 18 };
                DrawRectangleRounded(nb, 0.5f, 4, ColorAlpha(th.success, 0.25f));
                UI::DrawTextCenter(Bld(),
                                   I18n::Current() == Lang::BG ? "Сега" : "Now",
                                   nb, 9.0f, th.success, 0.5f);
            }

            px += pw + 8;
            if (px + pw > todayCard.x + todayCard.width - 20) break;  // overflow guard
        }
    }

    // ---- Weekly grid card ----
    Rectangle gridCard = { pad.x, todayCard.y + todayCard.height + 16,
                           pad.width, pad.height - todayCard.height - 60 };
    UI::DrawSoftShadow(gridCard, 0.04f, 14.0f, 0.05f);
    DrawRectangleRounded(gridCard, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(gridCard, 0.04f, 8, 1.0f, th.border);

    // Inner padded grid
    Rectangle grid = { gridCard.x + 16, gridCard.y + 14,
                       gridCard.width - 32, gridCard.height - 28 };
    const int    timeColW = 56;
    const int    days     = 5;
    const float  dayColW  = (grid.width - timeColW) / days;
    const int    startHour = 8;
    const int    endHour   = 18;
    const int    hours     = endHour - startHour;
    // Reserve top header row of fixed 32px height
    const float  headerH = 32.0f;
    const float  hourH   = (grid.height - headerH) / (float)hours;

    // Day headers
    for (int d = 0; d < days; ++d) {
        Rectangle h = { grid.x + timeColW + d * dayColW, grid.y, dayColW, headerH };
        bool isToday = (todayDow == d);
        if (isToday) {
            DrawRectangleRounded(
                { h.x + 4, h.y + 2, h.width - 8, h.height - 4 },
                0.30f, 6, ColorAlpha(th.primary, 0.10f));
        }
        UI::DrawTextCenter(Bld(), I18n::DayLong(d), h, 12.0f,
                           isToday ? th.primary : th.textPrimary, 0.5f);
    }

    // Horizontal alternating row tints + hour labels
    for (int hr = 0; hr < hours; ++hr) {
        float y = grid.y + headerH + hr * hourH;
        // Soft alternating background
        if (hr % 2 == 0) {
            Rectangle band = { grid.x + timeColW, y, grid.width - timeColW, hourH };
            DrawRectangleRec(band, ColorAlpha(th.surfaceMuted, 0.4f));
        }
        char tb[8]; std::snprintf(tb, sizeof(tb), "%02d:00", startHour + hr);
        UI::DrawTextRight(Fnt(), tb,
                          { grid.x, y - 6, (float)timeColW - 8, 14 },
                          10.0f, th.textMuted);
    }
    // Last hour line
    DrawLineEx({ grid.x + timeColW, grid.y + headerH + hours * hourH },
               { grid.x + grid.width, grid.y + headerH + hours * hourH },
               1.0f, ColorAlpha(th.border, 0.6f));
    // Vertical grid lines
    for (int d = 0; d <= days; ++d) {
        float x = grid.x + timeColW + d * dayColW;
        DrawLineEx({ x, grid.y + headerH }, { x, grid.y + headerH + hours * hourH },
                   1.0f, ColorAlpha(th.border, 0.5f));
    }
    // Top header divider
    DrawLineEx({ grid.x, grid.y + headerH }, { grid.x + grid.width, grid.y + headerH },
               1.0f, ColorAlpha(th.border, 0.6f));

    // Class blocks - clipped to inside the grid area below the header
    BeginScissorMode((int)(grid.x + timeColW),
                     (int)(grid.y + headerH),
                     (int)(grid.width - timeColW),
                     (int)(hours * hourH));
    for (const auto& e : schedule) {
        if (e.semester != u->currentSemester) continue;
        if (!e.program.empty() && e.program != u->className) continue;
        if (e.dayOfWeek < 0 || e.dayOfWeek >= days) continue;

        float startMin = (float)(e.startMinutes - startHour * 60);
        float endMin   = (float)(e.endMinutes   - startHour * 60);
        if (endMin <= 0 || startMin >= hours * 60) continue;
        if (startMin < 0) startMin = 0;
        if (endMin > hours * 60) endMin = hours * 60;

        Rectangle block = {
            grid.x + timeColW + e.dayOfWeek * dayColW + 4,
            grid.y + headerH + (startMin / 60.0f) * hourH,
            dayColW - 8,
            ((endMin - startMin) / 60.0f) * hourH
        };

        Subject* s = app.Data().FindSubject(e.subjectId);
        Color sc = s ? UI::SubjectColor(s->colorIndex) : th.primary;

        // Soft shadow under block for depth
        UI::DrawSoftShadow(block, 0.18f, 6.0f, 0.06f);
        DrawRectangleRounded(block, 0.18f, 6, ColorAlpha(sc, 0.18f));
        DrawRectangleRounded({ block.x, block.y, 4, block.height }, 1.0f, 4, sc);
        UI::StrokeRoundedRect(block, 0.18f, 6, 1.0f, ColorAlpha(sc, 0.55f));

        // Compact text inside block - fits even short blocks
        std::string codeText = s ? s->code : "?";
        UI::DrawTextLeft(Bld(), codeText.c_str(),
                         { block.x + 10, block.y + 6 }, 12.0f, th.textPrimary);
        std::string kt = kindLabel(e.kind);
        UI::DrawTextLeft(Fnt(), kt.c_str(),
                         { block.x + 10, block.y + 22 }, 10.0f, th.textSecondary);

        // Bottom-aligned time + room (only show if block tall enough)
        if (block.height > 56) {
            std::string tt = fmtTime(e.startMinutes) + " - " + fmtTime(e.endMinutes);
            UI::DrawTextLeft(Fnt(), tt.c_str(),
                             { block.x + 10, block.y + block.height - 30 }, 10.0f, th.textMuted);
            UI::DrawTextLeft(Fnt(), e.room.c_str(),
                             { block.x + 10, block.y + block.height - 16 }, 10.0f, th.textMuted);
        } else if (block.height > 36) {
            UI::DrawTextLeft(Fnt(), e.room.c_str(),
                             { block.x + 10, block.y + block.height - 14 }, 9.0f, th.textMuted);
        }
    }

    // ---- Current-time indicator line (only on weekdays during 8-18) ----
    if (todayDow >= 0 && todayDow < days) {
        int curMin = nowHr * 60 + nowMn - startHour * 60;
        if (curMin >= 0 && curMin < hours * 60) {
            float y = grid.y + headerH + (curMin / 60.0f) * hourH;
            float xStart = grid.x + timeColW + todayDow * dayColW;
            float xEnd   = xStart + dayColW;
            // Red dot at the left + line across today's column
            DrawCircleV({ xStart, y }, 5.0f, th.error);
            DrawLineEx({ xStart, y }, { xEnd, y }, 2.0f, th.error);
        }
    }
    EndScissorMode();
}

// ------------------------------------------------------------------------
//  TAB: Exam session (list on left + calendar/summary on right)
// ------------------------------------------------------------------------

void StudentDashboard::DrawTabExams(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();
    if (!u) return;

    Rectangle pad = { area.x + 24, area.y + 24, area.width - 48, area.height - 48 };

    UI::DrawTextLeft(Dis(), I18n::T("exam.title"),
                     { pad.x, pad.y }, 22.0f, th.textPrimary);

    // Two-column split: left = list (60%), right = calendar+summary (40%)
    float leftW  = pad.width * 0.62f - 8;
    float rightW = pad.width * 0.38f - 8;
    Rectangle leftCol  = { pad.x,                  pad.y + 44, leftW,  pad.height - 60 };
    Rectangle rightCol = { pad.x + leftW + 16,     pad.y + 44, rightW, pad.height - 60 };

    // ---- Filter pills (in the left column header) ----
    const char* fkeys[] = { "exam.upcoming", "exam.past", "exam.all" };
    float fx = leftCol.x;
    for (int i = 0; i < 3; ++i) {
        const char* lbl = I18n::T(fkeys[i]);
        Vector2 m = UI::MeasureText(Bld(), lbl, 12.0f);
        Rectangle b = { fx, pad.y + 8, m.x + 24, 28 };
        bool active = (examFilter == i);
        bool hov    = CheckCollisionPointRec(GetMousePosition(), b);
        Color bg = active ? th.primary : (hov ? th.surfaceAlt : th.surface);
        Color fg = active ? WHITE : th.textSecondary;
        DrawRectangleRounded(b, 0.5f, 6, bg);
        UI::StrokeRoundedRect(b, 0.5f, 6, 1.0f, active ? th.primary : th.border);
        UI::DrawTextCenter(Bld(), lbl, b, 12.0f, fg, 0.5f);
        if (hov) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) examFilter = i;
        }
        fx += b.width + 8;
    }

    // ---- Build filtered + sorted list ----
    int todayY, todayM, todayD; Models::TodayYMD(todayY, todayM, todayD);
    auto& exams = app.Data().Exams();
    std::vector<const ExamSession*> list;
    for (const auto& e : exams) {
        if (!e.program.empty() && e.program != u->className) continue;
        int diff = Models::DaysBetween(todayY, todayM, todayD, e.year, e.month, e.day);
        if (examFilter == 0 && diff < 0) continue;
        if (examFilter == 1 && diff >= 0) continue;
        list.push_back(&e);
    }
    std::sort(list.begin(), list.end(), [](const ExamSession* a, const ExamSession* b){
        long ja = jdn(a->year, a->month, a->day);
        long jb = jdn(b->year, b->month, b->day);
        if (ja != jb) return ja < jb;
        return a->startMinutes < b->startMinutes;
    });

    // ---- Left: scrollable list of exam cards ----
    UI::DrawSoftShadow(leftCol, 0.04f, 14.0f, 0.05f);
    DrawRectangleRounded(leftCol, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(leftCol, 0.04f, 8, 1.0f, th.border);

    Rectangle scroll = { leftCol.x + 12, leftCol.y + 12,
                         leftCol.width - 24, leftCol.height - 24 };
    BeginScissorMode((int)scroll.x, (int)scroll.y, (int)scroll.width, (int)scroll.height);
    float py = scroll.y - scrollY[(int)StudentTab::EXAMS];

    if (list.empty()) {
        UI::IconCalendar({ scroll.x + scroll.width / 2, scroll.y + scroll.height / 2 - 18 },
                         18.0f, th.textMuted, 1.5f);
        UI::DrawTextCenter(Fnt(), I18n::T("exam.no_exams"),
                           { scroll.x, scroll.y + scroll.height / 2 + 4, scroll.width, 24 },
                           14.0f, th.textMuted, 0.5f);
    }

    for (const ExamSession* e : list) {
        Rectangle row = { scroll.x, py, scroll.width, 80 };
        DrawRectangleRounded(row, 0.06f, 6, th.surfaceMuted);

        Subject* s = app.Data().FindSubject(e->subjectId);
        Color sc = s ? UI::SubjectColor(s->colorIndex) : th.primary;
        DrawRectangleRounded({ row.x, row.y, 4, row.height }, 1.0f, 4, sc);

        // Date pill (left)
        Rectangle dp = { row.x + 16, row.y + 14, 60, 52 };
        int diff = Models::DaysBetween(todayY, todayM, todayD, e->year, e->month, e->day);
        Color dpFg = (diff < 0)
            ? th.textMuted
            : (diff <= 3 ? th.error : (diff <= 7 ? th.warning : th.primary));
        DrawRectangleRounded(dp, 0.18f, 6, ColorAlpha(dpFg, 0.10f));
        char db[8]; std::snprintf(db, sizeof(db), "%02d", e->day);
        UI::DrawTextCenter(Dis(), db,
                           { dp.x, dp.y + 4, dp.width, 24 }, 22.0f, dpFg, 0.5f);
        UI::DrawTextCenter(Bld(), I18n::MonthShort(e->month),
                           { dp.x, dp.y + 30, dp.width, 16 }, 11.0f, dpFg, 0.5f);

        // Subject and lecturer
        UI::DrawTextLeft(Bld(), s ? s->code.c_str() : "—",
                         { row.x + 88, row.y + 12 }, 12.0f, th.textMuted);
        UI::DrawTextLeft(Bld(), s ? s->name.c_str() : "—",
                         { row.x + 88, row.y + 28 }, 13.0f, th.textPrimary);
        UI::DrawTextLeft(Fnt(), e->lecturer.c_str(),
                         { row.x + 88, row.y + 50 }, 11.0f, th.textMuted);

        // Time + room (right) with clock icon
        UI::IconClock({ row.x + row.width - 144, row.y + 22 }, 7.0f, th.textPrimary, 1.5f);
        std::string when = fmtTime(e->startMinutes) + " · " + e->room;
        UI::DrawTextLeft(Fnt(), when.c_str(),
                         { row.x + row.width - 132, row.y + 16 }, 12.0f, th.textPrimary);

        // Type pill
        const char* tlbl = (e->type == ExamSessionType::REGULAR)
            ? I18n::T("exam.regular")
            : I18n::T("exam.makeup");
        Color tc = (e->type == ExamSessionType::REGULAR) ? th.primary : th.warning;
        Vector2 tm = UI::MeasureText(Bld(), tlbl, 10.0f);
        Rectangle tb = { row.x + row.width - tm.x - 28, row.y + 38, tm.x + 16, 20 };
        DrawRectangleRounded(tb, 0.5f, 6, ColorAlpha(tc, 0.15f));
        UI::DrawTextCenter(Bld(), tlbl, tb, 10.0f, tc, 0.5f);

        // Days-until below pill
        char ub[64];
        if (diff > 1)       std::snprintf(ub, sizeof(ub), "%s %d %s",
                                         I18n::T("exam.days_until"), diff, I18n::T("exam.days"));
        else if (diff == 1) std::snprintf(ub, sizeof(ub), "%s", I18n::T("exam.tomorrow"));
        else if (diff == 0) std::snprintf(ub, sizeof(ub), "%s", I18n::T("exam.today"));
        else                std::snprintf(ub, sizeof(ub), "—");
        Color uc = (diff < 0) ? th.textMuted
                              : (diff <= 3 ? th.error : (diff <= 7 ? th.warning : th.textSecondary));
        UI::DrawTextRight(Bld(), ub,
                          { row.x, row.y + 62, row.width - 12, 14 }, 11.0f, uc);

        py += 88;
    }
    float used = py + scrollY[(int)StudentTab::EXAMS] - scroll.y;
    if (used < scroll.height) scrollTarget[(int)StudentTab::EXAMS] = 0;
    else if (scrollTarget[(int)StudentTab::EXAMS] > used - scroll.height)
        scrollTarget[(int)StudentTab::EXAMS] = used - scroll.height;
    EndScissorMode();

    // ---- Right: Calendar widget + Stats summary ----
    Rectangle calCard = { rightCol.x, rightCol.y, rightCol.width, 280 };
    UI::DrawSoftShadow(calCard, 0.04f, 14.0f, 0.05f);
    DrawRectangleRounded(calCard, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(calCard, 0.04f, 8, 1.0f, th.border);

    // Calendar marks: every exam in the user's program
    std::vector<UI::CalendarMark> marks;
    for (const auto& e : exams) {
        if (!e.program.empty() && e.program != u->className) continue;
        int diff = Models::DaysBetween(todayY, todayM, todayD, e.year, e.month, e.day);
        Color c = (diff < 0) ? th.textMuted
                             : (diff <= 3 ? th.error : (diff <= 7 ? th.warning : th.primary));
        marks.push_back({ e.year, e.month, e.day, c });
    }
    UI::DrawMiniCalendar({ calCard.x + 12, calCard.y + 8, calCard.width - 24, calCard.height - 16 },
                        todayY, todayM, todayY, todayM, todayD, marks);

    // Summary card below
    Rectangle sumCard = { rightCol.x, calCard.y + calCard.height + 12,
                          rightCol.width, rightCol.height - calCard.height - 12 };
    UI::DrawSoftShadow(sumCard, 0.04f, 14.0f, 0.05f);
    DrawRectangleRounded(sumCard, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(sumCard, 0.04f, 8, 1.0f, th.border);

    UI::DrawTextLeft(Bld(),
                     I18n::Current() == Lang::BG ? "Резюме на сесията" : "Session summary",
                     { sumCard.x + 16, sumCard.y + 14 }, 13.0f, th.textPrimary);

    // Count upcoming, regular, makeup
    int upcoming = 0, regular = 0, makeup = 0;
    int nextDays = -1;
    for (const auto& e : exams) {
        if (!e.program.empty() && e.program != u->className) continue;
        int diff = Models::DaysBetween(todayY, todayM, todayD, e.year, e.month, e.day);
        if (diff < 0) continue;
        upcoming++;
        if (e.type == ExamSessionType::REGULAR) regular++; else makeup++;
        if (nextDays < 0 || diff < nextDays) nextDays = diff;
    }

    auto statRow = [&](float y, const char* lbl, const char* val, Color valColor){
        UI::DrawTextLeft(Fnt(), lbl, { sumCard.x + 16, y }, 11.0f, th.textMuted);
        UI::DrawTextRight(Bld(), val,
                          { sumCard.x, y - 1, sumCard.width - 16, 14 }, 13.0f, valColor);
    };
    char b1[16], b2[16], b3[16];
    std::snprintf(b1, sizeof(b1), "%d", upcoming);
    std::snprintf(b2, sizeof(b2), "%d", regular);
    std::snprintf(b3, sizeof(b3), "%d", makeup);
    statRow(sumCard.y + 44, I18n::Current() == Lang::BG ? "Предстоящи" : "Upcoming", b1, th.primary);
    statRow(sumCard.y + 68, I18n::Current() == Lang::BG ? "Редовни"    : "Regular",  b2, th.textPrimary);
    statRow(sumCard.y + 92, I18n::Current() == Lang::BG ? "Поправителни" : "Makeup", b3, th.warning);

    if (nextDays >= 0) {
        Rectangle hb = { sumCard.x + 12, sumCard.y + sumCard.height - 56,
                         sumCard.width - 24, 44 };
        Color hbg = (nextDays <= 3) ? th.error
                  : (nextDays <= 7 ? th.warning : th.primary);
        DrawRectangleRounded(hb, 0.30f, 6, ColorAlpha(hbg, 0.12f));
        UI::IconClock({ hb.x + 18, hb.y + hb.height / 2 }, 9.0f, hbg, 1.8f);
        const char* lbl = (nextDays == 0)
            ? (I18n::Current() == Lang::BG ? "Изпит днес" : "Exam today")
            : (nextDays == 1
                ? (I18n::Current() == Lang::BG ? "Изпит утре" : "Exam tomorrow")
                : (I18n::Current() == Lang::BG ? "До следващ изпит" : "Until next exam"));
        UI::DrawTextLeft(Fnt(), lbl,
                         { hb.x + 36, hb.y + 8 }, 11.0f, th.textSecondary);
        char nb[24];
        if (nextDays > 1) std::snprintf(nb, sizeof(nb), "%d %s", nextDays, I18n::T("exam.days"));
        else              std::snprintf(nb, sizeof(nb), "—");
        UI::DrawTextLeft(Bld(), nb,
                         { hb.x + 36, hb.y + 22 }, 14.0f, hbg);
    }
}

// ------------------------------------------------------------------------
//  TAB: Documents
// ------------------------------------------------------------------------

void StudentDashboard::DrawTabDocuments(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();
    if (!u) return;

    Rectangle pad = { area.x + 24, area.y + 24, area.width - 48, area.height - 48 };

    // ---- Form card ----
    Rectangle form = { pad.x, pad.y, pad.width, 240 };
    UI::DrawSoftShadow(form, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(form, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(form, 0.04f, 8, 1.0f, th.border);

    // Heading with document icon
    UI::IconDocument({ form.x + 26, form.y + 28 }, 11.0f, th.primary, 1.8f);
    UI::DrawTextLeft(Bld(), I18n::T("doc.request"),
                     { form.x + 46, form.y + 16 }, 16.0f, th.textPrimary);
    UI::DrawTextLeft(Fnt(), I18n::T("doc.type"),
                     { form.x + 20, form.y + 50 }, 11.0f, th.textMuted);

    // Doc-kind pills with checkmark when selected
    const DocKind kinds[] = { DocKind::TRANSCRIPT, DocKind::ENROLLMENT,
                              DocKind::SEMESTER_CERT, DocKind::DIPLOMA_DUP };
    float kx = form.x + 20;
    for (DocKind k : kinds) {
        const char* lbl = docKindLabel(k);
        Vector2 m = UI::MeasureText(Bld(), lbl, 12.0f);
        bool active = (selectedDocKind == k);
        // Add space for the check mark when active
        Rectangle b = { kx, form.y + 70, m.x + (active ? 38 : 24), 32 };
        bool hov = CheckCollisionPointRec(GetMousePosition(), b);
        Color bg = active ? th.primarySoft : (hov ? th.surfaceAlt : th.surface);
        Color fg = active ? th.primary : th.textSecondary;
        DrawRectangleRounded(b, 0.5f, 6, bg);
        UI::StrokeRoundedRect(b, 0.5f, 6, 1.0f, active ? th.primary : th.border);
        if (active) {
            UI::IconCheck({ b.x + 14, b.y + b.height * 0.5f }, 6.0f, th.primary, 1.8f);
            UI::DrawTextLeft(Bld(), lbl,
                             { b.x + 26, b.y + b.height * 0.5f - 7 }, 12.0f, fg);
        } else {
            UI::DrawTextCenter(Bld(), lbl, b, 12.0f, fg, 0.5f);
        }
        if (hov) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) selectedDocKind = k;
        }
        kx += b.width + 8;
    }

    // Purpose textarea
    UI::DrawTextLeft(Fnt(), I18n::T("doc.purpose"),
                     { form.x + 20, form.y + 116 }, 11.0f, th.textMuted);
    docPurpose.bounds = { form.x + 20, form.y + 134, form.width - 280, 44 };
    docPurpose.Draw();

    // Submit button
    submitDocBtn.bounds = { form.x + form.width - 240, form.y + 134, 220, 44 };
    submitDocBtn.Draw();

    // Estimated processing time hint
    UI::DrawTextLeft(Fnt(),
                     I18n::Current() == Lang::BG
                         ? "Обработката отнема обикновено 3–5 работни дни."
                         : "Processing typically takes 3–5 working days.",
                     { form.x + 20, form.y + 188 }, 11.0f, th.textMuted);

    // ---- Past requests list ----
    Rectangle list = { pad.x, form.y + form.height + 16,
                       pad.width, pad.height - form.height - 16 };
    UI::DrawSoftShadow(list, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(list, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(list, 0.04f, 8, 1.0f, th.border);
    UI::DrawTextLeft(Bld(), I18n::Current() == Lang::BG ? "Минали заявки" : "Past requests",
                     { list.x + 20, list.y + 16 }, 14.0f, th.textPrimary);

    auto& reqs = app.Data().DocumentRequests();
    std::vector<const DocumentRequest*> mine;
    for (const auto& d : reqs) if (d.userId == u->id) mine.push_back(&d);
    std::sort(mine.begin(), mine.end(),
              [](const DocumentRequest* a, const DocumentRequest* b){
                  return a->submittedAt > b->submittedAt;
              });

    // Right-side count chip
    char cb[16];
    std::snprintf(cb, sizeof(cb), "%d", (int)mine.size());
    Rectangle countChip = { list.x + list.width - 60, list.y + 16, 40, 22 };
    DrawRectangleRounded(countChip, 0.5f, 6, th.surfaceMuted);
    UI::DrawTextCenter(Bld(), cb, countChip, 11.0f, th.textSecondary, 0.5f);

    Rectangle scroll = { list.x + 12, list.y + 50,
                         list.width - 24, list.height - 64 };
    BeginScissorMode((int)scroll.x, (int)scroll.y, (int)scroll.width, (int)scroll.height);
    float py = scroll.y - scrollY[(int)StudentTab::DOCUMENTS];

    if (mine.empty()) {
        UI::IconDocument({ scroll.x + scroll.width / 2, scroll.y + scroll.height / 2 - 18 },
                         16.0f, th.textMuted, 1.5f);
        UI::DrawTextCenter(Fnt(), I18n::T("doc.no_requests"),
                           { scroll.x, scroll.y + scroll.height / 2 + 4, scroll.width, 24 },
                           14.0f, th.textMuted, 0.5f);
    }

    for (const DocumentRequest* d : mine) {
        Rectangle row = { scroll.x + 4, py, scroll.width - 8, 64 };
        bool hov = CheckCollisionPointRec(mouse, row);
        DrawRectangleRounded(row, 0.10f, 6, hov ? th.surfaceAlt : th.surfaceMuted);

        // Status color stripe
        Color sc = docStatusColor(d->status);
        DrawRectangleRounded({ row.x, row.y, 4, row.height }, 1.0f, 4, sc);

        UI::DrawTextLeft(Bld(), docKindLabel(d->kind),
                         { row.x + 18, row.y + 10 }, 13.0f, th.textPrimary);
        UI::DrawTextLeft(Fnt(),
                         (std::string(I18n::T("doc.requested_on")) + "  " +
                          Models::RelativeTime(d->submittedAt)).c_str(),
                         { row.x + 18, row.y + 30 }, 11.0f, th.textMuted);
        // Truncate purpose
        std::string purp = UI::Ellipsize(Fnt(), d->purpose, row.width - 220, 11.0f, 1.0f);
        UI::DrawTextLeft(Fnt(), purp.c_str(),
                         { row.x + 18, row.y + 46 }, 11.0f, th.textSecondary);

        // Status pill (right)
        const char* slbl = docStatusLabel(d->status);
        Vector2 m = UI::MeasureText(Bld(), slbl, 11.0f);
        Rectangle pill = { row.x + row.width - m.x - 30, row.y + 18, m.x + 22, 26 };
        DrawRectangleRounded(pill, 0.5f, 6, ColorAlpha(sc, 0.18f));
        UI::DrawTextCenter(Bld(), slbl, pill, 11.0f, sc, 0.5f);

        py += 72;
    }
    float used = py + scrollY[(int)StudentTab::DOCUMENTS] - scroll.y;
    if (used < scroll.height) scrollTarget[(int)StudentTab::DOCUMENTS] = 0;
    else if (scrollTarget[(int)StudentTab::DOCUMENTS] > used - scroll.height)
        scrollTarget[(int)StudentTab::DOCUMENTS] = used - scroll.height;
    EndScissorMode();
}

// ------------------------------------------------------------------------
//  TAB: Profile
// ------------------------------------------------------------------------

void StudentDashboard::DrawTabProfile(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    const Theme& th = ThemeMgr::Current();
    User* u = app.CurrentUser();
    if (!u) return;

    Rectangle padR = { area.x + 24, area.y + 24, area.width - 48, area.height - 48 };

    // Personal info card with avatar
    Rectangle pi = { padR.x, padR.y, padR.width / 2 - 12, 320 };
    UI::DrawSoftShadow(pi, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(pi, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(pi, 0.04f, 8, 1.0f, th.border);
    UI::DrawTextLeft(Bld(), I18n::T("profile.personal"),
                     { pi.x + 22, pi.y + 18 }, 16.0f, th.textPrimary);

    // Avatar circle: derive 2-letter initials and a color from name hash
    {
        Vector2 ac = { pi.x + 60, pi.y + 88 };
        unsigned int h = 0;
        for (char c : u->fullName) h = h * 131 + (unsigned char)c;
        Color av = UI::SubjectColor((int)(h % UI::SubjectColorCount()));
        DrawCircleV(ac, 32.0f, ColorAlpha(av, 0.18f));
        DrawCircleV(ac, 28.0f, av);

        // Initials (first letter of first + last name token)
        std::string init;
        bool prevSpace = true;
        for (char c : u->fullName) {
            if (c == ' ' || c == '\t') { prevSpace = true; continue; }
            if (prevSpace && (int)init.size() < 4) {
                init.push_back(c);
                prevSpace = false;
            } else if (init.size() > 0 && ((unsigned char)c & 0xC0) == 0x80) {
                // continuation byte from a multi-byte UTF-8 sequence
                init.push_back(c);
            }
        }
        // Trim to first 2 user-perceived chars (rough — ok for Cyrillic & Latin)
        std::string firstChar, secondChar;
        size_t i = 0;
        while (i < init.size() && firstChar.empty()) {
            unsigned char c = (unsigned char)init[i];
            if ((c & 0x80) == 0) { firstChar = init.substr(i, 1); i += 1; }
            else if ((c & 0xE0) == 0xC0) { firstChar = init.substr(i, 2); i += 2; }
            else if ((c & 0xF0) == 0xE0) { firstChar = init.substr(i, 3); i += 3; }
            else { firstChar = init.substr(i, 4); i += 4; }
        }
        while (i < init.size() && secondChar.empty()) {
            unsigned char c = (unsigned char)init[i];
            if ((c & 0x80) == 0) { secondChar = init.substr(i, 1); i += 1; }
            else if ((c & 0xE0) == 0xC0) { secondChar = init.substr(i, 2); i += 2; }
            else if ((c & 0xF0) == 0xE0) { secondChar = init.substr(i, 3); i += 3; }
            else { secondChar = init.substr(i, 4); i += 4; }
        }
        std::string both = firstChar + secondChar;
        Vector2 tm = ::MeasureTextEx(Bld(), both.c_str(), 24.0f, 1.0f);
        DrawTextEx(Bld(), both.c_str(),
                   { ac.x - tm.x * 0.5f, ac.y - tm.y * 0.5f }, 24.0f, 1.0f, WHITE);
    }
    UI::DrawTextLeft(Bld(), u->fullName.c_str(),
                     { pi.x + 110, pi.y + 70 }, 20.0f, th.textPrimary);
    UI::DrawTextLeft(Bld(), u->facultyNumber.c_str(),
                     { pi.x + 110, pi.y + 100 }, 16.0f, th.primary);

    auto kvRow = [&](Rectangle parent, float y, const char* k, const char* v){
        UI::DrawTextLeft(Fnt(), k, { parent.x + 22, parent.y + y }, 12.0f, th.textMuted);
        UI::DrawTextLeft(Bld(), v ? v : "—", { parent.x + 22, parent.y + y + 18 }, 15.0f, th.textPrimary);
    };
    kvRow(pi, 160, I18n::T("profile.username"),    u->username.c_str());
    kvRow(pi, 220, I18n::T("student.faculty"),     u->faculty.c_str());

    // Academic info card with university crest
    Rectangle ai = { padR.x + padR.width / 2 + 12, padR.y, padR.width / 2 - 12, 320 };
    UI::DrawSoftShadow(ai, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(ai, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(ai, 0.04f, 8, 1.0f, th.border);
    UI::DrawTextLeft(Bld(), I18n::T("profile.academic"),
                     { ai.x + 22, ai.y + 18 }, 16.0f, th.textPrimary);

    // Crest at top right of card
    UI::DrawUniversityCrest({ ai.x + ai.width - 50, ai.y + 50 }, 22.0f, th.primary, WHITE);

    char yb[8], sb[8];
    std::snprintf(yb, sizeof(yb), "%d", u->year);
    std::snprintf(sb, sizeof(sb), "%d", u->currentSemester);
    kvRow(ai,  56, I18n::T("student.program"),  u->className.c_str());
    kvRow(ai, 116, I18n::T("student.year"),     yb);
    kvRow(ai, 176, I18n::T("student.semester"), sb);

    // Academic year (top right of card)
    int nowY, nowM, nowD; Models::TodayYMD(nowY, nowM, nowD);
    std::string ay = Models::AcademicYear(nowY, nowM);
    UI::DrawTextLeft(Fnt(),
                     I18n::Current() == Lang::BG ? "Учебна година" : "Academic year",
                     { ai.x + 220, ai.y + 56 }, 12.0f, th.textMuted);
    UI::DrawTextLeft(Bld(), ay.c_str(),
                     { ai.x + 220, ai.y + 76 }, 15.0f, th.textPrimary);

    // Status pill (bottom)
    UI::DrawTextLeft(Fnt(), I18n::T("student.status"),
                     { ai.x + 22, ai.y + 220 }, 12.0f, th.textMuted);
    Rectangle sp = { ai.x + 22, ai.y + 244, 110, 32 };
    DrawRectangleRounded(sp, 0.5f, 6, ColorAlpha(th.success, 0.15f));
    DrawCircleV({ sp.x + 14, sp.y + sp.height * 0.5f }, 5.0f, th.success);
    UI::DrawTextLeft(Bld(), I18n::T("student.status.active"),
                     { sp.x + 26, sp.y + 9 }, 13.0f, th.success);

    // Preferences card
    Rectangle pr = { padR.x, pi.y + pi.height + 16, padR.width, 200 };
    UI::DrawSoftShadow(pr, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(pr, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(pr, 0.04f, 8, 1.0f, th.border);
    UI::DrawTextLeft(Bld(), I18n::T("profile.preferences"),
                     { pr.x + 22, pr.y + 18 }, 16.0f, th.textPrimary);

    // Language segmented control
    UI::DrawTextLeft(Fnt(), I18n::T("profile.language"),
                     { pr.x + 22, pr.y + 58 }, 12.0f, th.textMuted);
    Rectangle lg = { pr.x + 22, pr.y + 78, 220, 40 };
    DrawRectangleRounded(lg, 0.5f, 6, th.surfaceMuted);
    Rectangle lgBG = { lg.x + 4, lg.y + 4, lg.width / 2 - 6, lg.height - 8 };
    Rectangle lgEN = { lg.x + lg.width / 2 + 2, lg.y + 4, lg.width / 2 - 6, lg.height - 8 };
    Rectangle lgActive = (profileLangChoice == 0) ? lgBG : lgEN;
    DrawRectangleRounded(lgActive, 0.5f, 6, th.surface);
    UI::StrokeRoundedRect(lgActive, 0.5f, 6, 1.0f, th.primary);
    UI::DrawTextCenter(Bld(), "Български", lgBG, 13.0f,
                       profileLangChoice == 0 ? th.primary : th.textSecondary, 0.5f);
    UI::DrawTextCenter(Bld(), "English",   lgEN, 13.0f,
                       profileLangChoice == 1 ? th.primary : th.textSecondary, 0.5f);
    if (CheckCollisionPointRec(GetMousePosition(), lgBG)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            profileLangChoice = 0; I18n::SetLang(Lang::BG);
        }
    }
    if (CheckCollisionPointRec(GetMousePosition(), lgEN)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            profileLangChoice = 1; I18n::SetLang(Lang::EN);
        }
    }

    // Theme segmented control
    UI::DrawTextLeft(Fnt(), I18n::T("profile.theme"),
                     { pr.x + 280, pr.y + 58 }, 12.0f, th.textMuted);
    Rectangle tg = { pr.x + 280, pr.y + 78, 220, 40 };
    DrawRectangleRounded(tg, 0.5f, 6, th.surfaceMuted);
    Rectangle tgL = { tg.x + 4, tg.y + 4, tg.width / 2 - 6, tg.height - 8 };
    Rectangle tgD = { tg.x + tg.width / 2 + 2, tg.y + 4, tg.width / 2 - 6, tg.height - 8 };
    Rectangle tgActive = (profileThemeChoice == 0) ? tgL : tgD;
    DrawRectangleRounded(tgActive, 0.5f, 6, th.surface);
    UI::StrokeRoundedRect(tgActive, 0.5f, 6, 1.0f, th.primary);
    UI::DrawTextCenter(Bld(), I18n::T("common.theme.light"), tgL, 13.0f,
                       profileThemeChoice == 0 ? th.primary : th.textSecondary, 0.5f);
    UI::DrawTextCenter(Bld(), I18n::T("common.theme.dark"), tgD, 13.0f,
                       profileThemeChoice == 1 ? th.primary : th.textSecondary, 0.5f);
    if (CheckCollisionPointRec(GetMousePosition(), tgL)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            profileThemeChoice = 0;
            ThemeMgr::SetMode(ThemeMode::LIGHT);
            u->darkMode = false;
            app.Data().UpdateUser(*u);
        }
    }
    if (CheckCollisionPointRec(GetMousePosition(), tgD)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            profileThemeChoice = 1;
            ThemeMgr::SetMode(ThemeMode::DARK);
            u->darkMode = true;
            app.Data().UpdateUser(*u);
        }
    }

    // Export Transcript button (bottom-left)
    {
        Rectangle eb = { pr.x + 20, pr.y + pr.height - 60, 220, 44 };
        bool hov = CheckCollisionPointRec(GetMousePosition(), eb);
        Color bg = hov ? th.primarySoft : ColorAlpha(th.primary, 0.08f);
        DrawRectangleRounded(eb, 0.5f, 6, bg);
        UI::StrokeRoundedRect(eb, 0.5f, 6, 1.0f, ColorAlpha(th.primary, 0.4f));
        UI::IconDocument({ eb.x + 18, eb.y + eb.height * 0.5f }, 9.0f, th.primary, 1.6f);
        UI::DrawTextLeft(Bld(),
                         I18n::Current() == Lang::BG ? "Експорт на справка" : "Export transcript",
                         { eb.x + 36, eb.y + 8 }, 13.0f, th.primary);
        UI::DrawTextLeft(Fnt(), "Ctrl+E",
                         { eb.x + 36, eb.y + 26 }, 10.0f, th.textMuted);
        if (hov) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                std::string p = ExportTranscript(app.Data(), *u);
                if (!p.empty()) {
                    std::string msg = (I18n::Current() == Lang::BG
                        ? "Справката е експортирана: "
                        : "Transcript exported to: ") + p;
                    app.Toast(msg, ThemeMgr::Current().success);
                } else {
                    app.Toast(I18n::Current() == Lang::BG
                                  ? "Грешка при експорт"
                                  : "Export failed",
                              ThemeMgr::Current().error);
                }
            }
        }
    }

    // Logout button (bottom-right of preferences card)
    profileLogoutBtn.label = I18n::T("common.logout");
    profileLogoutBtn.bounds = { pr.x + pr.width - 180, pr.y + pr.height - 60, 160, 44 };
    profileLogoutBtn.Draw();
}

// ------------------------------------------------------------------------
//  Master Draw
// ------------------------------------------------------------------------

void StudentDashboard::Draw(App& app) {
    int sw = app.W(), sh = app.H();
    const float topH = 96.0f;
    const float tabH = 56.0f;

    // 1. Solid header background spanning the top of the window
    DrawRectangle(0, 0, sw, (int)topH, ThemeMgr::Current().surface);
    DrawRectangle(0, (int)topH, sw, 1, ThemeMgr::Current().border);

    DrawTopBar(app, { 0, 0, (float)sw, topH });
    DrawTabStrip(app, { 0, topH, (float)sw, tabH }, GetFrameTime(), GetMousePosition());

    Rectangle area = { 0, topH + tabH, (float)sw, (float)sh - topH - tabH };

    // Animated entry overlay using tabSwitchT
    float t = Easing::OutCubic(Easing::Clamp01(tabSwitchT));
    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    {
        float dx = (1.0f - t) * 12.0f;
        Rectangle inner = { area.x + dx, area.y, area.width - dx, area.height };
        switch (activeTab) {
            case StudentTab::GRADES:     DrawTabGrades(app,     inner, GetFrameTime(), GetMousePosition()); break;
            case StudentTab::CURRICULUM: DrawTabCurriculum(app, inner, GetFrameTime(), GetMousePosition()); break;
            case StudentTab::SCHEDULE:   DrawTabSchedule(app,   inner, GetFrameTime(), GetMousePosition()); break;
            case StudentTab::EXAMS:      DrawTabExams(app,      inner, GetFrameTime(), GetMousePosition()); break;
            case StudentTab::DOCUMENTS:  DrawTabDocuments(app,  inner, GetFrameTime(), GetMousePosition()); break;
            case StudentTab::PROFILE:    DrawTabProfile(app,    inner, GetFrameTime(), GetMousePosition()); break;
        }
    }
    EndScissorMode();

    // ---- Help hint button (bottom-right) ----
    {
        const Theme& th = ThemeMgr::Current();
        Rectangle hb = { (float)sw - 56, (float)sh - 56, 36, 36 };
        bool hov = CheckCollisionPointRec(GetMousePosition(), hb);
        Color bg = hov ? th.primary : th.surface;
        Color fg = hov ? WHITE : th.textSecondary;
        UI::DrawSoftShadow(hb, 1.0f, 14.0f, 0.10f);
        DrawCircleV({ hb.x + hb.width / 2, hb.y + hb.height / 2 }, hb.width / 2, bg);
        UI::StrokeRoundedRect(hb, 1.0f, 12, 1.0f, th.border);
        Vector2 m = ::MeasureTextEx(Bld(), "?", 20.0f, 1.0f);
        DrawTextEx(Bld(), "?",
                   { hb.x + hb.width * 0.5f - m.x * 0.5f,
                     hb.y + hb.height * 0.5f - m.y * 0.5f },
                   20.0f, 1.0f, fg);
        if (hov) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) showHelp = !showHelp;
        }
    }

    // ---- Keyboard shortcut overlay ----
    if (helpT > 0.001f) {
        const Theme& th = ThemeMgr::Current();
        // Dim background
        DrawRectangle(0, 0, sw, sh, ColorAlpha(BLACK, 0.35f * helpT));

        // Card (centered)
        float cardW = 480, cardH = 380;
        Rectangle card = {
            (float)sw / 2 - cardW / 2,
            (float)sh / 2 - cardH / 2 + (1.0f - helpT) * 12.0f,
            cardW, cardH
        };
        UI::DrawSoftShadow(card, 0.06f, 30.0f, 0.18f * helpT);
        Color bg = th.surface; bg.a = (unsigned char)(255 * helpT);
        DrawRectangleRounded(card, 0.06f, 8, bg);
        UI::StrokeRoundedRect(card, 0.06f, 8, 1.0f, ColorAlpha(th.border, helpT));

        UI::DrawTextLeft(Dis(),
                         I18n::Current() == Lang::BG ? "Бързи клавиши" : "Keyboard shortcuts",
                         { card.x + 28, card.y + 24 }, 22.0f,
                         ColorAlpha(th.textPrimary, helpT));

        struct KB { const char* key; const char* descBG; const char* descEN; };
        const KB shortcuts[] = {
            { "1",       "Оценки",                "Grades" },
            { "2",       "Учебен план",            "Curriculum" },
            { "3",       "График",                "Schedule" },
            { "4",       "Изпитна сесия",          "Exam session" },
            { "5",       "Документи",              "Documents" },
            { "6",       "Профил",                 "Profile" },
            { "Ctrl+E",  "Експорт на справка",     "Export transcript" },
            { "F1",      "Показва тази помощ",     "Show this help" },
            { "Esc",     "Затвори",                "Close modal" },
        };
        float py = card.y + 70;
        bool en = (I18n::Current() == Lang::EN);
        for (const KB& kb : shortcuts) {
            // Key chip
            Vector2 km = ::MeasureTextEx(Bld(), kb.key, 12.0f, 1.0f);
            Rectangle chip = { card.x + 28, py, km.x + 20, 26 };
            DrawRectangleRounded(chip, 0.30f, 4, ColorAlpha(th.surfaceMuted, helpT));
            UI::StrokeRoundedRect(chip, 0.30f, 4, 1.0f, ColorAlpha(th.border, helpT));
            DrawTextEx(Bld(), kb.key,
                       { chip.x + chip.width * 0.5f - km.x * 0.5f, chip.y + 6 },
                       12.0f, 1.0f, ColorAlpha(th.textPrimary, helpT));
            // Description
            DrawTextEx(Fnt(), en ? kb.descEN : kb.descBG,
                       { card.x + 130, py + 6 }, 13.0f, 1.0f,
                       ColorAlpha(th.textSecondary, helpT));
            py += 32;
        }

        UI::DrawTextRight(Fnt(),
                          I18n::Current() == Lang::BG ? "F1 за затваряне" : "F1 to close",
                          { card.x, card.y + card.height - 24, card.width - 28, 14 },
                          11.0f, ColorAlpha(th.textMuted, helpT));
    }
}

// ============================================================================
//  StudentDetailScreen (teacher drilling into one student)
//
//  Shows the chosen student's full record - subjects, grades, trend chart,
//  insights, and an Add-Grade modal. This is the teacher workflow only.
// ============================================================================

void StudentDetailScreen::OnEnter(App& app) {
    (void)app;
    backBtn.kind = UI::Button::GHOST;
    backBtn.label = I18n::Current() == Lang::BG ? "Назад" : "Back";

    addGradeBtn.kind = UI::Button::PRIMARY;
    addGradeBtn.label = I18n::T("teacher.add_grade");

    overallCircle.colorByValue = true;
    overallCircle.thickness = 10.0f;

    addGradeModal.bounds = {0, 0, 480, 460};
    confirmDeleteModal.bounds = {0, 0, 420, 220};

    agComment.placeholder = I18n::Current() == Lang::BG
        ? "Бележка (по желание)"
        : "Comment (optional)";
    agSave.kind   = UI::Button::PRIMARY;
    agCancel.kind = UI::Button::GHOST;
    cdDelete.kind = UI::Button::DANGER;
    cdCancel.kind = UI::Button::GHOST;

    trendChart.title = I18n::Current() == Lang::BG ? "Развитие на оценките" : "Grade trend";
    trendChart.minY = 2.0f; trendChart.maxY = 6.0f;
}

void StudentDetailScreen::OnShow(App& app) {
    enterT = 0.0f;
    scrollY = scrollTarget = 0.0f;
    User* st = app.Data().FindUserById(app.SelectedStudent());
    if (st) {
        float avg = Models::StudentOverallAverage(st->id, app.Data().Grades());
        if (avg < 0) avg = 0;
        overallCircle.SetTarget(avg / 6.0f);

        // Build trend chart
        trendChart.values.clear();
        trendChart.xLabels.clear();
        std::vector<const Grade*> gs;
        for (const auto& g : app.Data().Grades()) if (g.studentId == st->id) gs.push_back(&g);
        std::sort(gs.begin(), gs.end(),
                  [](const Grade* a, const Grade* b){ return a->createdAt < b->createdAt; });
        for (const Grade* g : gs) {
            trendChart.values.push_back((float)g->value);
            char b[8]; std::snprintf(b, sizeof(b), "%02d/%02d", g->month, g->day);
            trendChart.xLabels.push_back(b);
        }
        trendChart.OnShow();
    }
}

std::vector<std::string> StudentDetailScreen::BuildInsights(App& app) {
    std::vector<std::string> out;
    User* st = app.Data().FindUserById(app.SelectedStudent());
    if (!st) return out;
    auto& grades = app.Data().Grades();
    auto& subjects = app.Data().Subjects();

    float overall = Models::StudentOverallAverage(st->id, grades);
    if (overall >= 0) {
        char buf[96];
        std::snprintf(buf, sizeof(buf),
                      I18n::Current() == Lang::BG
                          ? "Общ среден успех: %.2f"
                          : "Overall GPA: %.2f",
                      overall);
        out.push_back(buf);
    }
    // Best subject
    int bestId = 0; float bestAvg = -1;
    for (const auto& s : subjects) {
        float a = Models::StudentSubjectAverage(st->id, s.id, grades);
        if (a > bestAvg) { bestAvg = a; bestId = s.id; }
    }
    if (bestId > 0 && bestAvg >= 0) {
        Subject* s = app.Data().FindSubject(bestId);
        if (s) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          I18n::Current() == Lang::BG
                              ? "Най-силна дисциплина: „%s“ (%.2f)"
                              : "Strongest course: \"%s\" (%.2f)",
                          s->name.c_str(), bestAvg);
            out.push_back(buf);
        }
    }
    return out;
}

void StudentDetailScreen::Update(App& app, float dt, Vector2 mouse) {
    enterT = Easing::Approach(enterT, 1.0f, 0.30f, dt);
    overallCircle.Update(dt);
    trendChart.Update(dt);

    backBtn.bounds      = { 24, 92, 100, 36 };
    addGradeBtn.bounds  = { (float)app.W() - 200, 92, 176, 36 };
    backBtn.Update(dt, mouse);
    addGradeBtn.Update(dt, mouse);

    bool consumed = addGradeModal.IsVisible() || confirmDeleteModal.IsVisible();
    if (!consumed) {
        if (backBtn.Clicked(mouse))     app.Goto(ScreenId::TEACHER_DASH);
        if (addGradeBtn.Clicked(mouse)) {
            addGradeModal.Show();
            agSubjectId = 0; agValue = 6; agComment.value.clear();
        }
    }

    // Modals
    addGradeModal.Update(dt);
    confirmDeleteModal.Update(dt);

    if (addGradeModal.IsVisible()) {
        agComment.Update(dt, mouse);
        agSave.Update(dt, mouse);
        agCancel.Update(dt, mouse);
        if (agCancel.Clicked(mouse) || IsKeyPressed(KEY_ESCAPE)) addGradeModal.Hide();
        if (agSave.Clicked(mouse) && agSubjectId != 0) {
            app.Data().AddGrade(app.SelectedStudent(), agSubjectId, agValue, agComment.value);
            addGradeModal.Hide();
            app.Toast(I18n::Current() == Lang::BG ? "Оценката беше запазена."
                                                  : "Grade saved.",
                      ThemeMgr::Current().success);
        }
    }
    if (confirmDeleteModal.IsVisible()) {
        cdDelete.Update(dt, mouse);
        cdCancel.Update(dt, mouse);
        if (cdCancel.Clicked(mouse) || IsKeyPressed(KEY_ESCAPE)) confirmDeleteModal.Hide();
        if (cdDelete.Clicked(mouse)) {
            app.Data().DeleteGrade(pendingDeleteId);
            confirmDeleteModal.Hide();
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && !consumed) scrollTarget -= wheel * 60.0f;
    if (scrollTarget < 0) scrollTarget = 0;
    scrollY = Easing::Approach(scrollY, scrollTarget, 0.15f, dt);
}

void StudentDetailScreen::DrawHeader(App& app) {
    const Theme& th = ThemeMgr::Current();
    int sw = app.W();
    DrawRectangle(0, 0, sw, 80, th.surface);
    DrawRectangle(0, 80, sw, 1, th.border);

    User* st = app.Data().FindUserById(app.SelectedStudent());
    if (st) {
        UI::DrawTextLeft(Bld(), st->fullName.c_str(), { 28, 22 }, 18.0f, th.textPrimary);
        std::string sub = std::string(I18n::T("student.faculty_number")) + " " +
                          st->facultyNumber + "  ·  " + st->className;
        UI::DrawTextLeft(Fnt(), sub.c_str(), { 28, 48 }, 12.0f, th.textMuted);
    }
    backBtn.Draw();
    addGradeBtn.Draw();
}

void StudentDetailScreen::DrawSummary(App& app, Rectangle r, float dt) {
    (void)dt;
    const Theme& th = ThemeMgr::Current();
    UI::DrawSoftShadow(r, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(r, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(r, 0.04f, 8, 1.0f, th.border);

    User* st = app.Data().FindUserById(app.SelectedStudent());
    if (!st) return;
    float avg = Models::StudentOverallAverage(st->id, app.Data().Grades());

    UI::DrawTextLeft(Bld(), I18n::T("student.gpa"),
                     { r.x + 24, r.y + 24 }, 14.0f, th.textMuted);
    char gv[16];
    if (avg >= 0) std::snprintf(gv, sizeof(gv), "%.2f", avg);
    else          std::snprintf(gv, sizeof(gv), "—");
    UI::DrawTextLeft(Dis(), gv, { r.x + 24, r.y + 50 }, 36.0f, th.textPrimary);

    overallCircle.center = { r.x + r.width - 70, r.y + r.height / 2 };
    overallCircle.radius = 40;
    overallCircle.label  = gv;
    overallCircle.Draw(Bld());
}

void StudentDetailScreen::DrawInsights(App& app, Rectangle r) {
    const Theme& th = ThemeMgr::Current();
    UI::DrawSoftShadow(r, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(r, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(r, 0.04f, 8, 1.0f, th.border);
    UI::DrawTextLeft(Bld(),
                     I18n::Current() == Lang::BG ? "Анализ" : "Insights",
                     { r.x + 20, r.y + 16 }, 14.0f, th.textPrimary);
    auto items = BuildInsights(app);
    float y = r.y + 50;
    if (items.empty()) {
        UI::DrawTextLeft(Fnt(),
                         I18n::Current() == Lang::BG ? "Няма достатъчно данни." : "Not enough data yet.",
                         { r.x + 20, y }, 12.0f, th.textMuted);
        return;
    }
    for (const auto& s : items) {
        DrawCircleV({ r.x + 26, y + 8 }, 3.0f, th.primary);
        UI::DrawTextLeft(Fnt(), s.c_str(), { r.x + 38, y }, 12.0f, th.textSecondary);
        y += 24;
    }
}

void StudentDetailScreen::DrawTrend(App& app, Rectangle r) {
    (void)app;
    const Theme& th = ThemeMgr::Current();
    UI::DrawSoftShadow(r, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(r, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(r, 0.04f, 8, 1.0f, th.border);
    trendChart.bounds = { r.x + 16, r.y + 16, r.width - 32, r.height - 32 };
    trendChart.stroke = th.primary;
    trendChart.Draw(Bld());
}

void StudentDetailScreen::DrawSubjectsAndGrades(App& app, Rectangle area, float dt, Vector2 mouse) {
    (void)dt;
    const Theme& th = ThemeMgr::Current();
    User* st = app.Data().FindUserById(app.SelectedStudent());
    if (!st) return;

    UI::DrawSoftShadow(area, 0.04f, 18.0f, 0.06f);
    DrawRectangleRounded(area, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(area, 0.04f, 8, 1.0f, th.border);
    UI::DrawTextLeft(Bld(),
                     I18n::Current() == Lang::BG ? "Оценки" : "Grades",
                     { area.x + 20, area.y + 16 }, 14.0f, th.textPrimary);

    Rectangle scroll = { area.x + 12, area.y + 50, area.width - 24, area.height - 64 };
    BeginScissorMode((int)scroll.x, (int)scroll.y, (int)scroll.width, (int)scroll.height);
    float py = scroll.y - scrollY;

    std::vector<const Grade*> gs;
    for (const auto& g : app.Data().Grades()) if (g.studentId == st->id) gs.push_back(&g);
    std::sort(gs.begin(), gs.end(),
              [](const Grade* a, const Grade* b){ return a->createdAt > b->createdAt; });

    hoverGradeId = 0;
    for (const Grade* gp : gs) {
        Rectangle row = { scroll.x + 4, py, scroll.width - 8, 56 };
        bool hov = CheckCollisionPointRec(mouse, row);
        if (hov) hoverGradeId = gp->id;
        DrawRectangleRounded(row, 0.08f, 6, hov ? th.surfaceAlt : th.surfaceMuted);

        Subject* s = app.Data().FindSubject(gp->subjectId);
        Color sc = s ? UI::SubjectColor(s->colorIndex) : th.primary;
        DrawRectangleRounded({ row.x, row.y, 4, row.height }, 1.0f, 4, sc);

        UI::DrawTextLeft(Bld(), s ? s->code.c_str() : "—",
                         { row.x + 18, row.y + 8 }, 11.0f, th.textMuted);
        UI::DrawTextLeft(Bld(), s ? s->name.c_str() : "—",
                         { row.x + 18, row.y + 22 }, 13.0f, th.textPrimary);
        UI::DrawTextLeft(Fnt(), Models::FormatDate(gp->year, gp->month, gp->day).c_str(),
                         { row.x + 18, row.y + 40 }, 10.0f, th.textMuted);

        Rectangle gb = { row.x + row.width - 70, row.y + 12, 50, 32 };
        Color gc = ThemeMgr::GradeColor((float)gp->value);
        DrawRectangleRounded(gb, 0.30f, 6, ColorAlpha(gc, 0.15f));
        UI::StrokeRoundedRect(gb, 0.30f, 6, 1.0f, gc);
        char gv[8]; std::snprintf(gv, sizeof(gv), "%d", gp->value);
        UI::DrawTextCenter(Bld(), gv, gb, 18.0f, gc, 0.5f);

        if (hov) {
            // delete button to the left of grade box
            Rectangle xb = { gb.x - 36, gb.y + 4, 28, 24 };
            bool xh = CheckCollisionPointRec(mouse, xb);
            DrawRectangleRounded(xb, 0.4f, 4, xh ? ColorAlpha(th.error, 0.20f) : ColorAlpha(th.error, 0.10f));
            UI::IconX({ xb.x + xb.width * 0.5f, xb.y + xb.height * 0.5f }, 7.0f, th.error, 2.0f);
            if (xh) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    pendingDeleteId = gp->id;
                    confirmDeleteModal.Show();
                }
            }
        }

        py += 64;
    }

    float used = py + scrollY - scroll.y;
    if (used < scroll.height) scrollTarget = 0;
    else if (scrollTarget > used - scroll.height) scrollTarget = used - scroll.height;

    EndScissorMode();
}

void StudentDetailScreen::DrawAddGradeModal(App& app, float dt, Vector2 mouse) {
    (void)dt; (void)mouse;
    if (!addGradeModal.IsVisible()) return;
    const Theme& th = ThemeMgr::Current();
    Rectangle content = addGradeModal.BeginFrame();

    UI::DrawTextLeft(Dis(), I18n::T("teacher.add_grade"),
                     { content.x, content.y }, 22.0f, th.textPrimary);

    // Subject grid
    UI::DrawTextLeft(Fnt(),
                     I18n::Current() == Lang::BG ? "Дисциплина" : "Subject",
                     { content.x, content.y + 50 }, 11.0f, th.textMuted);
    auto& subjects = app.Data().Subjects();
    float bx = content.x;
    float by = content.y + 70;
    for (size_t i = 0; i < subjects.size(); ++i) {
        const auto& s = subjects[i];
        Vector2 m = UI::MeasureText(Bld(), s.code.c_str(), 11.0f);
        Rectangle b = { bx, by, m.x + 18, 28 };
        if (b.x + b.width > content.x + content.width) {
            bx = content.x; by += 36;
            b.x = bx; b.y = by;
        }
        bool active = (agSubjectId == s.id);
        bool hov = CheckCollisionPointRec(mouse, b);
        Color bg = active ? th.primary : (hov ? th.surfaceAlt : th.surface);
        Color fg = active ? WHITE : th.textSecondary;
        DrawRectangleRounded(b, 0.5f, 6, bg);
        UI::StrokeRoundedRect(b, 0.5f, 6, 1.0f, active ? th.primary : th.border);
        UI::DrawTextCenter(Bld(), s.code.c_str(), b, 11.0f, fg, 0.5f);
        if (hov) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) agSubjectId = s.id;
        }
        bx += b.width + 6;
    }

    // Grade pills
    UI::DrawTextLeft(Fnt(),
                     I18n::Current() == Lang::BG ? "Оценка" : "Grade",
                     { content.x, by + 50 }, 11.0f, th.textMuted);
    float gx = content.x;
    for (int v = 2; v <= 6; ++v) {
        Rectangle b = { gx, by + 70, 50, 40 };
        bool active = (agValue == v);
        bool hov = CheckCollisionPointRec(mouse, b);
        Color gc = ThemeMgr::GradeColor((float)v);
        Color bg = active ? gc : (hov ? th.surfaceAlt : th.surface);
        Color fg = active ? WHITE : gc;
        DrawRectangleRounded(b, 0.30f, 6, bg);
        UI::StrokeRoundedRect(b, 0.30f, 6, 1.0f, gc);
        char gb[8]; std::snprintf(gb, sizeof(gb), "%d", v);
        UI::DrawTextCenter(Bld(), gb, b, 18.0f, fg, 0.5f);
        if (hov) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) agValue = v;
        }
        gx += b.width + 8;
    }

    // Comment
    agComment.bounds = { content.x, by + 132, content.width, 44 };
    agComment.Draw();

    // Buttons
    agSave.label   = I18n::T("common.save");
    agCancel.label = I18n::T("common.cancel");
    agSave.bounds   = { content.x + content.width - 130, content.y + content.height - 50, 130, 44 };
    agCancel.bounds = { content.x + content.width - 270, content.y + content.height - 50, 130, 44 };
    agSave.Draw();
    agCancel.Draw();

    addGradeModal.EndFrame();
}

void StudentDetailScreen::DrawConfirmDeleteModal(App& app, float dt, Vector2 mouse) {
    (void)app; (void)dt; (void)mouse;
    if (!confirmDeleteModal.IsVisible()) return;
    const Theme& th = ThemeMgr::Current();
    Rectangle content = confirmDeleteModal.BeginFrame();
    UI::DrawTextLeft(Bld(),
                     I18n::Current() == Lang::BG ? "Изтриване на оценка?" : "Delete this grade?",
                     { content.x, content.y }, 16.0f, th.textPrimary);
    UI::DrawTextLeft(Fnt(),
                     I18n::Current() == Lang::BG
                         ? "Това действие не може да бъде отменено."
                         : "This action cannot be undone.",
                     { content.x, content.y + 28 }, 12.0f, th.textMuted);

    cdDelete.label = I18n::T("common.delete");
    cdCancel.label = I18n::T("common.cancel");
    cdDelete.bounds = { content.x + content.width - 130, content.y + content.height - 50, 130, 44 };
    cdCancel.bounds = { content.x + content.width - 270, content.y + content.height - 50, 130, 44 };
    cdDelete.Draw();
    cdCancel.Draw();
    confirmDeleteModal.EndFrame();
}

void StudentDetailScreen::Draw(App& app) {
    int sw = app.W(), sh = app.H();
    const float topH = 80.0f;
    DrawHeader(app);

    Rectangle area = { 24, topH + 56, (float)sw - 48, (float)sh - topH - 80 };

    Rectangle summary = { area.x, area.y, area.width / 2 - 12, 140 };
    Rectangle insights = { area.x + area.width / 2 + 12, area.y, area.width / 2 - 12, 140 };
    DrawSummary(app, summary, GetFrameTime());
    DrawInsights(app, insights);

    Rectangle trend = { area.x, summary.y + summary.height + 16,
                        area.width / 2 - 12, 220 };
    Rectangle gradesArea = { area.x + area.width / 2 + 12, summary.y + summary.height + 16,
                             area.width / 2 - 12, area.height - summary.height - 16 };
    DrawTrend(app, trend);
    DrawSubjectsAndGrades(app, gradesArea, GetFrameTime(), GetMousePosition());

    DrawAddGradeModal(app, GetFrameTime(), GetMousePosition());
    DrawConfirmDeleteModal(app, GetFrameTime(), GetMousePosition());
}

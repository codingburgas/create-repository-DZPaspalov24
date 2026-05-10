#include "screen_teacher.h"
#include "app.h"
#include "theme.h"
#include "easing.h"
#include "i18n.h"
#include <algorithm>
#include <cstdio>
#include <set>

namespace {
    const float kHeaderH = 80.0f;
    const float kToolbarH = 96.0f;
    const float kRowH = 90.0f;

    // Build initials "AJ" from "Alice Johnson"
    std::string Initials(const std::string& full) {
        std::string out;
        bool nextLetter = true;
        for (char c : full) {
            if (c == ' ') { nextLetter = true; continue; }
            if (nextLetter && (int)out.size() < 2) { out.push_back((char)toupper((unsigned char)c)); nextLetter = false; }
        }
        if (out.empty() && !full.empty()) out.push_back((char)toupper((unsigned char)full[0]));
        return out;
    }

    // Stable color from name (so each student has consistent avatar color)
    Color AvatarColor(const std::string& name, int idx) {
        unsigned int h = 2166136261u;
        for (char c : name) { h ^= (unsigned char)c; h *= 16777619u; }
        return UI::SubjectColor((int)((h + idx) % UI::SubjectColorCount()));
    }
}

void TeacherDashboard::OnEnter(App& app) {
    search.placeholder = "Search students by name or program...";
    search.maxLength = 60;

    addStudentBtn.kind = UI::Button::PRIMARY;
    addStudentBtn.iconChar = '+';
    addStudentBtn.label = I18n::T("teacher.add_student");
    manageSubjectsBtn.kind = UI::Button::SECONDARY;
    manageSubjectsBtn.label = I18n::T("teacher.subjects");
    statisticsBtn.kind = UI::Button::SECONDARY;
    statisticsBtn.label = I18n::T("teacher.statistics");
    logoutBtn.kind = UI::Button::GHOST;
    logoutBtn.label = I18n::T("common.logout");

    sortName.kind = sortAvgDesc.kind = sortAvgAsc.kind = sortClass.kind = UI::Button::GHOST;

    nsName.placeholder = "Full name";
    nsUser.placeholder = "Username";
    nsPass.placeholder = "Password";
    nsClass.placeholder = "Program";
    nsPass.password = true;
    nsSave.kind = UI::Button::PRIMARY;
    nsCancel.kind = UI::Button::GHOST;

    newSubjName.placeholder = "New subject name";
    newSubjAdd.kind = UI::Button::PRIMARY;

    agComment.placeholder = "Comment (optional)";
    agSave.kind = UI::Button::PRIMARY;
    agCancel.kind = UI::Button::GHOST;

    addStudentModal.bounds = {0, 0, 520, 460};
    subjectsModal.bounds   = {0, 0, 600, 540};
    addGradeModal.bounds   = {0, 0, 520, 460};

    RefreshClasses(app);
}

void TeacherDashboard::OnShow(App& app) {
    enterT = 0.0f;
    search.value.clear();
    RefreshClasses(app);
}

void TeacherDashboard::RefreshClasses(App& app) {
    std::set<std::string> uniq;
    for (auto& u : app.Data().Users()) {
        if (u.role == Role::STUDENT && !u.className.empty()) uniq.insert(u.className);
    }
    classes.assign(uniq.begin(), uniq.end());
    classChips.clear();
    UI::Button all{"All"};        all.kind = UI::Button::GHOST; classChips.push_back(all);
    for (auto& c : classes) {
        UI::Button b{c};          b.kind = UI::Button::GHOST;   classChips.push_back(b);
    }
    if (activeClass >= (int)classes.size()) activeClass = -1;
}

std::vector<int> TeacherDashboard::ComputeFilteredSorted(App& app) {
    std::vector<int> ids;
    auto& users = app.Data().Users();
    for (auto& u : users) {
        if (u.role != Role::STUDENT) continue;
        if (activeClass >= 0 && u.className != classes[activeClass]) continue;
        if (!search.value.empty()) {
            std::string q = search.value;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            std::string n = u.fullName, c = u.className;
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            std::transform(c.begin(), c.end(), c.begin(), ::tolower);
            if (n.find(q) == std::string::npos && c.find(q) == std::string::npos) continue;
        }
        ids.push_back(u.id);
    }

    auto& grades = app.Data().Grades();
    std::sort(ids.begin(), ids.end(), [&](int a, int b){
        User* ua = app.Data().FindUserById(a);
        User* ub = app.Data().FindUserById(b);
        if (!ua || !ub) return false;
        switch (sortBy) {
            case SortBy::NAME:     return ua->fullName < ub->fullName;
            case SortBy::AVG_DESC: {
                float xa = Models::StudentOverallAverage(a, grades);
                float xb = Models::StudentOverallAverage(b, grades);
                if (xa < 0) xa = -1; if (xb < 0) xb = -1;
                return xa > xb;
            }
            case SortBy::AVG_ASC: {
                float xa = Models::StudentOverallAverage(a, grades);
                float xb = Models::StudentOverallAverage(b, grades);
                if (xa < 0) xa = 9; if (xb < 0) xb = 9;
                return xa < xb;
            }
            case SortBy::CLASS:    return ua->className < ub->className;
        }
        return false;
    });

    return ids;
}

void TeacherDashboard::Update(App& app, float dt, Vector2 mouse) {
    enterT = Easing::Approach(enterT, 1.0f, 0.20f, dt);
    addStudentModal.Update(dt);
    subjectsModal.Update(dt);
    addGradeModal.Update(dt);

    int sw = app.W(), sh = app.H();

    // Header buttons - sit LEFT of the global theme/notif controls (sw-230, sw-290)
    addStudentBtn.bounds     = { (float)sw - 460, 24, 132, 40 };
    manageSubjectsBtn.bounds = { (float)sw - 604, 24, 132, 40 };
    statisticsBtn.bounds     = { (float)sw - 748, 24, 132, 40 };
    logoutBtn.bounds         = { (float)sw - 836, 24,  80, 40 };

    addStudentBtn.Update(dt, mouse);
    manageSubjectsBtn.Update(dt, mouse);
    statisticsBtn.Update(dt, mouse);
    logoutBtn.Update(dt, mouse);

    // Toolbar
    Rectangle toolbar = {28, kHeaderH + 8, (float)sw - 56, kToolbarH};
    search.bounds = { toolbar.x + 16, toolbar.y + 14, 360, 44 };
    sortName.bounds    = { toolbar.x + 16,           toolbar.y + 64, 90, 28 };
    sortAvgDesc.bounds = { toolbar.x + 110,          toolbar.y + 64, 80, 28 };
    sortAvgAsc.bounds  = { toolbar.x + 194,          toolbar.y + 64, 80, 28 };
    sortClass.bounds   = { toolbar.x + 278,          toolbar.y + 64, 90, 28 };

    bool clickConsumed = addStudentModal.IsVisible() || subjectsModal.IsVisible() || addGradeModal.IsVisible();
    search.Update(dt, mouse, clickConsumed);
    sortName.Update(dt, mouse);
    sortAvgDesc.Update(dt, mouse);
    sortAvgAsc.Update(dt, mouse);
    sortClass.Update(dt, mouse);

    if (sortName.Clicked(mouse))    sortBy = SortBy::NAME;
    if (sortAvgDesc.Clicked(mouse)) sortBy = SortBy::AVG_DESC;
    if (sortAvgAsc.Clicked(mouse))  sortBy = SortBy::AVG_ASC;
    if (sortClass.Clicked(mouse))   sortBy = SortBy::CLASS;

    // Class chips - layout
    float cx = toolbar.x + 396, cy = toolbar.y + 14;
    for (size_t i = 0; i < classChips.size(); ++i) {
        const std::string& lbl = classChips[i].label;
        float w = std::max(54.0f, UI::MeasureText(ThemeMgr::GetFont(), lbl.c_str(), 14.0f).x + 26.0f);
        if (cx + w > toolbar.x + toolbar.width - 16) { cx = toolbar.x + 396; cy = toolbar.y + 50; }
        classChips[i].bounds = { cx, cy, w, 30 };
        cx += w + 8;
        classChips[i].Update(dt, mouse);
        if (classChips[i].Clicked(mouse)) activeClass = (int)i - 1; // -1 = "All"
    }

    if (clickConsumed && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        // Modal interactions handled below; skip global background actions
    } else {
        if (addStudentBtn.Clicked(mouse))     { nsName.value.clear(); nsUser.value.clear(); nsPass.value.clear(); nsClass.value.clear(); nsError.clear(); addStudentModal.Show(); nsName.Focus(); }
        if (manageSubjectsBtn.Clicked(mouse)) { newSubjName.value.clear(); subjectsModal.Show(); newSubjName.Focus(); }
        if (statisticsBtn.Clicked(mouse))     app.Goto(ScreenId::STATISTICS);
        if (logoutBtn.Clicked(mouse))         { app.Logout(); app.Goto(ScreenId::LOGIN); }
    }

    // Scrolling student list
    Rectangle list = { 28, kHeaderH + kToolbarH + 24, (float)sw - 56,
                       (float)sh - (kHeaderH + kToolbarH + 24) - 24 };
    if (CheckCollisionPointRec(mouse, list)) {
        scrollTarget -= GetMouseWheelMove() * 60.0f;
    }
    int visibleCount = (int)ComputeFilteredSorted(app).size();
    float contentH = (float)visibleCount * (kRowH + 12.0f);
    float maxScroll = std::max(0.0f, contentH - list.height);
    if (scrollTarget < 0) scrollTarget = 0;
    if (scrollTarget > maxScroll) scrollTarget = maxScroll;
    scrollY = Easing::Approach(scrollY, scrollTarget, 0.10f, dt);

    // Process student row clicks (handled in DrawStudentList via hit-testing).
    // Modals
    if (addStudentModal.IsVisible()) {
        nsSave.bounds   = {0,0,0,0}; nsCancel.bounds = {0,0,0,0};
        // Will be positioned during Draw; but we want a single update step.
        // Our pattern: set bounds in DrawXXXModal, then call Update there.
        // To respect that, the modal-specific draw also handles button updates.
    }

    // Reset cursor for next frame so widgets can re-set it
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        // ok
    }
}

void TeacherDashboard::DrawHeader(App& app) {
    const Theme& th = ThemeMgr::Current();
    int sw = app.W();

    Font fb = ThemeMgr::GetFontBold();
    Font f  = ThemeMgr::GetFont();
    Font fd = ThemeMgr::GetFontDisplay();

    // Top bar (transparent over background)
    Rectangle brandPill = { 28, 28, 6, 32 };
    DrawRectangleRounded(brandPill, 1.0f, 6, th.primary);

    UI::DrawTextLeft(fd, I18n::T("app.subtitle"), {44, 18}, 22.0f, th.textPrimary);
    User* u = app.CurrentUser();
    std::string sub = u ? (std::string(I18n::T("login.welcome_back")) + ", " + u->fullName) : I18n::T("login.welcome_back");
    UI::DrawTextLeft(f, sub.c_str(), {44, 48}, 15.0f, th.textSecondary);

    addStudentBtn.Draw();
    manageSubjectsBtn.Draw();
    statisticsBtn.Draw();
    logoutBtn.Draw();
}

void TeacherDashboard::DrawToolbar(App& app, Rectangle r, float dt, Vector2 mouse) {
    const Theme& th = ThemeMgr::Current();
    UI::DrawCard(r, 0.10f);

    // Search input
    search.Draw();

    // Sort buttons - active one gets primary tint
    auto styleSort = [&](UI::Button& b, bool active){
        // Render as ghost, but with active highlight
        UI::Button copy = b;
        if (active) copy.kind = UI::Button::SECONDARY;
        copy.Draw();
    };
    styleSort(sortName,    sortBy == SortBy::NAME);
    styleSort(sortAvgDesc, sortBy == SortBy::AVG_DESC);
    styleSort(sortAvgAsc,  sortBy == SortBy::AVG_ASC);
    styleSort(sortClass,   sortBy == SortBy::CLASS);

    // Class chips
    Font f = ThemeMgr::GetFont();
    UI::DrawTextLeft(f, "Filter:", { r.x + 396 - 56, r.y + 22 }, 13.0f, th.textMuted);
    for (size_t i = 0; i < classChips.size(); ++i) {
        UI::Button& b = classChips[i];
        bool active = ((int)i - 1) == activeClass;
        // Custom render so we don't mutate the underlying button kind state
        UI::DrawSoftShadow(b.bounds, 1.0f, 6.0f, active ? 0.10f : 0.0f);
        Color bg = active ? th.primarySoft : th.surfaceMuted;
        if (b.hoverT > 0.05f && !active) bg = ThemeMgr::Lerp(bg, th.surfaceAlt, b.hoverT);
        DrawRectangleRounded(b.bounds, 1.0f, 6, bg);
        UI::StrokeRoundedRect(b.bounds, 1.0f, 6, 1.0f, active ? th.primary : th.border);
        Color tc = active ? th.primary : th.textSecondary;
        UI::DrawTextCenter(ThemeMgr::GetFontBold(), b.label.c_str(), b.bounds, 13.0f, tc, 0.5f);
    }
}

void TeacherDashboard::DrawStudentList(App& app, Rectangle area, float dt, Vector2 mouse) {
    const Theme& th = ThemeMgr::Current();
    Font f  = ThemeMgr::GetFont();
    Font fb = ThemeMgr::GetFontBold();
    Font fd = ThemeMgr::GetFontDisplay();

    // Use scissor so scroll content clips inside the area
    BeginScissorMode((int)area.x - 4, (int)area.y - 4, (int)area.width + 8, (int)area.height + 8);

    auto ids = ComputeFilteredSorted(app);
    auto& grades = app.Data().Grades();

    if (ids.empty()) {
        UI::DrawTextCenter(f, "No students match the current filters.", area, 16.0f, th.textMuted);
        EndScissorMode();
        return;
    }

    User* curUser = app.CurrentUser();
    bool clickConsumed = addStudentModal.IsVisible() || subjectsModal.IsVisible() || addGradeModal.IsVisible();

    for (size_t i = 0; i < ids.size(); ++i) {
        Rectangle row = { area.x, area.y + i * (kRowH + 12.0f) - scrollY,
                          area.width, kRowH };
        if (row.y + row.height < area.y - 80.0f) continue;
        if (row.y > area.y + area.height + 80.0f) continue;

        bool hover = !clickConsumed && CheckCollisionPointRec(mouse, row);
        UI::DrawCard(row, 0.10f, hover);

        User* u = app.Data().FindUserById(ids[i]);
        if (!u) continue;

        // Avatar
        Color avc = AvatarColor(u->fullName, ids[i]);
        Vector2 ac = { row.x + 36, row.y + row.height * 0.5f };
        DrawCircleV(ac, 26.0f, avc);
        std::string ini = Initials(u->fullName);
        Rectangle initBox = { ac.x - 26, ac.y - 26, 52, 52 };
        UI::DrawTextCenter(fb, ini.c_str(), initBox, 16.0f, WHITE, 0.5f);

        // Name + class
        UI::DrawTextLeft(fb, u->fullName.c_str(), {row.x + 80, row.y + 18}, 17.0f, th.textPrimary);
        std::string sub = u->className.empty() ? std::string("(no program)") : u->className;
        UI::DrawTextLeft(f,  sub.c_str(),         {row.x + 80, row.y + 42}, 13.0f, th.textSecondary);

        // Pin star (left of name) - is this student pinned by current teacher?
        bool pinned = false;
        if (curUser) {
            for (int p : curUser->pinnedStudents) if (p == u->id) { pinned = true; break; }
        }
        Rectangle pinR = { row.x + 76, row.y + 60, 16, 16 };
        Color starC = pinned ? th.warning : ColorAlpha(th.textMuted, 0.6f);
        // simple star approximated with a circle + bracket
        DrawTextEx(fb, pinned ? "*" : "*", {pinR.x, pinR.y - 4}, 18.0f, 1.0f, starC);
        if (!clickConsumed && CheckCollisionPointRec(mouse, pinR) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (curUser) {
                if (pinned) {
                    auto it = std::remove(curUser->pinnedStudents.begin(), curUser->pinnedStudents.end(), u->id);
                    curUser->pinnedStudents.erase(it, curUser->pinnedStudents.end());
                } else {
                    curUser->pinnedStudents.push_back(u->id);
                }
                app.Data().UpdateUser(*curUser);
            }
        }

        // Average (centered-ish)
        float avg = Models::StudentOverallAverage(u->id, grades);
        char avgBuf[16]; std::snprintf(avgBuf, sizeof(avgBuf), avg > 0 ? "%.2f" : "—", avg);
        Color ac2 = (avg > 0) ? ThemeMgr::GradeColor(avg) : th.textMuted;
        Rectangle avgBox = { row.x + row.width - 380, row.y + 14, 90, row.height - 28 };
        DrawRectangleRounded(avgBox, 0.30f, 8, ColorAlpha(ac2, 0.10f));
        UI::DrawTextCenter(fd, avgBuf, avgBox, 26.0f, ac2, 0.5f);
        UI::DrawTextCenter(f, "Avg", {avgBox.x, avgBox.y + avgBox.height - 14, avgBox.width, 12}, 11.0f, th.textMuted);

        // Mini grade pills (last few grades)
        std::vector<const Grade*> recent;
        for (const auto& g : grades) if (g.studentId == u->id) recent.push_back(&g);
        std::sort(recent.begin(), recent.end(),
                  [](const Grade* a, const Grade* b){ return a->createdAt > b->createdAt; });
        float px = row.x + row.width - 280;
        for (size_t k = 0; k < std::min<size_t>(5, recent.size()); ++k) {
            float gv = (float)recent[k]->value;
            Rectangle pill = { px, row.y + 32, 26, 26 };
            DrawRectangleRounded(pill, 0.50f, 8, ThemeMgr::GradeColor(gv));
            char b[8]; std::snprintf(b, sizeof(b), "%d", recent[k]->value);
            UI::DrawTextCenter(fb, b, pill, 14.0f, WHITE, 0.5f);
            px += 32;
        }

        // Action buttons (Add Grade, Open) - draw as inline mini-buttons
        Rectangle openBtn = { row.x + row.width - 100, row.y + 24, 84, 38 };
        bool openHover = !clickConsumed && CheckCollisionPointRec(mouse, openBtn);
        Color obg = ThemeMgr::Lerp(th.surfaceMuted, th.primarySoft, openHover ? 1.0f : 0.0f);
        DrawRectangleRounded(openBtn, 0.40f, 8, obg);
        UI::StrokeRoundedRect(openBtn, 0.40f, 8, 1.0f, th.border);
        Color otc = openHover ? th.primary : th.textSecondary;
        UI::DrawTextCenter(fb, "View", openBtn, 13.0f, otc);

        Rectangle gradeBtn = { row.x + row.width - 196, row.y + 24, 84, 38 };
        bool gradeHover = !clickConsumed && CheckCollisionPointRec(mouse, gradeBtn);
        Color gbg = ThemeMgr::Lerp(th.primary, th.primaryHover, gradeHover ? 1.0f : 0.0f);
        if (gradeHover) UI::DrawSoftShadow(gradeBtn, 0.40f, 10.0f, 0.18f);
        DrawRectangleRounded(gradeBtn, 0.40f, 8, gbg);
        UI::DrawTextCenter(fb, "+ Grade", gradeBtn, 13.0f, WHITE);

        if (!clickConsumed && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, openBtn)) {
                app.SetSelectedStudent(u->id);
                app.Goto(ScreenId::STUDENT_DETAIL);
            } else if (CheckCollisionPointRec(mouse, gradeBtn)) {
                agStudentId = u->id;
                agSubjectId = app.Data().Subjects().empty() ? 0 : app.Data().Subjects()[0].id;
                agValue = 6;
                agComment.value.clear();
                addGradeModal.Show();
            } else if (CheckCollisionPointRec(mouse, row)) {
                // Click on row body opens detail
                app.SetSelectedStudent(u->id);
                app.Goto(ScreenId::STUDENT_DETAIL);
            }
        }
    }

    EndScissorMode();
}

void TeacherDashboard::DrawAddStudentModal(App& app, float dt, Vector2 mouse) {
    if (!addStudentModal.IsVisible()) return;
    Rectangle content = addStudentModal.BeginFrame();
    const Theme& th = ThemeMgr::Current();
    Font f = ThemeMgr::GetFont();
    Font fb = ThemeMgr::GetFontBold();
    Font fd = ThemeMgr::GetFontDisplay();

    UI::DrawTextLeft(fd, "Add Student", {content.x, content.y}, 24.0f, th.textPrimary);
    UI::DrawTextLeft(f,  "Create a new student account",
                     {content.x, content.y + 32}, 13.0f, th.textSecondary);

    nsName.bounds  = { content.x, content.y + 76,  content.width, 44 };
    nsUser.bounds  = { content.x, content.y + 146, content.width * 0.48f, 44 };
    nsClass.bounds = { content.x + content.width * 0.52f, content.y + 146, content.width * 0.48f, 44 };
    nsPass.bounds  = { content.x, content.y + 216, content.width, 44 };

    UI::DrawTextLeft(f, "Full name",  {nsName.bounds.x, nsName.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(f, "Username",   {nsUser.bounds.x, nsUser.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(f, "Program",    {nsClass.bounds.x, nsClass.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(f, "Password",   {nsPass.bounds.x, nsPass.bounds.y - 18}, 12.0f, th.textMuted);

    nsName.Update(dt, mouse);
    nsUser.Update(dt, mouse);
    nsClass.Update(dt, mouse);
    nsPass.Update(dt, mouse);
    nsName.Draw(); nsUser.Draw(); nsClass.Draw(); nsPass.Draw();

    if (!nsError.empty()) {
        Color e = th.error;
        Rectangle bg = { content.x, content.y + 290, content.width, 28 };
        DrawRectangleRounded(bg, 0.40f, 8, ColorAlpha(th.error, 0.10f));
        UI::DrawTextCenter(f, nsError.c_str(), bg, 13.0f, e);
    }

    nsSave.bounds   = { content.x + content.width - 130, content.y + content.height - 56, 130, 44 };
    nsCancel.bounds = { content.x + content.width - 270, content.y + content.height - 56, 130, 44 };

    nsSave.Update(dt, mouse);
    nsCancel.Update(dt, mouse);
    nsSave.Draw(); nsCancel.Draw();

    if (nsCancel.Clicked(mouse) || IsKeyPressed(KEY_ESCAPE)) {
        addStudentModal.Hide();
    }
    if (nsSave.Clicked(mouse) || IsKeyPressed(KEY_ENTER)) {
        nsError.clear();
        if (nsName.value.empty() || nsUser.value.empty() || nsPass.value.empty()) {
            nsError = "Please fill in name, username and password.";
        } else if (app.Data().FindUserByUsername(nsUser.value)) {
            nsError = "Username already exists.";
        } else {
            User u;
            u.role = Role::STUDENT;
            u.fullName = nsName.value; u.username = nsUser.value;
            u.passwordEnc = Models::ObfuscatePassword(nsPass.value);
            u.className = nsClass.value;
            app.Data().CreateUser(u);
            app.Toast("Student added: " + u.fullName, ThemeMgr::Current().success);
            addStudentModal.Hide();
            RefreshClasses(app);
        }
    }
    addStudentModal.EndFrame();
}

void TeacherDashboard::DrawSubjectsModal(App& app, float dt, Vector2 mouse) {
    if (!subjectsModal.IsVisible()) return;
    Rectangle content = subjectsModal.BeginFrame();
    const Theme& th = ThemeMgr::Current();
    Font f = ThemeMgr::GetFont();
    Font fb = ThemeMgr::GetFontBold();
    Font fd = ThemeMgr::GetFontDisplay();

    UI::DrawTextLeft(fd, "Subjects", {content.x, content.y}, 24.0f, th.textPrimary);
    UI::DrawTextLeft(f,  "Manage subjects offered to students",
                     {content.x, content.y + 32}, 13.0f, th.textSecondary);

    newSubjName.bounds = { content.x, content.y + 78, content.width - 280, 44 };
    UI::DrawTextLeft(f, "Name", {newSubjName.bounds.x, newSubjName.bounds.y - 18}, 12.0f, th.textMuted);
    newSubjName.Update(dt, mouse);
    newSubjName.Draw();

    // Color swatches
    UI::DrawTextLeft(f, "Color", {content.x + content.width - 260, newSubjName.bounds.y - 18}, 12.0f, th.textMuted);
    for (int i = 0; i < UI::SubjectColorCount(); ++i) {
        Rectangle s = { content.x + content.width - 260 + i * 30.0f,
                        newSubjName.bounds.y + 8, 24, 24 };
        DrawCircleV({s.x + 12, s.y + 12}, 12, UI::SubjectColor(i));
        if (pickedColor == i) {
            DrawCircleLines((int)(s.x + 12), (int)(s.y + 12), 16, th.textPrimary);
        }
        if (CheckCollisionPointRec(mouse, s) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            pickedColor = i;
        }
        if (CheckCollisionPointRec(mouse, s)) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    newSubjAdd.bounds = { content.x + content.width - 100, content.y + 78, 100, 44 };
    newSubjAdd.Update(dt, mouse);
    newSubjAdd.Draw();
    if (newSubjAdd.Clicked(mouse) && !newSubjName.value.empty()) {
        app.Data().CreateSubject(newSubjName.value, pickedColor);
        app.Toast("Subject added: " + newSubjName.value, th.secondary);
        newSubjName.value.clear();
    }

    // List existing
    Rectangle list = { content.x, content.y + 150,
                       content.width, content.height - 150 - 60 };
    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    auto& subs = app.Data().Subjects();
    for (size_t i = 0; i < subs.size(); ++i) {
        Rectangle row = { list.x, list.y + i * 56.0f, list.width, 48 };
        UI::DrawCard(row, 0.20f, false);
        DrawCircleV({row.x + 24, row.y + 24}, 10.0f, UI::SubjectColor(subs[i].colorIndex));
        UI::DrawTextLeft(fb, subs[i].name.c_str(), {row.x + 46, row.y + 14}, 16.0f, th.textPrimary);
        // Class average
        float ca = Models::SubjectClassAverage(subs[i].id, app.Data().Grades());
        char ab[24]; std::snprintf(ab, sizeof(ab), ca > 0 ? "Avg %.2f" : "No grades", ca);
        UI::DrawTextRight(f, ab, {row.x, row.y, row.width - 80, row.height}, 13.0f, th.textSecondary);

        // Delete button
        Rectangle delR = { row.x + row.width - 64, row.y + 8, 56, 32 };
        bool h = CheckCollisionPointRec(mouse, delR);
        Color bg = h ? ColorAlpha(th.error, 0.12f) : th.surfaceMuted;
        DrawRectangleRounded(delR, 0.40f, 8, bg);
        UI::DrawTextCenter(fb, "Delete", delR, 12.0f, h ? th.error : th.textSecondary);
        if (h && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            app.Data().DeleteSubject(subs[i].id);
            app.Toast("Subject removed", th.warning);
            EndScissorMode();
            subjectsModal.EndFrame();
            return;
        }
        if (h) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }
    EndScissorMode();

    UI::Button close{"Close"}; close.kind = UI::Button::GHOST;
    close.bounds = { content.x + content.width - 110, content.y + content.height - 50, 110, 40 };
    close.Update(dt, mouse); close.Draw();
    if (close.Clicked(mouse) || IsKeyPressed(KEY_ESCAPE)) subjectsModal.Hide();

    subjectsModal.EndFrame();
}

void TeacherDashboard::DrawAddGradeModal(App& app, float dt, Vector2 mouse) {
    if (!addGradeModal.IsVisible()) return;
    Rectangle content = addGradeModal.BeginFrame();
    const Theme& th = ThemeMgr::Current();
    Font f = ThemeMgr::GetFont();
    Font fb = ThemeMgr::GetFontBold();
    Font fd = ThemeMgr::GetFontDisplay();

    User* st = app.Data().FindUserById(agStudentId);
    if (!st) { addGradeModal.Hide(); addGradeModal.EndFrame(); return; }

    UI::DrawTextLeft(fd, "Add Grade", {content.x, content.y}, 24.0f, th.textPrimary);
    std::string subline = std::string("For student ") + st->fullName;
    UI::DrawTextLeft(f, subline.c_str(), {content.x, content.y + 32}, 13.0f, th.textSecondary);

    // Subject picker - row of pills
    UI::DrawTextLeft(f, "Subject", {content.x, content.y + 72}, 12.0f, th.textMuted);
    auto& subs = app.Data().Subjects();
    float px = content.x, py = content.y + 92;
    for (size_t i = 0; i < subs.size(); ++i) {
        Vector2 m = ::MeasureTextEx(fb, subs[i].name.c_str(), 14.0f, 1.0f);
        float w = m.x + 28.0f;
        if (px + w > content.x + content.width) { px = content.x; py += 38; }
        Rectangle r = { px, py, w, 30 };
        bool active = subs[i].id == agSubjectId;
        bool hov = CheckCollisionPointRec(mouse, r);
        Color bg = active ? UI::SubjectColor(subs[i].colorIndex) : (hov ? th.surfaceAlt : th.surfaceMuted);
        DrawRectangleRounded(r, 1.0f, 6, bg);
        if (!active) UI::StrokeRoundedRect(r, 1.0f, 6, 1.0f, th.border);
        Color tc = active ? WHITE : th.textPrimary;
        UI::DrawTextCenter(fb, subs[i].name.c_str(), r, 13.0f, tc);
        if (hov) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (hov && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) agSubjectId = subs[i].id;
        px += w + 8;
    }

    // Grade selector (2..6) - big tap-able cells
    py += 56;
    UI::DrawTextLeft(f, "Grade", {content.x, py - 18}, 12.0f, th.textMuted);
    float gw = (content.width - 16) / 5.0f;
    for (int v = 2; v <= 6; ++v) {
        Rectangle r = { content.x + (v - 2) * (gw + 4), py, gw, 56 };
        bool active = v == agValue;
        bool hov = CheckCollisionPointRec(mouse, r);
        Color base = ThemeMgr::GradeColor((float)v);
        Color bg = active ? base : (hov ? ColorAlpha(base, 0.15f) : th.surfaceMuted);
        if (active) UI::DrawSoftShadow(r, 0.30f, 10.0f, 0.20f);
        DrawRectangleRounded(r, 0.30f, 8, bg);
        if (!active) UI::StrokeRoundedRect(r, 0.30f, 8, 1.5f, ColorAlpha(base, 0.6f));
        char b[4]; std::snprintf(b, sizeof(b), "%d", v);
        Color tc = active ? WHITE : base;
        UI::DrawTextCenter(fd, b, r, 28.0f, tc, 0.5f);
        if (hov) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (hov && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) agValue = v;
    }

    // Comment
    py += 80;
    agComment.bounds = { content.x, py, content.width, 44 };
    UI::DrawTextLeft(f, "Comment (optional)", {content.x, py - 18}, 12.0f, th.textMuted);
    agComment.Update(dt, mouse);
    agComment.Draw();

    agSave.bounds   = { content.x + content.width - 150, content.y + content.height - 50, 150, 44 };
    agCancel.bounds = { content.x + content.width - 280, content.y + content.height - 50, 120, 44 };
    agSave.Update(dt, mouse); agCancel.Update(dt, mouse);
    agSave.Draw(); agCancel.Draw();

    bool save = agSave.Clicked(mouse) || IsKeyPressed(KEY_ENTER);
    bool cancel = agCancel.Clicked(mouse) || IsKeyPressed(KEY_ESCAPE);
    if (cancel) addGradeModal.Hide();
    if (save && agSubjectId > 0) {
        app.Data().AddGrade(agStudentId, agSubjectId, agValue, agComment.value);
        Subject* s = app.Data().FindSubject(agSubjectId);
        std::string msg = std::string("Added grade ") + std::to_string(agValue);
        if (s) msg += " in " + s->name;
        app.Toast(msg, th.success);
        addGradeModal.Hide();
    }
    addGradeModal.EndFrame();
}

void TeacherDashboard::Draw(App& app) {
    int sw = app.W(), sh = app.H();
    DrawHeader(app);

    Rectangle toolbar = {28, kHeaderH, (float)sw - 56, kToolbarH};
    DrawToolbar(app, toolbar, GetFrameTime(), GetMousePosition());

    Rectangle list = { 28, kHeaderH + kToolbarH + 24, (float)sw - 56,
                       (float)sh - (kHeaderH + kToolbarH + 24) - 24 };
    DrawStudentList(app, list, GetFrameTime(), GetMousePosition());

    DrawAddStudentModal(app, GetFrameTime(), GetMousePosition());
    DrawSubjectsModal(app,  GetFrameTime(), GetMousePosition());
    DrawAddGradeModal(app,  GetFrameTime(), GetMousePosition());
}

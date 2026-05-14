#include "screen_stats.h"
#include "app.h"
#include "theme.h"
#include "easing.h"
#include "models.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <map>

namespace {
    const float kHeaderH = 80.0f;
}

void StatisticsScreen::OnEnter(App& app) {
    backBtn.kind = UI::Button::GHOST;
    backBtn.iconChar = '<';

    distChart.title = "Grade distribution";
    distChart.maxValue = 1.0f;       // recomputed per data
    distChart.showValues = true;

    subjectChart.title = "Subject averages";
    subjectChart.maxValue = 6.0f;

    trendChart.title = "Program performance over time";
    trendChart.minY = 2.0f;
    trendChart.maxY = 6.0f;

    RefreshClasses(app);
}

void StatisticsScreen::OnShow(App& app) {
    enterT = 0.0f;
    RefreshClasses(app);
    RebuildData(app);
    distChart.OnShow();
    subjectChart.OnShow();
    trendChart.OnShow();
}

bool StatisticsScreen::PassesFilter(const User& u) const {
    if (u.role != Role::STUDENT) return false;
    if (activeClass < 0) return true;
    if (activeClass >= (int)classes.size()) return true;
    return u.className == classes[activeClass];
}

void StatisticsScreen::RefreshClasses(App& app) {
    std::set<std::string> uniq;
    for (auto& u : app.Data().Users()) {
        if (u.role == Role::STUDENT && !u.className.empty()) uniq.insert(u.className);
    }
    classes.assign(uniq.begin(), uniq.end());
    classChips.clear();
    UI::Button all{"All programs"}; all.kind = UI::Button::GHOST; classChips.push_back(all);
    for (auto& c : classes) {
        UI::Button b{c}; b.kind = UI::Button::GHOST; classChips.push_back(b);
    }
    if (activeClass >= (int)classes.size()) activeClass = -1;
}

void StatisticsScreen::RebuildData(App& app) {
    auto& grades = app.Data().Grades();
    auto& subs   = app.Data().Subjects();
    auto& users  = app.Data().Users();
    const Theme& th = ThemeMgr::Current();

    // Filtered student-id set
    std::set<int> studentIds;
    for (auto& u : users) if (PassesFilter(u)) studentIds.insert(u.id);

    // Filter grades to that set
    std::vector<Grade> filteredG;
    filteredG.reserve(grades.size());
    for (auto& g : grades) if (studentIds.count(g.studentId)) filteredG.push_back(g);

    // ----- Distribution: 5 buckets (grade 2..6) -----
    int buckets[5] = {0,0,0,0,0};
    Models::GradeDistribution(filteredG, buckets);
    distChart.items.clear();
    int distMax = 1;
    for (int i = 0; i < 5; ++i) if (buckets[i] > distMax) distMax = buckets[i];
    distChart.maxValue = (float)distMax;
    for (int i = 0; i < 5; ++i) {
        UI::BarChartItem it;
        char lbl[8]; std::snprintf(lbl, sizeof(lbl), "%d", i + 2);
        it.label = lbl;
        it.value = (float)buckets[i];
        it.color = ThemeMgr::GradeColor((float)(i + 2));
        distChart.items.push_back(it);
    }

    // ----- Subject averages -----
    subjectChart.items.clear();
    subjectChart.maxValue = 6.0f;
    for (auto& s : subs) {
        // class average within filtered students
        float sum = 0; int n = 0;
        for (auto& g : filteredG) {
            if (g.subjectId == s.id) { sum += g.value; n++; }
        }
        UI::BarChartItem it;
        it.label = s.name.size() > 9 ? (s.name.substr(0, 8) + ".") : s.name;
        it.value = n > 0 ? sum / (float)n : 0.0f;
        it.color = UI::SubjectColor(s.colorIndex);
        subjectChart.items.push_back(it);
    }

    // ----- Trend over time: monthly averages, last 8 months -----
    std::map<int, std::pair<float,int>> bucketByYearMonth;
    for (auto& g : filteredG) {
        int key = g.year * 100 + g.month;
        auto& p = bucketByYearMonth[key];
        p.first += g.value;
        p.second += 1;
    }
    trendChart.values.clear();
    trendChart.xLabels.clear();
    int kept = 0;
    int total = (int)bucketByYearMonth.size();
    int skip = std::max(0, total - 8);
    int i = 0;
    for (auto& kv : bucketByYearMonth) {
        if (i++ < skip) continue;
        if (kv.second.second == 0) continue;
        trendChart.values.push_back(kv.second.first / (float)kv.second.second);
        char lbl[16];
        std::snprintf(lbl, sizeof(lbl), "%d/%02d", kv.first / 100, kv.first % 100);
        trendChart.xLabels.push_back(lbl);
        ++kept;
    }
    if (kept == 0) {
        // Fallback: show single point at 0 to avoid empty chart
        trendChart.values.push_back(0.0f);
        trendChart.xLabels.push_back("—");
    }
    trendChart.stroke = th.primary;
}

void StatisticsScreen::Update(App& app, float dt, Vector2 mouse) {
    enterT = Easing::Approach(enterT, 1.0f, 0.3f, dt);

    backBtn.bounds = { 28, 24, 88, 40 };
    backBtn.Update(dt, mouse);
    if (backBtn.Clicked(mouse)) {
        // Go back to the appropriate dashboard based on role
        User* u = app.CurrentUser();
        if (u && u->role == Role::TEACHER) app.Goto(ScreenId::TEACHER_DASH);
        else                                app.Goto(ScreenId::STUDENT_DASH);
    }

    // Class chips - wrap layout below header
    int sw = app.W();
    float cx = 140, cy = 36;
    bool changed = false;
    for (size_t i = 0; i < classChips.size(); ++i) {
        const std::string& lbl = classChips[i].label;
        float w = std::max(64.0f, UI::MeasureText(ThemeMgr::GetFont(), lbl.c_str(), 13.0f).x + 28.0f);
        if (cx + w > sw - 28) { cx = 140; cy += 38; }
        classChips[i].bounds = { cx, cy, w, 26 };
        cx += w + 6;
        classChips[i].Update(dt, mouse);
        if (classChips[i].Clicked(mouse)) {
            int newAc = (int)i - 1;
            if (newAc != activeClass) { activeClass = newAc; changed = true; }
        }
    }
    if (changed) {
        RebuildData(app);
        distChart.OnShow();
        subjectChart.OnShow();
        trendChart.OnShow();
    }

    distChart.Update(dt);
    subjectChart.Update(dt);
    trendChart.Update(dt);
}

void StatisticsScreen::DrawHeader(App& app) {
    (void)app;
    const Theme& th = ThemeMgr::Current();
    Font fd = ThemeMgr::GetFontDisplay();
    Font f  = ThemeMgr::GetFont();

    backBtn.Draw();
    UI::DrawTextLeft(fd, "Statistics", {136, 22}, 24.0f, th.textPrimary);
    UI::DrawTextLeft(f,  "A bird's-eye view of grades", {136, 52}, 13.0f, th.textSecondary);

    // Class chips
    for (size_t i = 0; i < classChips.size(); ++i) {
        UI::Button& b = classChips[i];
        bool active = ((int)i - 1) == activeClass;
        UI::DrawSoftShadow(b.bounds, 1.0f, 6.0f, active ? 0.10f : 0.0f);
        Color bg = active ? th.primarySoft : th.surfaceMuted;
        if (b.hoverT > 0.05f && !active) bg = ThemeMgr::Lerp(bg, th.surfaceAlt, b.hoverT);
        DrawRectangleRounded(b.bounds, 1.0f, 6, bg);
        UI::StrokeRoundedRect(b.bounds, 1.0f, 6, 1.0f, active ? th.primary : th.border);
        Color tc = active ? th.primary : th.textSecondary;
        UI::DrawTextCenter(ThemeMgr::GetFontBold(), b.label.c_str(), b.bounds, 12.0f, tc, 0.5f);
    }
}

void StatisticsScreen::DrawSummaryStrip(App& app, Rectangle r) {
    const Theme& th = ThemeMgr::Current();
    Font fd = ThemeMgr::GetFontDisplay();
    Font f  = ThemeMgr::GetFont();
    Font fb = ThemeMgr::GetFontBold();

    UI::DrawCard(r, 0.10f);

    // Compute high-level numbers
    auto& users = app.Data().Users();
    auto& grades = app.Data().Grades();

    std::set<int> studentIds;
    for (auto& u : users) if (PassesFilter(u)) studentIds.insert(u.id);

    int gradeCount = 0; int sum = 0;
    int high = 0, low = 0;
    for (auto& g : grades) {
        if (!studentIds.count(g.studentId)) continue;
        gradeCount++;
        sum += g.value;
        if (g.value >= 5) high++;
        if (g.value <= 3) low++;
    }
    float avg = gradeCount ? (float)sum / (float)gradeCount : 0.0f;

    // Tile helper
    auto tile = [&](float x, float y, float w, const char* lbl, const char* val, Color c){
        Rectangle b = { r.x + x, r.y + y, w, r.height - 24 };
        Rectangle bar = { b.x, b.y, 4, b.height };
        DrawRectangleRounded(bar, 1.0f, 4, c);
        UI::DrawTextLeft(f, lbl, {b.x + 16, b.y + 4}, 11.0f, th.textMuted, 0.8f);
        UI::DrawTextLeft(fd, val, {b.x + 16, b.y + 22}, 28.0f, th.textPrimary);
    };

    float pad = 16.0f;
    float tileW = (r.width - 5 * pad) / 4.0f;

    char b1[16]; std::snprintf(b1, sizeof(b1), "%d", (int)studentIds.size());
    char b2[16]; std::snprintf(b2, sizeof(b2), "%d", gradeCount);
    char b3[16]; std::snprintf(b3, sizeof(b3), avg > 0 ? "%.2f" : "—", avg);
    char b4[16]; std::snprintf(b4, sizeof(b4), "%d / %d", high, low);

    tile(pad,                         12, tileW, "STUDENTS",       b1, th.primary);
    tile(pad * 2 + tileW,             12, tileW, "GRADES",         b2, th.secondary);
    tile(pad * 3 + tileW * 2,         12, tileW, "AVG GRADE",      b3, th.accent);
    tile(pad * 4 + tileW * 3,         12, tileW, "HIGH / LOW",     b4, th.warning);

    // Bottom line for context
    UI::DrawTextLeft(f, "Filter: ", {r.x + 16, r.y + r.height - 18}, 11.0f, th.textMuted);
    std::string flbl = (activeClass < 0 || activeClass >= (int)classes.size())
                       ? "All programs" : classes[activeClass];
    UI::DrawTextLeft(fb, flbl.c_str(), {r.x + 64, r.y + r.height - 18}, 11.0f, th.primary);
}

void StatisticsScreen::DrawDistribution(App& app, Rectangle r) {
    (void)app;
    UI::DrawCard(r, 0.10f);
    Rectangle inner = { r.x + 16, r.y + 16, r.width - 32, r.height - 32 };
    distChart.bounds = inner;
    distChart.Draw(ThemeMgr::GetFont());
}

void StatisticsScreen::DrawSubjectAverages(App& app, Rectangle r) {
    (void)app;
    UI::DrawCard(r, 0.10f);
    Rectangle inner = { r.x + 16, r.y + 16, r.width - 32, r.height - 32 };
    subjectChart.bounds = inner;
    subjectChart.Draw(ThemeMgr::GetFont());
}

void StatisticsScreen::DrawTrend(App& app, Rectangle r) {
    (void)app;
    UI::DrawCard(r, 0.10f);
    Rectangle inner = { r.x + 16, r.y + 16, r.width - 32, r.height - 32 };
    trendChart.bounds = inner;
    trendChart.Draw(ThemeMgr::GetFont());
}

void StatisticsScreen::Draw(App& app) {
    int sw = app.W(), sh = app.H();
    DrawHeader(app);

    // Compute chip area height (chips wrap onto extra rows)
    float chipBottom = 36.0f + 26.0f;
    for (auto& b : classChips) {
        chipBottom = std::max(chipBottom, b.bounds.y + b.bounds.height);
    }
    float topY = std::max((float)kHeaderH + 8.0f, chipBottom + 12.0f);

    // Summary strip
    Rectangle summary = { 28, topY, (float)sw - 56, 96 };
    DrawSummaryStrip(app, summary);

    // 2x2-ish grid: distribution + subject averages on top, trend below full-width
    float gap = 16.0f;
    float colW = ((float)sw - 56 - gap) / 2.0f;
    float gridY = summary.y + summary.height + gap;
    float chartH = ((float)sh - gridY - 28 - gap) / 2.0f;

    Rectangle distR    = { 28,                gridY, colW, chartH };
    Rectangle subjR    = { 28 + colW + gap,   gridY, colW, chartH };
    Rectangle trendR   = { 28, gridY + chartH + gap, (float)sw - 56, chartH };

    DrawDistribution(app, distR);
    DrawSubjectAverages(app, subjR);
    DrawTrend(app, trendR);
}

#pragma once
#include "screen.h"
#include "ui.h"
#include "data_store.h"
#include <vector>
#include <string>

// =====================================================================
//  StudentDashboard
//
//  The main "student information system" screen, modeled on real
//  Bulgarian university SIS portals. Six tabs:
//
//    Оценки         (Grades)
//    Учебен план    (Curriculum)
//    График         (Schedule)
//    Изпитна сесия  (Exam session)
//    Документи      (Document requests)
//    Профил         (Profile)
//
// =====================================================================

enum class StudentTab { GRADES, CURRICULUM, SCHEDULE, EXAMS, DOCUMENTS, PROFILE };

class StudentDashboard : public Screen {
public:
    void OnEnter(App& app) override;
    void OnShow(App& app) override;
    void Update(App& app, float dt, Vector2 mouse) override;
    void Draw(App& app) override;
    const char* Name() const override { return "Student Dashboard"; }

private:
    StudentTab activeTab = StudentTab::GRADES;
    float      tabSwitchT = 1.0f;       // animates 0->1 on tab change
    float      activeUnderlineX = 0.0f; // animated underline x
    float      activeUnderlineW = 0.0f;
    float      enterT = 0.0f;

    // Header
    UI::Button logoutBtn;
    Rectangle  tabBounds[6] = {};

    // Per-tab scroll state
    float scrollY[6]      = { 0, 0, 0, 0, 0, 0 };
    float scrollTarget[6] = { 0, 0, 0, 0, 0, 0 };

    // ---- Grades tab ----
    int      gradesFilterSemester = 0;       // 0 = all
    UI::ProgressCircle gpaCircle;

    // ---- Schedule tab ----
    int scheduleSemFilter = 0;               // 0 = current sem, otherwise specific

    // ---- Exam tab ----
    int examFilter = 0;  // 0 = upcoming, 1 = past, 2 = all

    // ---- Documents tab ----
    DocKind selectedDocKind = DocKind::ENROLLMENT;
    UI::TextInput docPurpose;
    UI::Button submitDocBtn;
    bool       docSubmitted = false;

    // ---- Profile tab ----
    UI::Button profileLogoutBtn;
    int        profileLangChoice = 0;        // 0 = BG, 1 = EN
    int        profileThemeChoice = 0;       // 0 = light, 1 = dark

    // ---- Keyboard help overlay (toggled by '?') ----
    bool  showHelp = false;
    float helpT = 0.0f;

    // helpers
    void DrawTopBar(App& app, Rectangle r);
    void DrawTabStrip(App& app, Rectangle r, float dt, Vector2 mouse);
    void DrawTabGrades(App& app, Rectangle area, float dt, Vector2 mouse);
    void DrawTabCurriculum(App& app, Rectangle area, float dt, Vector2 mouse);
    void DrawTabSchedule(App& app, Rectangle area, float dt, Vector2 mouse);
    void DrawTabExams(App& app, Rectangle area, float dt, Vector2 mouse);
    void DrawTabDocuments(App& app, Rectangle area, float dt, Vector2 mouse);
    void DrawTabProfile(App& app, Rectangle area, float dt, Vector2 mouse);
};

// =====================================================================
//  StudentDetailScreen (teacher drilling into one student)
//  Same as before - kept for the teacher workflow.
// =====================================================================
class StudentDetailScreen : public Screen {
public:
    void OnEnter(App& app) override;
    void OnShow(App& app) override;
    void Update(App& app, float dt, Vector2 mouse) override;
    void Draw(App& app) override;
    const char* Name() const override { return "Student Detail"; }

private:
    UI::Button backBtn{"Back"};
    UI::Button addGradeBtn{"Add Grade"};

    UI::ProgressCircle overallCircle;
    UI::LineChart trendChart;

    UI::Modal addGradeModal;
    int agSubjectId = 0;
    int agValue = 6;
    UI::TextInput agComment;
    UI::Button agSave{"Save Grade"};
    UI::Button agCancel{"Cancel"};

    UI::Modal confirmDeleteModal;
    int pendingDeleteId = 0;
    UI::Button cdDelete{"Delete"};
    UI::Button cdCancel{"Cancel"};

    float enterT = 0.0f;
    float scrollY = 0.0f;
    float scrollTarget = 0.0f;
    int   hoverGradeId = 0;

    void DrawHeader(App& app);
    void DrawSummary(App& app, Rectangle r, float dt);
    void DrawInsights(App& app, Rectangle r);
    void DrawTrend(App& app, Rectangle r);
    void DrawSubjectsAndGrades(App& app, Rectangle area, float dt, Vector2 mouse);

    void DrawAddGradeModal(App& app, float dt, Vector2 mouse);
    void DrawConfirmDeleteModal(App& app, float dt, Vector2 mouse);

    std::vector<std::string> BuildInsights(App& app);
};

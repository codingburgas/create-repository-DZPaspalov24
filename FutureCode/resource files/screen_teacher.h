#pragma once
#include "screen.h"
#include "ui.h"

class TeacherDashboard : public Screen {
public:
    void OnEnter(App& app) override;
    void OnShow(App& app) override;
    void Update(App& app, float dt, Vector2 mouse) override;
    void Draw(App& app) override;
    const char* Name() const override { return "Teacher Dashboard"; }
private:
    enum class SortBy { NAME, AVG_DESC, AVG_ASC, CLASS };
    SortBy sortBy = SortBy::NAME;

    UI::TextInput search;
    UI::Button addStudentBtn{"Add Student"};
    UI::Button manageSubjectsBtn{"Subjects"};
    UI::Button statisticsBtn{"Statistics"};
    UI::Button logoutBtn{"Logout"};

    // Sort buttons
    UI::Button sortName{"Name"};
    UI::Button sortAvgDesc{"Avg ↓"};
    UI::Button sortAvgAsc{"Avg ↑"};
    UI::Button sortClass{"Program"};

    // Class filter chips - dynamically discovered
    std::vector<std::string> classes;
    int activeClass = -1;          // -1 = all
    std::vector<UI::Button> classChips;

    // Modals
    UI::Modal addStudentModal;
    UI::Modal subjectsModal;
    UI::Modal addGradeModal;
    UI::TextInput nsName, nsUser, nsPass, nsClass;
    UI::Button    nsSave{"Save"};
    UI::Button    nsCancel{"Cancel"};
    std::string   nsError;

    UI::TextInput newSubjName;
    UI::Button    newSubjAdd{"Add Subject"};
    int           pickedColor = 0;
    std::vector<int> subjectDeleteIds; // track for hover-button bookkeeping

    // Add grade modal state
    int agStudentId = 0;
    int agSubjectId = 0;
    int agValue = 6;
    UI::TextInput agComment;
    UI::Button    agSave{"Save Grade"};
    UI::Button    agCancel{"Cancel"};

    // Per-row card animation - we just track scroll
    float scrollY = 0.0f;
    float scrollTarget = 0.0f;

    float enterT = 0.0f;

    void DrawHeader(App& app);
    void DrawToolbar(App& app, Rectangle r, float dt, Vector2 mouse);
    void DrawStudentList(App& app, Rectangle area, float dt, Vector2 mouse);

    void DrawAddStudentModal(App& app, float dt, Vector2 mouse);
    void DrawSubjectsModal(App& app, float dt, Vector2 mouse);
    void DrawAddGradeModal(App& app, float dt, Vector2 mouse);

    // Helpers
    std::vector<int> ComputeFilteredSorted(App& app);
    void RefreshClasses(App& app);
};

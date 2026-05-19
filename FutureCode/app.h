#pragma once
#include "raylib.h"
#include "screen.h"
#include "data_store.h"
#include "ui.h"
#include <memory>
#include <unordered_map>

// Top-level controller. Owns DataStore + all screens. Handles transitions and
// global UI overlays (background particles, toasts, theme toggle).

class App {
public:
    App();
    ~App();

    void Init();
    void Shutdown();
    void Update();
    void Draw();
    bool IsRunning() const { return running; }
    void Quit()            { running = false; }

    // ---- nav
    void Goto(ScreenId id);                 // animated transition
    void GotoImmediate(ScreenId id);        // no transition (used at boot)
    ScreenId CurrentId() const { return currentId; }

    // ---- session
    void SetCurrentUser(int id) { currentUserId = id; }
    int  CurrentUserId() const  { return currentUserId; }
    User* CurrentUser();
    void Logout();

    // ---- shared UI hooks
    void Toast(const std::string& text, Color accent);
    DataStore& Data() { return data; }

    // For student-detail screen
    void SetSelectedStudent(int id) { selectedStudentId = id; }
    int  SelectedStudent() const    { return selectedStudentId; }

    // Window size shortcut
    int W() const { return GetScreenWidth(); }
    int H() const { return GetScreenHeight(); }

private:
    bool running = true;

    DataStore data;
    UI::BackgroundParticles particles;
    UI::ToastStack toasts;

    int currentUserId = 0;
    int selectedStudentId = 0;

    ScreenId currentId = ScreenId::LOGIN;
    ScreenId pendingId = ScreenId::LOGIN;
    bool transitioning = false;
    float transitionT = 0.0f;        // 0..1 (mid = 0.5 swap)
    bool transitionSwapped = false;  // did we already swap currentId?

    std::unordered_map<int, std::unique_ptr<Screen>> screens;
    Screen* GetScreen(ScreenId id);

    // Top-right global header (theme toggle, user info, notifications)
    UI::Toggle themeToggle;
    bool       notifPanelOpen = false;
    float      notifPanelT = 0.0f;
    Rectangle  notifBellBounds = {0,0,40,40};
    void DrawHeader(float dt);
    void UpdateHeader(float dt, Vector2 mouse);

    void BuildScreens();
};

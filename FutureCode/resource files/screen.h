#pragma once
#include "raylib.h"
#include <string>

class App;
class DataStore;

// Base class for every full-screen view. App owns lifecycle and transitions.
class Screen {
public:
    virtual ~Screen() = default;

    // Called once when the screen is first activated.
    virtual void OnEnter(App& app) { (void)app; }
    // Called whenever the screen becomes visible again (e.g. after coming back
    // from a different screen). animT for transition is owned by App.
    virtual void OnShow(App& app)  { (void)app; }
    virtual void OnHide(App& app)  { (void)app; }

    // dt = delta time in seconds. mouse = current mouse position.
    virtual void Update(App& app, float dt, Vector2 mouse) = 0;
    virtual void Draw(App& app) = 0;

    virtual const char* Name() const { return "Screen"; }
};

// Identifiers for navigation.
enum class ScreenId {
    LOGIN, SIGNUP, TEACHER_DASH, STUDENT_DASH,
    STUDENT_DETAIL, STATISTICS
};

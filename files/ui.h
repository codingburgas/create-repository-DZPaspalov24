#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <functional>

// All reusable UI widgets and primitive draw helpers go here. Animations are
// owned by the widget instances - call Update() with dt before Draw().

namespace UI {

// raylib API compatibility shim.
//
// raylib 5.5 split the rounded-outline draw call into two functions:
//   - DrawRectangleRoundedLines(rec, roundness, segments, color)            (4 args, no thickness)
//   - DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, c)   (5 args, with thickness)
//
// Older raylib (<= 5.4) had a single 5-arg DrawRectangleRoundedLines that
// included lineThick. We always want the 5-arg form, so this wrapper picks
// the right symbol based on the raylib version macros.
inline void StrokeRoundedRect(Rectangle r, float roundness, int segments, float thickness, Color c) {
#if defined(RAYLIB_VERSION_MAJOR) && defined(RAYLIB_VERSION_MINOR) && \
    ((RAYLIB_VERSION_MAJOR > 5) || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5))
    DrawRectangleRoundedLinesEx(r, roundness, segments, thickness, c);
#else
    DrawRectangleRoundedLines(r, roundness, segments, thickness, c);
#endif
}

// ------------------ Primitives ------------------

void DrawSoftShadow(Rectangle r, float roundness, float blur = 18.0f, float opacity = 0.12f);
void DrawCard(Rectangle r, float roundness = 0.18f, bool hovered = false);
void DrawRoundedRectFill(Rectangle r, float roundness, Color c);
void DrawRoundedRectBorder(Rectangle r, float roundness, float thickness, Color c);

// Vertical gradient fill (no roundness).
void DrawVerticalGradient(Rectangle r, Color top, Color bottom);
// Vertical gradient with rounded corners (slow-ish - uses many strips).
void DrawRoundedVerticalGradient(Rectangle r, float roundness, Color top, Color bottom);

void DrawTextLeft(const Font& f, const char* text, Vector2 pos, float size, Color c, float spacing = 1.0f);
void DrawTextCenter(const Font& f, const char* text, Rectangle bounds, float size, Color c, float spacing = 1.0f);
void DrawTextRight(const Font& f, const char* text, Rectangle bounds, float size, Color c, float spacing = 1.0f);
Vector2 MeasureText(const Font& f, const char* text, float size, float spacing = 1.0f);

// Truncate text with "..." to fit within a width.
std::string Ellipsize(const Font& f, const std::string& s, float maxWidth, float size, float spacing = 1.0f);

// Subject palette (used by colorIndex).
Color SubjectColor(int colorIndex);
int   SubjectColorCount();

// ------------------ Vector icons ------------------
//
// All icons are drawn as line segments / shapes inside a square at (cx,cy)
// with the given half-size `s`. Color and stroke thickness are explicit
// so callers can tint per-state.

void IconCheck(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconX(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconBell(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconBook(Vector2 c, float s, Color color);
void IconCalendar(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconClock(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconDocument(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconUser(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconChevronDown(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconChevronRight(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconChevronUp(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconArrowUp(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconArrowDown(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconSearch(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconLogout(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconMenu(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconStar(Vector2 c, float s, Color color, bool filled = true);
void IconCog(Vector2 c, float s, Color color, float thickness = 2.0f);
void IconGoogleG(Vector2 c, float s); // multi-color Google G

// Big university-style crest (used as login logo). Draws a filled rounded
// shield with the inner monogram in `inkColor`.
void DrawUniversityCrest(Vector2 center, float size, Color shieldFill, Color inkColor);

// Draw a tiny sparkline inside the given rectangle. Auto-scales to value
// range. Optional `target` value drawn as a soft horizontal reference line.
void DrawSparkline(Rectangle r, const std::vector<float>& values,
                   Color stroke, float thickness = 1.5f,
                   bool fillUnder = true, float minY = 0.0f, float maxY = 0.0f);

// Mini calendar widget. Highlights the given list of "marked" YMD dates.
// Returns the cell rectangle the user clicked, or {0,0,0,0} if no click.
struct CalendarMark {
    int year, month, day;
    Color color;
};
Rectangle DrawMiniCalendar(Rectangle r, int viewYear, int viewMonth,
                           int todayY, int todayM, int todayD,
                           const std::vector<CalendarMark>& marks,
                           int hoverDay = -1);

// ------------------ Button ------------------

class Button {
public:
    enum Kind { PRIMARY, SECONDARY, GHOST, DANGER };

    std::string label;
    Rectangle   bounds = {0,0,120,44};
    Kind        kind = PRIMARY;
    bool        enabled = true;
    bool        full = false;        // visually fills (icon-only or wide)
    char        iconChar = 0;        // optional 1-character "icon" prefix

    // Internal animation state
    float hoverT = 0.0f;
    float pressT = 0.0f;
    float scale  = 1.0f;
    bool  hovered = false;
    bool  pressed = false;

    void Update(float dt, Vector2 mouse);
    bool Clicked(Vector2 mouse);    // call after Update on a mouse-released frame
    void Draw() const;
};

// ------------------ Text Input ------------------

class TextInput {
public:
    Rectangle bounds = {0,0,280,44};
    std::string value;
    std::string placeholder;
    bool        password = false;
    int         maxLength = 64;
    bool        focused = false;
    float       focusT = 0.0f;
    float       cursorBlink = 0.0f;

    // Optional: validate live; if returns non-empty, error is shown beneath.
    std::function<std::string(const std::string&)> validator;

    void Update(float dt, Vector2 mouse, bool clickConsumed = false);
    void Draw(const std::string& errorOverride = "") const;
    void Focus();
    void Blur();
};

// ------------------ Toggle Switch ------------------

class Toggle {
public:
    Rectangle bounds = {0,0,52,28};
    bool      value = false;
    float     animT = 0.0f;
    float     hoverT = 0.0f;
    bool      hovered = false;

    void Update(float dt, Vector2 mouse);
    bool Clicked(Vector2 mouse);
    void Draw() const;
};

// ------------------ Animated Progress (bar + circle) ------------------

class ProgressBar {
public:
    Rectangle bounds = {0,0,200,8};
    float     value = 0.0f;     // target 0..1
    float     animValue = 0.0f; // animated
    Color     fill = {123, 97, 255, 255}; // explicit color
    bool      colorByValue = false; // use grade palette mapping (value 0..1 -> grade 2..6)

    void SetTarget(float v);    // animates toward this
    void Update(float dt);
    void Draw() const;
};

class ProgressCircle {
public:
    Vector2 center = {0,0};
    float   radius = 36.0f;
    float   thickness = 8.0f;
    float   value = 0.0f;       // 0..1
    float   animValue = 0.0f;
    Color   color = {123, 97, 255, 255};   // overwritten when colorByValue is true
    bool    colorByValue = false;
    std::string label;          // big number drawn in center

    void SetTarget(float v);
    void Update(float dt);
    void Draw(const Font& f) const;
};

// ------------------ Modal ------------------

class Modal {
public:
    bool      open = false;
    float     animT = 0.0f;     // 0 closed, 1 open
    Rectangle bounds = {0,0,520,460}; // content size

    void Show();
    void Hide();
    void Update(float dt);

    // Begin/End rendering: BeginFrame draws the dim overlay and animated card
    // and returns the actual content rectangle (after scaling). End just resets
    // a few internal flags.
    bool   IsVisible() const { return animT > 0.001f || open; }
    Rectangle BeginFrame();    // draws dim + scaled card, returns content area rect (with padding)
    void   EndFrame();
};

// ------------------ Toast notification ------------------

struct Toast {
    std::string text;
    float       life = 4.0f;       // seconds remaining
    float       fadeIn = 0.0f;     // 0..1 grow in
    Color       accent = {123, 97, 255, 255};
};

class ToastStack {
public:
    void Push(const std::string& text, Color accent);
    void Update(float dt);
    void Draw(const Font& f, float screenW, float screenH);
private:
    std::vector<Toast> toasts;
};

// ------------------ Animated Bar Chart ------------------

struct BarChartItem {
    std::string label;
    float       value = 0.0f;       // raw numeric value
    Color       color = {0,0,0,255};
};

class BarChart {
public:
    std::vector<BarChartItem> items;
    Rectangle bounds = {0,0,500,260};
    float     animT = 0.0f;        // 0 -> 1 over OnShow
    float     maxValue = 6.0f;     // y-axis maximum
    std::string title;
    bool      showValues = true;

    void OnShow();                 // resets animation
    void Update(float dt);
    void Draw(const Font& f) const;
};

// ------------------ Animated Line Chart ------------------

class LineChart {
public:
    std::vector<float> values;     // y values
    std::vector<std::string> xLabels;
    Rectangle bounds = {0,0,500,260};
    float animT = 0.0f;
    float minY = 2.0f, maxY = 6.0f;
    std::string title;
    Color stroke = {123, 97, 255, 255};

    void OnShow();
    void Update(float dt);
    void Draw(const Font& f) const;
};

// ------------------ Particles for animated background ------------------

class BackgroundParticles {
public:
    struct Blob {
        Vector2 pos = {0, 0};
        Vector2 vel = {0, 0};
        float   radius = 0.0f;
        Color   color = {0, 0, 0, 0};
    };
    std::vector<Blob> blobs;

    void Init(int count, int screenW, int screenH);
    void Update(float dt, int screenW, int screenH);
    void Draw(int screenW, int screenH) const;
};

} // namespace UI

#include "ui.h"
#include "theme.h"
#include "easing.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <ctime>

namespace UI {

// ============================================================================
//  Primitives
// ============================================================================

static Color FadeColor(Color c, float a) {
    Color o = c; o.a = (unsigned char)(c.a * Easing::Clamp01(a)); return o;
}

void DrawSoftShadow(Rectangle r, float roundness, float blur, float opacity) {
    // Layered fake-blur shadow. Cheap to draw and looks acceptable.
    Color base = ThemeMgr::Current().shadow;
    int layers = 6;
    for (int i = 0; i < layers; ++i) {
        float t  = (float)i / (float)(layers - 1);
        float sp = blur * (0.4f + 0.6f * t);
        Rectangle e = { r.x - sp, r.y - sp + blur * 0.35f,
                        r.width + 2 * sp, r.height + 2 * sp };
        Color c = base; c.a = (unsigned char)(255.0f * opacity * (1.0f - t) * 0.6f);
        DrawRectangleRounded(e, roundness, 12, c);
    }
}

void DrawRoundedRectFill(Rectangle r, float roundness, Color c) {
    DrawRectangleRounded(r, roundness, 12, c);
}

void DrawRoundedRectBorder(Rectangle r, float roundness, float thickness, Color c) {
    UI::StrokeRoundedRect(r, roundness, 12, thickness, c);
}

void DrawCard(Rectangle r, float roundness, bool hovered) {
    const Theme& t = ThemeMgr::Current();
    DrawSoftShadow(r, roundness, hovered ? 22.0f : 16.0f, hovered ? 0.16f : 0.10f);
    Color body = hovered ? t.surfaceAlt : t.surface;
    DrawRectangleRounded(r, roundness, 12, body);
}

void DrawVerticalGradient(Rectangle r, Color top, Color bottom) {
    DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)r.height, top, bottom);
}

void DrawRoundedVerticalGradient(Rectangle r, float roundness, Color top, Color bottom) {
    // Approximate by stacking thin gradient strips inside a clipping mask shape.
    // Cheap approach: stack many short DrawRectangleGradientV bands inside a
    // rounded-rect outline. The corners alpha-blend with the background since we
    // first draw the rounded rect base.
    DrawRectangleRounded(r, roundness, 12, top); // base
    int bands = 28;
    for (int i = 0; i < bands; ++i) {
        float t1 = (float)i / (float)bands;
        float t2 = (float)(i + 1) / (float)bands;
        Color c1 = ThemeMgr::Lerp(top, bottom, t1);
        Color c2 = ThemeMgr::Lerp(top, bottom, t2);
        Rectangle band = {
            r.x + 1.5f, r.y + r.height * t1,
            r.width - 3.0f, r.height * (t2 - t1) + 1.0f
        };
        // Soften horizontal edges so we don't paint over the rounded corner pixels
        // too aggressively. (Still leaves corners with the base color.)
        DrawRectangleGradientV((int)band.x, (int)band.y, (int)band.width, (int)band.height, c1, c2);
    }
}

void DrawTextLeft(const Font& f, const char* text, Vector2 pos, float size, Color c, float spacing) {
    DrawTextEx(f, text, pos, size, spacing, c);
}

void DrawTextCenter(const Font& f, const char* text, Rectangle bounds, float size, Color c, float spacing) {
    Vector2 ts = ::MeasureTextEx(f, text, size, spacing);
    Vector2 pos = { bounds.x + (bounds.width - ts.x) * 0.5f,
                    bounds.y + (bounds.height - ts.y) * 0.5f };
    DrawTextEx(f, text, pos, size, spacing, c);
}

void DrawTextRight(const Font& f, const char* text, Rectangle bounds, float size, Color c, float spacing) {
    Vector2 ts = ::MeasureTextEx(f, text, size, spacing);
    Vector2 pos = { bounds.x + bounds.width - ts.x,
                    bounds.y + (bounds.height - ts.y) * 0.5f };
    DrawTextEx(f, text, pos, size, spacing, c);
}

Vector2 MeasureText(const Font& f, const char* text, float size, float spacing) {
    return ::MeasureTextEx(f, text, size, spacing);
}

std::string Ellipsize(const Font& f, const std::string& s, float maxWidth, float size, float spacing) {
    if (s.empty()) return s;
    if (::MeasureTextEx(f, s.c_str(), size, spacing).x <= maxWidth) return s;
    std::string out = s;
    while (out.size() > 1) {
        out.pop_back();
        std::string trial = out + "...";
        if (::MeasureTextEx(f, trial.c_str(), size, spacing).x <= maxWidth) return trial;
    }
    return "...";
}

static const Color kSubjectPalette[] = {
    {123, 97, 255, 255},  //  0 violet
    { 56,189,168, 255},   //  1 mint
    {236,121,178, 255},   //  2 pink
    { 80,168,232, 255},   //  3 sky blue
    {255,168, 80, 255},   //  4 amber
    {110,200,120, 255},   //  5 grass
    {235, 87, 98, 255},   //  6 coral
    {120,150,210, 255},   //  7 cornflower
};
Color SubjectColor(int colorIndex) {
    int n = (int)(sizeof(kSubjectPalette) / sizeof(Color));
    int i = ((colorIndex % n) + n) % n;
    return kSubjectPalette[i];
}
int SubjectColorCount() { return (int)(sizeof(kSubjectPalette) / sizeof(Color)); }

// ============================================================================
//  Vector icons
//
//  All drawn from line segments / primitives so they scale crisply at any
//  zoom and tint to any color without needing image assets.
// ============================================================================

void IconCheck(Vector2 c, float s, Color color, float thickness) {
    // Two-segment polyline forming a check: down-stroke then up-stroke.
    // Coordinates expressed as fractions of half-size s.
    Vector2 a = { c.x - s * 0.55f, c.y + s * 0.05f };
    Vector2 b = { c.x - s * 0.10f, c.y + s * 0.50f };
    Vector2 d = { c.x + s * 0.60f, c.y - s * 0.40f };
    DrawLineEx(a, b, thickness, color);
    DrawLineEx(b, d, thickness, color);
    // Round the joint
    DrawCircleV(b, thickness * 0.5f, color);
    DrawCircleV(a, thickness * 0.5f, color);
    DrawCircleV(d, thickness * 0.5f, color);
}

void IconX(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x - s * 0.5f, c.y - s * 0.5f},
               {c.x + s * 0.5f, c.y + s * 0.5f}, thickness, color);
    DrawLineEx({c.x - s * 0.5f, c.y + s * 0.5f},
               {c.x + s * 0.5f, c.y - s * 0.5f}, thickness, color);
}

void IconBell(Vector2 c, float s, Color color, float thickness) {
    // Bell body: rounded trapezoid drawn as a polygon
    float w = s * 0.95f, h = s * 0.85f;
    // Top dome
    DrawCircleSector({c.x, c.y - h * 0.30f}, w * 0.65f, 180.0f, 360.0f, 12, color);
    // Body
    Rectangle body = { c.x - w * 0.65f, c.y - h * 0.30f, w * 1.30f, h * 0.55f };
    DrawRectangleRec(body, color);
    // Bottom rim
    DrawRectangle((int)(c.x - w * 0.85f), (int)(c.y + h * 0.22f),
                  (int)(w * 1.70f), (int)(thickness + 1), color);
    // Clapper
    DrawCircleV({c.x, c.y + h * 0.42f}, thickness * 1.4f, color);
    // Top knob
    DrawCircleV({c.x, c.y - h * 0.62f}, thickness * 1.2f, color);
}

void IconBook(Vector2 c, float s, Color color) {
    // Open book - two filled trapezoids with a center spine
    float w = s * 0.95f, h = s * 0.75f;
    Rectangle l = { c.x - w, c.y - h * 0.5f, w - 1, h };
    Rectangle r = { c.x + 1, c.y - h * 0.5f, w - 1, h };
    DrawRectangleRec(l, color);
    DrawRectangleRec(r, color);
    // Inset white "pages" lines
    Color page = ColorAlpha(WHITE, 0.45f);
    for (int i = 0; i < 3; ++i) {
        float ly = l.y + 8 + i * 6;
        DrawLineEx({l.x + 6, ly}, {l.x + l.width - 6, ly}, 1.0f, page);
        DrawLineEx({r.x + 6, ly}, {r.x + r.width - 6, ly}, 1.0f, page);
    }
}

void IconCalendar(Vector2 c, float s, Color color, float thickness) {
    Rectangle r = { c.x - s * 0.65f, c.y - s * 0.5f, s * 1.30f, s * 1.0f };
    StrokeRoundedRect(r, 0.18f, 8, thickness, color);
    // Top binding bar
    DrawRectangle((int)r.x, (int)(r.y + s * 0.20f),
                  (int)r.width, (int)thickness, color);
    // Two binding posts
    DrawLineEx({r.x + r.width * 0.27f, r.y - s * 0.10f},
               {r.x + r.width * 0.27f, r.y + s * 0.12f}, thickness, color);
    DrawLineEx({r.x + r.width * 0.73f, r.y - s * 0.10f},
               {r.x + r.width * 0.73f, r.y + s * 0.12f}, thickness, color);
    // Single date dot
    DrawCircleV({c.x, c.y + s * 0.15f}, thickness * 1.3f, color);
}

void IconClock(Vector2 c, float s, Color color, float thickness) {
    DrawCircleLines((int)c.x, (int)c.y, s * 0.65f, color);
    DrawCircleLines((int)c.x, (int)c.y, s * 0.65f - 1, color);
    // Hour hand pointing up
    DrawLineEx(c, { c.x, c.y - s * 0.40f }, thickness, color);
    // Minute hand pointing right
    DrawLineEx(c, { c.x + s * 0.50f, c.y }, thickness, color);
}

void IconDocument(Vector2 c, float s, Color color, float thickness) {
    Rectangle r = { c.x - s * 0.55f, c.y - s * 0.65f, s * 1.10f, s * 1.30f };
    StrokeRoundedRect(r, 0.10f, 6, thickness, color);
    // Folded corner
    Vector2 fa = { r.x + r.width - s * 0.30f, r.y };
    Vector2 fb = { r.x + r.width, r.y + s * 0.30f };
    DrawLineEx(fa, fb, thickness, color);
    DrawLineEx(fa, { fa.x, fb.y }, thickness, color);
    DrawLineEx({ fa.x, fb.y }, fb, thickness, color);
    // Three text lines
    for (int i = 0; i < 3; ++i) {
        float ly = r.y + s * (0.55f + i * 0.22f);
        DrawLineEx({ r.x + s * 0.18f, ly }, { r.x + r.width - s * 0.18f, ly },
                   thickness * 0.8f, color);
    }
}

void IconUser(Vector2 c, float s, Color color, float thickness) {
    // Head circle
    DrawCircleV({ c.x, c.y - s * 0.30f }, s * 0.32f, color);
    // Shoulders / torso (filled rounded shape)
    Rectangle torso = { c.x - s * 0.55f, c.y + s * 0.10f, s * 1.10f, s * 0.55f };
    DrawRectangleRounded(torso, 0.6f, 8, color);
    (void)thickness;
}

void IconChevronDown(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x - s * 0.5f, c.y - s * 0.18f},
               {c.x,             c.y + s * 0.30f}, thickness, color);
    DrawLineEx({c.x + s * 0.5f, c.y - s * 0.18f},
               {c.x,             c.y + s * 0.30f}, thickness, color);
}
void IconChevronUp(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x - s * 0.5f, c.y + s * 0.18f},
               {c.x,             c.y - s * 0.30f}, thickness, color);
    DrawLineEx({c.x + s * 0.5f, c.y + s * 0.18f},
               {c.x,             c.y - s * 0.30f}, thickness, color);
}
void IconChevronRight(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x - s * 0.18f, c.y - s * 0.5f},
               {c.x + s * 0.30f, c.y            }, thickness, color);
    DrawLineEx({c.x - s * 0.18f, c.y + s * 0.5f},
               {c.x + s * 0.30f, c.y            }, thickness, color);
}

void IconArrowUp(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x, c.y + s * 0.55f}, {c.x, c.y - s * 0.55f}, thickness, color);
    DrawLineEx({c.x - s * 0.30f, c.y - s * 0.20f}, {c.x, c.y - s * 0.55f}, thickness, color);
    DrawLineEx({c.x + s * 0.30f, c.y - s * 0.20f}, {c.x, c.y - s * 0.55f}, thickness, color);
}
void IconArrowDown(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x, c.y - s * 0.55f}, {c.x, c.y + s * 0.55f}, thickness, color);
    DrawLineEx({c.x - s * 0.30f, c.y + s * 0.20f}, {c.x, c.y + s * 0.55f}, thickness, color);
    DrawLineEx({c.x + s * 0.30f, c.y + s * 0.20f}, {c.x, c.y + s * 0.55f}, thickness, color);
}

void IconSearch(Vector2 c, float s, Color color, float thickness) {
    Vector2 ringC = { c.x - s * 0.10f, c.y - s * 0.10f };
    float r = s * 0.40f;
    DrawCircleLines((int)ringC.x, (int)ringC.y, r, color);
    DrawCircleLines((int)ringC.x, (int)ringC.y, r - 1, color);
    // Handle
    Vector2 ha = { ringC.x + r * 0.71f, ringC.y + r * 0.71f };
    Vector2 hb = { c.x + s * 0.50f, c.y + s * 0.50f };
    DrawLineEx(ha, hb, thickness, color);
}

void IconLogout(Vector2 c, float s, Color color, float thickness) {
    // Door frame (left side)
    Rectangle frame = { c.x - s * 0.65f, c.y - s * 0.55f, s * 0.55f, s * 1.10f };
    StrokeRoundedRect(frame, 0.15f, 4, thickness, color);
    // Arrow exiting right
    DrawLineEx({c.x - s * 0.10f, c.y}, {c.x + s * 0.65f, c.y}, thickness, color);
    DrawLineEx({c.x + s * 0.30f, c.y - s * 0.30f}, {c.x + s * 0.65f, c.y}, thickness, color);
    DrawLineEx({c.x + s * 0.30f, c.y + s * 0.30f}, {c.x + s * 0.65f, c.y}, thickness, color);
}

void IconMenu(Vector2 c, float s, Color color, float thickness) {
    DrawLineEx({c.x - s * 0.55f, c.y - s * 0.35f},
               {c.x + s * 0.55f, c.y - s * 0.35f}, thickness, color);
    DrawLineEx({c.x - s * 0.55f, c.y},
               {c.x + s * 0.55f, c.y}, thickness, color);
    DrawLineEx({c.x - s * 0.55f, c.y + s * 0.35f},
               {c.x + s * 0.55f, c.y + s * 0.35f}, thickness, color);
}

void IconStar(Vector2 c, float s, Color color, bool filled) {
    // 5-pointed star using a triangle fan
    const int N = 10;
    Vector2 pts[N];
    for (int i = 0; i < N; ++i) {
        float ang = -PI / 2 + i * (PI * 2 / N);
        float r = (i % 2 == 0) ? s * 0.95f : s * 0.42f;
        pts[i] = { c.x + std::cos(ang) * r, c.y + std::sin(ang) * r };
    }
    if (filled) {
        for (int i = 0; i < N; ++i) {
            DrawTriangle(c, pts[i], pts[(i + 1) % N], color);
        }
    } else {
        for (int i = 0; i < N; ++i) {
            DrawLineEx(pts[i], pts[(i + 1) % N], 1.5f, color);
        }
    }
}

void IconCog(Vector2 c, float s, Color color, float thickness) {
    // Outer teeth approximated by 8 rectangles
    for (int i = 0; i < 8; ++i) {
        float ang = i * (PI * 2 / 8);
        Vector2 p = { c.x + std::cos(ang) * s * 0.55f,
                      c.y + std::sin(ang) * s * 0.55f };
        DrawCircleV(p, thickness * 1.3f, color);
    }
    // Outer ring
    DrawCircleLines((int)c.x, (int)c.y, s * 0.55f, color);
    // Inner hole
    DrawCircleLines((int)c.x, (int)c.y, s * 0.22f, color);
    DrawCircleLines((int)c.x, (int)c.y, s * 0.22f - 1, color);
}

void IconGoogleG(Vector2 c, float s) {
    // Stylized Google G mark - four colored arcs around a hole
    // Approximated using DrawCircleSector with different colors per quadrant
    float r = s * 0.85f;
    float thick = s * 0.30f;
    // Each "sector" is the ring between r-thick and r, drawn with DrawRing
    DrawRing(c, r - thick, r, 270, 360,  18, {66, 133, 244, 255});  // blue
    DrawRing(c, r - thick, r, 0,   90,   18, {234, 67, 53, 255});   // red
    DrawRing(c, r - thick, r, 90,  180,  18, {251, 188, 4, 255});   // yellow
    DrawRing(c, r - thick, r, 180, 270,  18, {52, 168, 83, 255});   // green
    // Inner G crossbar (a small horizontal bar)
    Rectangle bar = { c.x, c.y - thick * 0.20f, r * 0.85f, thick * 0.55f };
    DrawRectangleRec(bar, {66, 133, 244, 255});
}

// =====================================================================
//  University crest — a two-tone shield with a central monogram.
//
//  Drawn from primitives so it tints with the active theme. Used as
//  the login-screen logo.
// =====================================================================
void DrawUniversityCrest(Vector2 center, float size, Color shieldFill, Color inkColor) {
    // Shield silhouette: rounded rectangle on top, pointed bottom.
    float w = size * 1.2f, h = size * 1.5f;
    // Top rounded body
    Rectangle topBody = {
        center.x - w * 0.5f,
        center.y - h * 0.55f,
        w,
        h * 0.78f
    };
    DrawRectangleRounded(topBody, 0.20f, 12, shieldFill);
    // Pointed bottom: triangle
    DrawTriangle(
        { center.x - w * 0.5f, center.y + h * 0.20f },
        { center.x,             center.y + h * 0.55f },
        { center.x + w * 0.5f, center.y + h * 0.20f },
        shieldFill
    );

    // Inner divider line
    DrawLineEx(
        { center.x - w * 0.42f, center.y },
        { center.x + w * 0.42f, center.y },
        1.5f, ColorAlpha(inkColor, 0.4f)
    );

    // Two-letter monogram: "FC" (FutureCode)
    Font fb = ThemeMgr::GetFontBold();
    float fsTop = size * 0.42f;
    Vector2 mB = ::MeasureTextEx(fb, "FC", fsTop, 1.0f);
    DrawTextEx(fb, "FC",
               { center.x - mB.x * 0.5f, center.y - h * 0.32f - mB.y * 0.5f + mB.y * 0.1f },
               fsTop, 1.0f, inkColor);
    // Year
    float fsBottom = size * 0.18f;
    Vector2 mY = ::MeasureTextEx(fb, "2025", fsBottom, 1.0f);
    DrawTextEx(fb, "2025",
               { center.x - mY.x * 0.5f, center.y + h * 0.10f },
               fsBottom, 1.0f, inkColor);
}

// =====================================================================
//  Sparkline
// =====================================================================
void DrawSparkline(Rectangle r, const std::vector<float>& values,
                   Color stroke, float thickness,
                   bool fillUnder, float minY, float maxY) {
    if (values.size() < 2) return;
    // Auto-scale if min/max are equal (i.e. caller didn't set them)
    if (minY >= maxY) {
        float lo = values[0], hi = values[0];
        for (float v : values) { if (v < lo) lo = v; if (v > hi) hi = v; }
        // Pad by 5%
        float pad = (hi - lo) * 0.10f + 0.5f;
        minY = lo - pad; maxY = hi + pad;
    }
    float range = maxY - minY;
    if (range < 0.001f) range = 1.0f;

    auto pointAt = [&](size_t i) -> Vector2 {
        float xt = (float)i / (float)(values.size() - 1);
        float x  = r.x + xt * r.width;
        float yt = (values[i] - minY) / range;
        float y  = r.y + r.height - yt * r.height;
        return { x, y };
    };

    // Filled area underneath
    if (fillUnder) {
        Color fill = stroke; fill.a = (unsigned char)(stroke.a * 0.18f);
        for (size_t i = 1; i < values.size(); ++i) {
            Vector2 p1 = pointAt(i - 1);
            Vector2 p2 = pointAt(i);
            Vector2 b1 = { p1.x, r.y + r.height };
            Vector2 b2 = { p2.x, r.y + r.height };
            DrawTriangle(p1, b1, b2, fill);
            DrawTriangle(p1, b2, p2, fill);
        }
    }
    // Polyline
    for (size_t i = 1; i < values.size(); ++i) {
        DrawLineEx(pointAt(i - 1), pointAt(i), thickness, stroke);
    }
    // End-cap dot
    Vector2 last = pointAt(values.size() - 1);
    DrawCircleV(last, thickness * 1.6f, stroke);
}

// =====================================================================
//  Mini calendar
//
//  Compact month grid: 7 columns (Mon-Sun), up to 6 rows. Today is drawn
//  with a filled circle, "marked" dates as small color dots underneath.
// =====================================================================
static int kDaysInMonth(int y, int m) {
    static const int dim[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (m < 1 || m > 12) return 30;
    int d = dim[m - 1];
    if (m == 2) {
        bool leap = ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
        if (leap) d = 29;
    }
    return d;
}

Rectangle DrawMiniCalendar(Rectangle r, int viewYear, int viewMonth,
                           int todayY, int todayM, int todayD,
                           const std::vector<CalendarMark>& marks,
                           int hoverDay) {
    const Theme& th = ThemeMgr::Current();
    Font fb = ThemeMgr::GetFontBold();
    Font f  = ThemeMgr::GetFont();

    // Header: month + year
    char hdr[32];
    static const char* monthNamesBG[] = { "", "Януари","Февруари","Март","Април","Май","Юни",
                                           "Юли","Август","Септември","Октомври","Ноември","Декември" };
    std::snprintf(hdr, sizeof(hdr), "%s %d", monthNamesBG[viewMonth], viewYear);
    Vector2 hm = ::MeasureTextEx(fb, hdr, 14.0f, 1.0f);
    DrawTextEx(fb, hdr, { r.x + r.width / 2 - hm.x / 2, r.y + 8 }, 14.0f, 1.0f, th.textPrimary);

    // Column headers
    static const char* dayCols[] = { "Пн","Вт","Ср","Чт","Пт","Сб","Нд" };
    float cellW = r.width / 7.0f;
    float headerY = r.y + 32;
    for (int i = 0; i < 7; ++i) {
        Rectangle c = { r.x + i * cellW, headerY, cellW, 18 };
        DrawTextEx(f, dayCols[i],
                   { c.x + cellW * 0.5f - ::MeasureTextEx(f, dayCols[i], 10.0f, 1.0f).x * 0.5f,
                     c.y + 4 },
                   10.0f, 1.0f, th.textMuted);
    }

    // Day grid
    int firstDow;
    {
        std::tm cal{};
        cal.tm_year = viewYear - 1900; cal.tm_mon = viewMonth - 1; cal.tm_mday = 1; cal.tm_hour = 12;
        std::time_t t = std::mktime(&cal);
        std::tm lt{};
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        firstDow = (lt.tm_wday + 6) % 7;  // 0=Mon
    }
    int dim = kDaysInMonth(viewYear, viewMonth);
    float rowH = (r.height - 56) / 6.0f;
    float gridY = r.y + 56;
    Rectangle clicked = { 0, 0, 0, 0 };

    for (int day = 1; day <= dim; ++day) {
        int idx = firstDow + (day - 1);
        int row = idx / 7;
        int col = idx % 7;
        Rectangle cell = { r.x + col * cellW, gridY + row * rowH, cellW, rowH };
        Rectangle inset = { cell.x + 2, cell.y + 2, cell.width - 4, cell.height - 4 };

        bool isToday = (viewYear == todayY && viewMonth == todayM && day == todayD);
        bool isHover = (day == hoverDay);

        // Today background
        if (isToday) {
            DrawRectangleRounded(inset, 0.5f, 6, th.primary);
        } else if (isHover) {
            DrawRectangleRounded(inset, 0.5f, 6, th.surfaceMuted);
        }

        char db[8]; std::snprintf(db, sizeof(db), "%d", day);
        Color dc = isToday ? WHITE : th.textPrimary;
        Vector2 dm = ::MeasureTextEx(fb, db, 11.0f, 1.0f);
        DrawTextEx(fb, db,
                   { cell.x + cell.width * 0.5f - dm.x * 0.5f,
                     cell.y + 6 },
                   11.0f, 1.0f, dc);

        // Mark dots
        std::vector<Color> markColors;
        for (const auto& mk : marks) {
            if (mk.year == viewYear && mk.month == viewMonth && mk.day == day) {
                markColors.push_back(mk.color);
            }
        }
        for (size_t i = 0; i < markColors.size() && i < 3; ++i) {
            float dx = cell.x + cell.width * 0.5f + ((float)i - (markColors.size() - 1) * 0.5f) * 5.0f;
            DrawCircleV({ dx, cell.y + cell.height - 6 }, 2.0f,
                        isToday ? WHITE : markColors[i]);
        }

        // Click + hover detection
        if (CheckCollisionPointRec(GetMousePosition(), cell)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = cell;
        }
    }
    return clicked;
}

// ============================================================================
//  Button
// ============================================================================

void Button::Update(float dt, Vector2 mouse) {
    if (!enabled) {
        hoverT = Easing::Approach(hoverT, 0.0f, 0.10f, dt);
        pressT = Easing::Approach(pressT, 0.0f, 0.10f, dt);
        scale  = Easing::Approach(scale,  1.0f, 0.10f, dt);
        hovered = pressed = false; return;
    }
    hovered = CheckCollisionPointRec(mouse, bounds);
    pressed = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    hoverT = Easing::Approach(hoverT, hovered ? 1.0f : 0.0f, 0.10f, dt);
    pressT = Easing::Approach(pressT, pressed ? 1.0f : 0.0f, 0.06f, dt);
    float targetScale = 1.0f + (hovered ? 0.025f : 0.0f) - (pressed ? 0.04f : 0.0f);
    scale = Easing::Approach(scale, targetScale, 0.08f, dt);
    if (hovered) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
}

bool Button::Clicked(Vector2 mouse) {
    return enabled && CheckCollisionPointRec(mouse, bounds) &&
           IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

void Button::Draw() const {
    const Theme& t = ThemeMgr::Current();
    // Compute scaled rect about the center
    float sw = bounds.width * scale, sh = bounds.height * scale;
    Rectangle r = {
        bounds.x + (bounds.width - sw) * 0.5f,
        bounds.y + (bounds.height - sh) * 0.5f, sw, sh
    };

    Color bg, fg, border = {0,0,0,0};
    switch (kind) {
        case PRIMARY:
            bg     = ThemeMgr::Lerp(t.primary, t.primaryHover, hoverT);
            fg     = WHITE;
            break;
        case SECONDARY:
            bg     = ThemeMgr::Lerp(t.surfaceAlt, t.primarySoft, hoverT);
            fg     = t.primary;
            border = t.border;
            break;
        case GHOST:
            bg     = FadeColor(t.primary, 0.06f * hoverT);
            fg     = ThemeMgr::Lerp(t.textSecondary, t.primary, hoverT);
            break;
        case DANGER:
            bg     = ThemeMgr::Lerp(t.error, ThemeMgr::Lerp(t.error, BLACK, 0.15f), hoverT);
            fg     = WHITE;
            break;
    }
    if (!enabled) { bg.a = (unsigned char)(bg.a * 0.4f); fg.a = (unsigned char)(fg.a * 0.55f); }

    // Subtle elevation for primary/danger
    if ((kind == PRIMARY || kind == DANGER) && enabled) {
        DrawSoftShadow(r, 0.4f, 14.0f, 0.18f + hoverT * 0.10f);
    }

    DrawRectangleRounded(r, 0.4f, 12, bg);
    if (border.a > 0) UI::StrokeRoundedRect(r, 0.4f, 12, 1.2f, border);

    // Click ripple-ish highlight - subtle
    if (pressT > 0.05f) {
        Color flash = WHITE; flash.a = (unsigned char)(40 * pressT);
        DrawRectangleRounded(r, 0.4f, 12, flash);
    }

    Font f = ThemeMgr::GetFontBold();
    float fsize = (kind == GHOST) ? 17.0f : 18.0f;
    std::string text = label;
    if (iconChar) {
        char ic[2] = { iconChar, 0 };
        text = std::string(ic) + "  " + label;
    }
    DrawTextCenter(f, text.c_str(), r, fsize, fg, 0.5f);
}

// ============================================================================
//  TextInput
// ============================================================================

void TextInput::Focus() { focused = true; cursorBlink = 0.0f; }
void TextInput::Blur()  { focused = false; }

void TextInput::Update(float dt, Vector2 mouse, bool clickConsumed) {
    bool hover = CheckCollisionPointRec(mouse, bounds);
    if (hover) SetMouseCursor(MOUSE_CURSOR_IBEAM);

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !clickConsumed) {
        focused = hover;
        if (focused) cursorBlink = 0.0f;
    }

    focusT = Easing::Approach(focusT, focused ? 1.0f : 0.0f, 0.10f, dt);
    cursorBlink += dt;

    if (focused) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if ((int)value.size() < maxLength && ch >= 32 && ch < 127) {
                value.push_back((char)ch);
                cursorBlink = 0.0f;
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (!value.empty()) value.pop_back();
            cursorBlink = 0.0f;
        }
        // Tab/Enter handling left to caller via IsKeyPressed checks
    }
}

void TextInput::Draw(const std::string& errorOverride) const {
    const Theme& t = ThemeMgr::Current();
    Color bg = t.surfaceMuted;
    Color borderC = ThemeMgr::Lerp(t.border, t.primary, focusT);
    float borderTh = 1.2f + 1.0f * focusT;

    DrawRectangleRounded(bounds, 0.35f, 10, bg);
    UI::StrokeRoundedRect(bounds, 0.35f, 10, borderTh, borderC);

    // Soft glow when focused
    if (focusT > 0.01f) {
        Rectangle g = { bounds.x - 4, bounds.y - 4, bounds.width + 8, bounds.height + 8 };
        Color glow = t.primary; glow.a = (unsigned char)(35 * focusT);
        DrawRectangleRounded(g, 0.35f, 10, glow);
    }

    Font f = ThemeMgr::GetFont();
    float fsize = 17.0f;
    float padX = 14.0f;
    float maxW = bounds.width - padX * 2;

    std::string display = value;
    if (password) display = std::string(value.size(), '*');

    if (display.empty() && !focused) {
        DrawTextLeft(f, placeholder.c_str(),
                     {bounds.x + padX, bounds.y + (bounds.height - fsize) * 0.5f},
                     fsize, t.textMuted);
    } else {
        // If too long, scroll right (keep cursor visible)
        std::string visible = display;
        Vector2 ms = ::MeasureTextEx(f, visible.c_str(), fsize, 1.0f);
        while (ms.x > maxW && visible.size() > 1) {
            visible.erase(visible.begin());
            ms = ::MeasureTextEx(f, visible.c_str(), fsize, 1.0f);
        }
        DrawTextLeft(f, visible.c_str(),
                     {bounds.x + padX, bounds.y + (bounds.height - fsize) * 0.5f},
                     fsize, t.textPrimary);
        if (focused && fmodf(cursorBlink, 1.0f) < 0.5f) {
            float cx = bounds.x + padX + ms.x + 1.0f;
            DrawLineEx({cx, bounds.y + 8}, {cx, bounds.y + bounds.height - 8}, 1.4f, t.primary);
        }
    }

    // Validation error
    std::string err = errorOverride;
    if (err.empty() && validator) err = validator(value);
    if (!err.empty()) {
        DrawTextLeft(f, err.c_str(), {bounds.x + 4, bounds.y + bounds.height + 6}, 13.0f, t.error);
    }
}

// ============================================================================
//  Toggle
// ============================================================================

void Toggle::Update(float dt, Vector2 mouse) {
    hovered = CheckCollisionPointRec(mouse, bounds);
    if (hovered) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    hoverT = Easing::Approach(hoverT, hovered ? 1.0f : 0.0f, 0.08f, dt);
    animT  = Easing::Approach(animT,  value   ? 1.0f : 0.0f, 0.10f, dt);
}

bool Toggle::Clicked(Vector2 mouse) {
    if (CheckCollisionPointRec(mouse, bounds) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        value = !value; return true;
    }
    return false;
}

void Toggle::Draw() const {
    const Theme& t = ThemeMgr::Current();
    float r = bounds.height * 0.5f;
    Color trackOff = t.borderStrong;
    Color trackOn  = t.primary;
    Color track = ThemeMgr::Lerp(trackOff, trackOn, animT);
    DrawRectangleRounded(bounds, 1.0f, 12, track);

    float knobR = r - 3.0f;
    float kx = bounds.x + r + animT * (bounds.width - 2*r);
    float ky = bounds.y + r;
    DrawCircle((int)(kx + 1), (int)(ky + 2), knobR, FadeColor(BLACK, 0.18f)); // shadow
    DrawCircle((int)kx, (int)ky, knobR, WHITE);
}

// ============================================================================
//  ProgressBar
// ============================================================================

void ProgressBar::SetTarget(float v) { value = Easing::Clamp01(v); }
void ProgressBar::Update(float dt) {
    animValue = Easing::Approach(animValue, value, 0.18f, dt);
}
void ProgressBar::Draw() const {
    const Theme& t = ThemeMgr::Current();
    DrawRectangleRounded(bounds, 1.0f, 8, t.surfaceMuted);
    Color c = fill;
    if (colorByValue) {
        float gradeEq = 2.0f + animValue * 4.0f;
        c = ThemeMgr::GradeColor(gradeEq);
    }
    Rectangle f = {bounds.x, bounds.y, bounds.width * animValue, bounds.height};
    if (f.width > 0.5f) DrawRectangleRounded(f, 1.0f, 8, c);
}

// ============================================================================
//  ProgressCircle
// ============================================================================

void ProgressCircle::SetTarget(float v) { value = Easing::Clamp01(v); }
void ProgressCircle::Update(float dt) {
    animValue = Easing::Approach(animValue, value, 0.20f, dt);
}
void ProgressCircle::Draw(const Font& f) const {
    const Theme& t = ThemeMgr::Current();
    Color c = color;
    if (colorByValue) {
        float gradeEq = 2.0f + animValue * 4.0f;
        c = ThemeMgr::GradeColor(gradeEq);
    }
    // Track
    DrawRing(center, radius - thickness, radius, 0.0f, 360.0f, 64, t.surfaceMuted);
    // Progress arc
    if (animValue > 0.001f) {
        float angle = animValue * 360.0f;
        DrawRing(center, radius - thickness, radius, -90.0f, -90.0f + angle, 64, c);
    }
    // Center label
    if (!label.empty()) {
        float lblSize = radius * 0.7f;
        Rectangle bnd = {center.x - radius, center.y - radius, radius * 2, radius * 2};
        DrawTextCenter(f, label.c_str(), bnd, lblSize, t.textPrimary, 0.5f);
    }
}

// ============================================================================
//  Modal
// ============================================================================

void Modal::Show() { open = true; }
void Modal::Hide() { open = false; }

void Modal::Update(float dt) {
    animT = Easing::Approach(animT, open ? 1.0f : 0.0f, 0.10f, dt);
}

Rectangle Modal::BeginFrame() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    // Backdrop
    Color overlay = BLACK; overlay.a = (unsigned char)(160 * Easing::OutCubic(animT));
    DrawRectangle(0, 0, sw, sh, overlay);

    // Card scaled+faded
    float ease = Easing::OutBack(animT);
    float scale = 0.92f + 0.08f * ease;
    float w = bounds.width * scale, h = bounds.height * scale;
    Rectangle card = {(sw - w) * 0.5f, (sh - h) * 0.5f, w, h};

    DrawSoftShadow(card, 0.10f, 30.0f, 0.30f);
    DrawRectangleRounded(card, 0.10f, 14, ThemeMgr::Current().surface);

    // Inner padded content rect
    float padX = 28.0f, padY = 28.0f;
    return Rectangle{ card.x + padX, card.y + padY,
                      card.width - padX * 2, card.height - padY * 2 };
}

void Modal::EndFrame() {
    // No-op for now; reserved for any post-content effects.
}

// ============================================================================
//  Toast stack
// ============================================================================

void ToastStack::Push(const std::string& text, Color accent) {
    Toast t; t.text = text; t.accent = accent;
    toasts.push_back(t);
}

void ToastStack::Update(float dt) {
    for (auto& t : toasts) {
        t.fadeIn = Easing::Approach(t.fadeIn, 1.0f, 0.10f, dt);
        t.life -= dt;
    }
    toasts.erase(std::remove_if(toasts.begin(), toasts.end(),
                                [](const Toast& t){ return t.life <= 0.0f; }),
                 toasts.end());
}

void ToastStack::Draw(const Font& f, float screenW, float screenH) {
    const Theme& th = ThemeMgr::Current();
    float w = 320.0f, h = 56.0f;
    float x = screenW - w - 24.0f;
    float y = screenH - h - 24.0f;

    for (int i = (int)toasts.size() - 1; i >= 0; --i) {
        const auto& t = toasts[i];
        float aIn   = Easing::OutBack(t.fadeIn);
        float aOut  = (t.life < 0.5f) ? Easing::OutQuad(t.life / 0.5f) : 1.0f;
        float alpha = Easing::Clamp01(aOut);
        float slide = (1.0f - aIn) * 30.0f;

        Rectangle r = { x + slide, y, w, h };

        // shadow + body
        DrawSoftShadow(r, 0.30f, 18.0f, 0.20f * alpha);
        Color body = th.surface; body.a = (unsigned char)(255 * alpha);
        DrawRectangleRounded(r, 0.30f, 12, body);

        // accent bar on left
        Rectangle bar = { r.x, r.y, 4, r.height };
        Color bc = t.accent; bc.a = (unsigned char)(255 * alpha);
        DrawRectangleRounded(bar, 1.0f, 6, bc);

        // text
        Color tc = th.textPrimary; tc.a = (unsigned char)(255 * alpha);
        DrawTextLeft(f, t.text.c_str(), {r.x + 18, r.y + (r.height - 16) * 0.5f}, 16.0f, tc);

        y -= h + 12.0f;
    }
}

// ============================================================================
//  BarChart
// ============================================================================

void BarChart::OnShow() { animT = 0.0f; }
void BarChart::Update(float dt) {
    animT = Easing::Approach(animT, 1.0f, 0.30f, dt);
}

void BarChart::Draw(const Font& f) const {
    const Theme& th = ThemeMgr::Current();
    // Title
    if (!title.empty()) {
        DrawTextLeft(ThemeMgr::GetFontBold(), title.c_str(),
                     {bounds.x, bounds.y}, 18.0f, th.textPrimary);
    }
    float topPad = title.empty() ? 8.0f : 36.0f;
    Rectangle plot = { bounds.x, bounds.y + topPad,
                       bounds.width, bounds.height - topPad - 26.0f };
    // baseline grid
    int gridLines = 4;
    for (int i = 0; i <= gridLines; ++i) {
        float gy = plot.y + plot.height * (1.0f - (float)i / (float)gridLines);
        DrawLineEx({plot.x, gy}, {plot.x + plot.width, gy}, 1.0f,
                   ColorAlpha(th.border, 0.7f));
        // y label
        char buf[16]; std::snprintf(buf, sizeof(buf), "%.1f", maxValue * (float)i / (float)gridLines);
        DrawTextLeft(f, buf, {plot.x - 26, gy - 7}, 11.0f, th.textMuted);
    }
    if (items.empty()) return;
    int n = (int)items.size();
    float gap = 14.0f;
    float bw = (plot.width - gap * (n + 1)) / (float)n;
    float ease = Easing::OutCubic(animT);
    for (int i = 0; i < n; ++i) {
        float vt = (items[i].value / maxValue) * ease;
        if (vt < 0) vt = 0; if (vt > 1) vt = 1;
        float bh = plot.height * vt;
        Rectangle bar = { plot.x + gap + i * (bw + gap), plot.y + plot.height - bh, bw, bh };
        DrawRectangleRounded(bar, 0.30f, 8, items[i].color);
        // label below
        std::string lbl = Ellipsize(f, items[i].label, bw + gap, 12.0f, 1.0f);
        DrawTextCenter(f, lbl.c_str(),
                       { bar.x - gap*0.5f, plot.y + plot.height + 6, bw + gap, 18 },
                       12.0f, th.textSecondary);
        if (showValues && bh > 18) {
            char vb[16]; std::snprintf(vb, sizeof(vb), "%.1f", items[i].value);
            DrawTextCenter(ThemeMgr::GetFontBold(), vb,
                           { bar.x, bar.y - 18, bar.width, 18 },
                           12.0f, th.textPrimary);
        }
    }
}

// ============================================================================
//  LineChart
// ============================================================================

void LineChart::OnShow() { animT = 0.0f; }
void LineChart::Update(float dt) { animT = Easing::Approach(animT, 1.0f, 0.30f, dt); }

void LineChart::Draw(const Font& f) const {
    const Theme& th = ThemeMgr::Current();
    if (!title.empty()) {
        DrawTextLeft(ThemeMgr::GetFontBold(), title.c_str(),
                     {bounds.x, bounds.y}, 18.0f, th.textPrimary);
    }
    float topPad = title.empty() ? 10.0f : 36.0f;
    Rectangle plot = { bounds.x + 8, bounds.y + topPad,
                       bounds.width - 8, bounds.height - topPad - 26.0f };

    int n = (int)values.size();
    if (n < 2) {
        DrawTextCenter(f, "Not enough data", plot, 14.0f, th.textMuted);
        return;
    }

    // grid
    int gridLines = 4;
    for (int i = 0; i <= gridLines; ++i) {
        float gy = plot.y + plot.height * (1.0f - (float)i / (float)gridLines);
        DrawLineEx({plot.x, gy}, {plot.x + plot.width, gy}, 1.0f, ColorAlpha(th.border, 0.7f));
    }
    float ease = Easing::OutCubic(animT);
    int cutoff = (int)((float)(n-1) * ease + 1.0f);
    if (cutoff > n) cutoff = n;

    auto pointAt = [&](int i) -> Vector2 {
        float xt = (float)i / (float)(n-1);
        float yt = (values[i] - minY) / (maxY - minY);
        if (yt < 0) yt = 0; if (yt > 1) yt = 1;
        return { plot.x + xt * plot.width, plot.y + (1.0f - yt) * plot.height };
    };

    // Filled area below line
    Color fill = stroke; fill.a = 50;
    if (cutoff >= 2) {
        for (int i = 0; i < cutoff - 1; ++i) {
            Vector2 p1 = pointAt(i), p2 = pointAt(i+1);
            Vector2 b1 = { p1.x, plot.y + plot.height };
            Vector2 b2 = { p2.x, plot.y + plot.height };
            DrawTriangle(p1, b1, p2, fill);
            DrawTriangle(p2, b1, b2, fill);
        }
    }

    // Line
    for (int i = 0; i < cutoff - 1; ++i) {
        Vector2 p1 = pointAt(i), p2 = pointAt(i+1);
        DrawLineEx(p1, p2, 2.5f, stroke);
    }
    // Points
    for (int i = 0; i < cutoff; ++i) {
        Vector2 p = pointAt(i);
        DrawCircleV(p, 5.0f, stroke);
        DrawCircleV(p, 2.5f, WHITE);
    }
    // x labels (only a few to avoid clutter)
    int step = (n > 7) ? (n / 6) : 1;
    for (int i = 0; i < n; i += step) {
        Vector2 p = pointAt(i);
        std::string lbl = (i < (int)xLabels.size()) ? xLabels[i] : std::to_string(i+1);
        DrawTextCenter(f, lbl.c_str(),
                       { p.x - 30, plot.y + plot.height + 6, 60, 14 },
                       11.0f, th.textMuted);
    }
}

// ============================================================================
//  Background particles
// ============================================================================

void BackgroundParticles::Init(int count, int sw, int sh) {
    blobs.clear();
    blobs.reserve(count);
    for (int i = 0; i < count; ++i) {
        Blob b;
        b.pos = { (float)GetRandomValue(0, sw), (float)GetRandomValue(0, sh) };
        b.vel = { ((float)GetRandomValue(-100, 100)) * 0.0002f,
                  ((float)GetRandomValue(-100, 100)) * 0.0002f };
        b.radius = (float)GetRandomValue(120, 280);
        // Pick a soft accent color (light pastel)
        Color base = (i % 2 == 0) ? ThemeMgr::Current().primary : ThemeMgr::Current().secondary;
        if (i % 3 == 0)            base = ThemeMgr::Current().accent;
        base.a = 36;
        b.color = base;
        blobs.push_back(b);
    }
}

void BackgroundParticles::Update(float dt, int sw, int sh) {
    for (auto& b : blobs) {
        b.pos.x += b.vel.x * dt * sw;
        b.pos.y += b.vel.y * dt * sh;
        // bounce softly
        if (b.pos.x < -b.radius)        b.pos.x = sw + b.radius;
        else if (b.pos.x > sw + b.radius) b.pos.x = -b.radius;
        if (b.pos.y < -b.radius)        b.pos.y = sh + b.radius;
        else if (b.pos.y > sh + b.radius) b.pos.y = -b.radius;
    }
    // Adapt color tint to current theme palette (so toggle theme refreshes visuals)
    for (size_t i = 0; i < blobs.size(); ++i) {
        Color base = (i % 2 == 0) ? ThemeMgr::Current().primary : ThemeMgr::Current().secondary;
        if (i % 3 == 0)            base = ThemeMgr::Current().accent;
        base.a = (ThemeMgr::Mode() == ThemeMode::DARK) ? 28 : 36;
        blobs[i].color = base;
    }
}

void BackgroundParticles::Draw(int sw, int sh) const {
    const Theme& th = ThemeMgr::Current();
    DrawRectangleGradientV(0, 0, sw, sh, th.background, th.backgroundAlt);
    for (const auto& b : blobs) {
        // Approximate soft circles via a couple of concentric layers
        Color outer = b.color; outer.a = (unsigned char)(b.color.a * 0.4f);
        Color inner = b.color;
        DrawCircleV(b.pos, b.radius,           outer);
        DrawCircleV(b.pos, b.radius * 0.65f,   inner);
    }
}

} // namespace UI

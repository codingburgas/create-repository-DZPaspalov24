#pragma once
#include "raylib.h"

// Visual theme + font registry. All UI reads colors from here so dark/light
// mode swap is instant across the whole app.

enum class ThemeMode { LIGHT, DARK };

struct Theme {
    Color background;        // window base
    Color backgroundAlt;     // gradient stop / second background tone
    Color surface;           // card body
    Color surfaceAlt;        // hovered/secondary card
    Color surfaceMuted;      // very subtle inset (e.g. input field)
    Color primary;           // dominant accent (soft blue/purple)
    Color primaryHover;
    Color primarySoft;       // tinted background of primary (chips, etc.)
    Color secondary;         // mint / teal accent
    Color accent;            // warm pink for highlights
    Color textPrimary;
    Color textSecondary;
    Color textMuted;
    Color border;
    Color borderStrong;
    Color success;           // green - high grades
    Color warning;           // amber - average grades
    Color error;             // red - low grades
    Color shadow;            // base shadow color (low alpha applied at draw)
};

namespace ThemeMgr {
    void Init();                // load fonts + apply default light theme
    void Shutdown();            // unload fonts

    const Theme& Current();
    ThemeMode Mode();
    void SetMode(ThemeMode m);
    void Toggle();

    Font GetFont();             // regular weight
    Font GetFontBold();         // emphasized weight (same TTF, larger spacing if no bold)
    Font GetFontDisplay();      // for big headings

    // Mix between a->b based on t in [0,1]. Used for hover/animation lerps.
    Color Lerp(Color a, Color b, float t);

    // Color for a numeric grade on the 2-6 scale.
    Color GradeColor(float grade);

    // Text label for a grade ("Excellent" / "Good" / etc).
    const char* GradeLabel(float grade);
}

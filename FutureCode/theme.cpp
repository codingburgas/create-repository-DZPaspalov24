#include "theme.h"
#include <cstdio>
#include <cstdlib>

namespace {
    Theme    g_theme;
    ThemeMode g_mode = ThemeMode::LIGHT;
    Font     g_font;
    Font     g_fontBold;
    Font     g_fontDisplay;
    bool     g_fontsLoaded = false;

    void ApplyLight() {
        g_theme.background    = {248, 250, 252, 255};   // crisp paper white
        g_theme.backgroundAlt = {236, 241, 247, 255};   // hint of cool grey
        g_theme.surface       = {255, 255, 255, 255};
        g_theme.surfaceAlt    = {245, 247, 251, 255};
        g_theme.surfaceMuted  = {238, 242, 247, 255};
        g_theme.primary       = { 19,  56, 110, 255};   // deep university navy
        g_theme.primaryHover  = { 28,  76, 138, 255};
        g_theme.primarySoft   = {221, 232, 245, 255};
        g_theme.secondary     = {142,  31,  45, 255};   // academic burgundy
        g_theme.accent        = {198, 156,  73, 255};   // warm gold
        g_theme.textPrimary   = { 18,  28,  48, 255};
        g_theme.textSecondary = { 79,  90, 113, 255};
        g_theme.textMuted     = {139, 149, 170, 255};
        g_theme.border        = {223, 229, 238, 255};
        g_theme.borderStrong  = {198, 207, 222, 255};
        g_theme.success       = { 34, 158, 110, 255};
        g_theme.warning       = {214, 158,  46, 255};
        g_theme.error         = {200,  56,  68, 255};
        g_theme.shadow        = { 19,  56, 110, 255};   // alpha applied per-draw
    }

    void ApplyDark() {
        g_theme.background    = { 15,  22,  38, 255};   // midnight navy
        g_theme.backgroundAlt = { 22,  32,  56, 255};
        g_theme.surface       = { 25,  36,  60, 255};
        g_theme.surfaceAlt    = { 36,  49,  78, 255};
        g_theme.surfaceMuted  = { 21,  31,  52, 255};
        g_theme.primary       = { 96, 156, 232, 255};   // bright sky-navy on dark
        g_theme.primaryHover  = {130, 178, 245, 255};
        g_theme.primarySoft   = { 38,  60, 105, 255};
        g_theme.secondary     = {220, 110, 124, 255};
        g_theme.accent        = {235, 198, 116, 255};
        g_theme.textPrimary   = {236, 240, 248, 255};
        g_theme.textSecondary = {174, 184, 207, 255};
        g_theme.textMuted     = {120, 130, 156, 255};
        g_theme.border        = { 46,  60,  92, 255};
        g_theme.borderStrong  = { 70,  86, 124, 255};
        g_theme.success       = { 75, 200, 148, 255};
        g_theme.warning       = {245, 188,  98, 255};
        g_theme.error         = {245, 130, 138, 255};
        g_theme.shadow        = {  0,   0,   0, 255};
    }

    Font TryLoadFont(const char* path, int size) {
        if (!FileExists(path)) return Font{0};

        // We need glyphs for the full ASCII range (32-126), Latin-1 supplement
        // (160-255 covers €, ©, ñ, é etc.) and Bulgarian Cyrillic
        // (1024-1279 covers А-Я, а-я and the Bulgarian-specific letters).
        // Plus a couple of common typographic punctuation marks.
        static int codepoints[512];
        static int codepointsCount = 0;
        if (codepointsCount == 0) {
            int idx = 0;
            for (int c = 32;   c <= 126;   ++c) codepoints[idx++] = c;     // ASCII
            for (int c = 160;  c <= 255;   ++c) codepoints[idx++] = c;     // Latin-1
            for (int c = 1024; c <= 1279;  ++c) codepoints[idx++] = c;     // Cyrillic
            // typographic punctuation
            codepoints[idx++] = 0x2013; // –
            codepoints[idx++] = 0x2014; // —
            codepoints[idx++] = 0x2018; // ‘
            codepoints[idx++] = 0x2019; // ’
            codepoints[idx++] = 0x201C; // “
            codepoints[idx++] = 0x201D; // ”
            codepoints[idx++] = 0x2022; // •
            codepoints[idx++] = 0x2026; // …
            codepoints[idx++] = 0x00B7; // ·
            codepointsCount = idx;
        }

        Font f = LoadFontEx(path, size, codepoints, codepointsCount);
        if (f.texture.id != 0) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
            return f;
        }
        return Font{0};
    }
}

namespace ThemeMgr {

void Init() {
    // We need a font that has Cyrillic glyphs. DejaVuSans is the most reliable
    // cross-platform choice. The user can also drop the .ttf file into resources/.
    const char* candidates[] = {
        "resources/DejaVuSans.ttf",
        "resources/fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        "/usr/local/share/fonts/DejaVuSans.ttf",
        "/Library/Fonts/DejaVuSans.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "C:/Windows/Fonts/DejaVuSans.ttf",
        "C:/Windows/Fonts/segoeui.ttf",  // Segoe UI also has Cyrillic
        "C:/Windows/Fonts/arial.ttf",    // Arial also has Cyrillic
    };
    const char* boldCandidates[] = {
        "resources/DejaVuSans-Bold.ttf",
        "resources/fonts/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
        "/usr/local/share/fonts/DejaVuSans-Bold.ttf",
        "/Library/Fonts/DejaVuSans-Bold.ttf",
        "C:/Windows/Fonts/DejaVuSans-Bold.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
    };

    g_font.texture.id = 0;
    g_fontBold.texture.id = 0;
    g_fontDisplay.texture.id = 0;

    for (const char* p : candidates) {
        g_font = TryLoadFont(p, 36);
        if (g_font.texture.id != 0) break;
    }
    for (const char* p : boldCandidates) {
        g_fontBold = TryLoadFont(p, 36);
        if (g_fontBold.texture.id != 0) break;
    }
    for (const char* p : boldCandidates) {
        g_fontDisplay = TryLoadFont(p, 64);
        if (g_fontDisplay.texture.id != 0) break;
    }

    if (g_font.texture.id == 0)        g_font = GetFontDefault();
    if (g_fontBold.texture.id == 0)    g_fontBold = g_font;
    if (g_fontDisplay.texture.id == 0) g_fontDisplay = g_fontBold;

    g_fontsLoaded = true;
    ApplyLight();
}

void Shutdown() {
    if (!g_fontsLoaded) return;
    Font def = GetFontDefault();
    if (g_font.texture.id        != def.texture.id) UnloadFont(g_font);
    if (g_fontBold.texture.id    != def.texture.id && g_fontBold.texture.id != g_font.texture.id) UnloadFont(g_fontBold);
    if (g_fontDisplay.texture.id != def.texture.id && g_fontDisplay.texture.id != g_fontBold.texture.id) UnloadFont(g_fontDisplay);
    g_fontsLoaded = false;
}

const Theme& Current() { return g_theme; }
ThemeMode Mode()       { return g_mode; }

void SetMode(ThemeMode m) {
    g_mode = m;
    if (m == ThemeMode::LIGHT) ApplyLight(); else ApplyDark();
}
void Toggle() { SetMode(g_mode == ThemeMode::LIGHT ? ThemeMode::DARK : ThemeMode::LIGHT); }

Font GetFont()        { return g_font; }
Font GetFontBold()    { return g_fontBold; }
Font GetFontDisplay() { return g_fontDisplay; }

Color Lerp(Color a, Color b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

Color GradeColor(float grade) {
    const Theme& t = g_theme;
    if (grade >= 5.5f) return t.success;
    if (grade >= 4.5f) return Lerp(t.success, t.warning, 0.35f);
    if (grade >= 3.5f) return t.warning;
    if (grade >= 2.5f) return Lerp(t.warning, t.error, 0.5f);
    return t.error;
}

const char* GradeLabel(float grade) {
    // Use Bulgarian academic grade labels (the official 6-point scale):
    //   6 = Отличен        (Excellent)
    //   5 = Мн. добър       (Very Good)
    //   4 = Добър           (Good)
    //   3 = Среден          (Average / passing)
    //   2 = Слаб            (Failing)
    if (grade >= 5.5f) return "Отличен";
    if (grade >= 4.5f) return "Мн. добър";
    if (grade >= 3.5f) return "Добър";
    if (grade >= 2.5f) return "Среден";
    return "Слаб";
}

} // namespace ThemeMgr

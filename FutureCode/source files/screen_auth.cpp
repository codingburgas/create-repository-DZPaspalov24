#include "screen_auth.h"
#include "app.h"
#include "theme.h"
#include "easing.h"
#include "i18n.h"

#include <cstdio>
#include <algorithm>

namespace {
    // Custom fonts at default sizes
    inline Font Fnt() { return ThemeMgr::GetFont(); }
    inline Font Bld() { return ThemeMgr::GetFontBold(); }
    inline Font Dis() { return ThemeMgr::GetFontDisplay(); }
}

// =====================================================================
//  LoginScreen
// =====================================================================

void LoginScreen::OnEnter(App& app) {
    (void)app;
    facultyInput.placeholder = "F23001";
    facultyInput.maxLength = 16;

    passInput.placeholder = "••••••••";
    passInput.password = true;
    passInput.maxLength = 64;

    loginBtn.kind   = UI::Button::PRIMARY;
    googleBtn.kind  = UI::Button::SECONDARY;
    signupBtn.kind  = UI::Button::GHOST;
    forgotFnBtn.kind   = UI::Button::GHOST;
    forgotPassBtn.kind = UI::Button::GHOST;
    whatNewBtn.kind = UI::Button::GHOST;
    langBtn.kind    = UI::Button::GHOST;

    welcomeModal.bounds = {0, 0, 560, 540};
    welcomeOkBtn.kind = UI::Button::PRIMARY;
}

void LoginScreen::OnShow(App& app) {
    (void)app;
    enterT = 0.0f;
    errorMsg.clear();
    errorFade = 0.0f;
    facultyInput.focused = false;
    passInput.focused    = false;
}

void LoginScreen::Update(App& app, float dt, Vector2 mouse) {
    enterT = Easing::Approach(enterT, 1.0f, 0.30f, dt);
    if (errorFade < 1.0f && !errorMsg.empty())
        errorFade = Easing::Approach(errorFade, 1.0f, 0.20f, dt);

    facultyInput.Update(dt, mouse);
    passInput.Update(dt, mouse);

    int sw = app.W();
    // Top-bar buttons
    whatNewBtn.label = I18n::T("login.what_new");
    {
        Vector2 m = UI::MeasureText(Fnt(), whatNewBtn.label.c_str(), 13.0f);
        whatNewBtn.bounds = { (float)sw - 240, 16, m.x + 24, 32 };
    }
    langBtn.label = (I18n::Current() == Lang::BG ? "БГ | en" : "bg | EN");
    {
        Vector2 m = UI::MeasureText(Fnt(), langBtn.label.c_str(), 13.0f);
        langBtn.bounds = { (float)sw - 90, 16, m.x + 28, 32 };
    }
    whatNewBtn.Update(dt, mouse);
    langBtn.Update(dt, mouse);

    if (welcomeModal.IsVisible()) {
        // While modal is visible, swallow form clicks
        welcomeModal.Update(dt);
        welcomeOkBtn.Update(dt, mouse);
        if (welcomeOkBtn.Clicked(mouse) || IsKeyPressed(KEY_ESCAPE)) welcomeModal.Hide();
        return;
    }
    welcomeModal.Update(dt);

    if (whatNewBtn.Clicked(mouse)) welcomeModal.Show();
    if (langBtn.Clicked(mouse)) {
        I18n::Toggle();
    }

    // Form geometry: hero card centred. Right column has the form.
    int sh = app.H();
    Rectangle card = {
        (float)(sw / 2 - 460),
        (float)(sh / 2 - 290),
        920, 580
    };
    Rectangle right = { card.x + card.width / 2.0f, card.y, card.width / 2.0f, card.height };
    float fx = right.x + 50;
    float fw = right.width - 100;

    facultyInput.bounds = { fx, right.y + 178, fw, 44 };
    passInput.bounds    = { fx, right.y + 268, fw, 44 };

    loginBtn.label = I18n::T("login.submit");
    loginBtn.bounds  = { fx, right.y + 358, fw, 48 };

    googleBtn.label = (I18n::Current() == Lang::BG ? "Вход с Google" : "Sign in with Google");
    googleBtn.bounds = { fx, right.y + 444, fw, 44 };

    signupBtn.label = I18n::T("login.create_account");
    signupBtn.bounds = { fx, right.y + right.height - 56, fw, 36 };

    // Forgot links - small, sit below their respective inputs
    forgotFnBtn.label = I18n::T("login.forgot_fn");
    {
        Vector2 m = UI::MeasureText(Fnt(), forgotFnBtn.label.c_str(), 11.0f);
        forgotFnBtn.bounds = { fx + fw - m.x - 12, right.y + 224, m.x + 12, 18 };
    }
    forgotPassBtn.label = I18n::T("login.forgot_pass");
    {
        Vector2 m = UI::MeasureText(Fnt(), forgotPassBtn.label.c_str(), 11.0f);
        forgotPassBtn.bounds = { fx + fw - m.x - 12, right.y + 314, m.x + 12, 18 };
    }

    loginBtn.Update(dt, mouse);
    googleBtn.Update(dt, mouse);
    signupBtn.Update(dt, mouse);
    forgotFnBtn.Update(dt, mouse);
    forgotPassBtn.Update(dt, mouse);

    bool tryLogin = loginBtn.Clicked(mouse) || IsKeyPressed(KEY_ENTER);

    if (forgotFnBtn.Clicked(mouse) || forgotPassBtn.Clicked(mouse)) {
        app.Toast(I18n::T("login.recovery_msg"), ThemeMgr::Current().accent);
    }
    if (googleBtn.Clicked(mouse)) {
        app.Toast(I18n::Current() == Lang::BG
                  ? "Външният вход не е достъпен в demo режим."
                  : "External sign-in is unavailable in the demo.",
                  ThemeMgr::Current().warning);
    }
    if (signupBtn.Clicked(mouse)) {
        app.Goto(ScreenId::SIGNUP);
    }

    // Tab cycles fields
    if (IsKeyPressed(KEY_TAB)) {
        if (facultyInput.focused) {
            facultyInput.focused = false;
            passInput.focused = true;
        } else {
            facultyInput.focused = true;
            passInput.focused = false;
        }
    }

    if (tryLogin) {
        std::string fn = facultyInput.value;
        std::string pw = passInput.value;
        if (fn.empty() || pw.empty()) {
            errorMsg = I18n::T("login.error_empty");
            errorFade = 0.0f;
            return;
        }
        User* u = app.Data().FindUserByLogin(fn);
        if (!u || !Models::VerifyPassword(pw, u->passwordEnc)) {
            errorMsg = I18n::T("login.error_credentials");
            errorFade = 0.0f;
            return;
        }
        // Success
        app.SetCurrentUser(u->id);
        ThemeMgr::SetMode(u->darkMode ? ThemeMode::DARK : ThemeMode::LIGHT);
        std::string greet = std::string(I18n::T("login.welcome_back")) + ", " + u->fullName + "!";
        app.Toast(greet, ThemeMgr::Current().success);
        if (u->role == Role::TEACHER) app.Goto(ScreenId::TEACHER_DASH);
        else                          app.Goto(ScreenId::STUDENT_DASH);
    }
}

void LoginScreen::Draw(App& app) {
    const Theme& th = ThemeMgr::Current();
    int sw = app.W(), sh = app.H();

    // -------- Top bar (full width) --------
    UI::DrawTextLeft(Bld(), I18n::T("app.university"), {28, 22}, 14.0f, th.textPrimary);
    UI::DrawTextLeft(Fnt(), I18n::T("app.subtitle"),   {28, 42}, 11.0f, th.textMuted);

    // Top-right buttons (whatNewBtn + langBtn) - manually styled as ghost text
    auto drawTopBtn = [&](UI::Button& b, bool active = false){
        Color tc = active ? th.primary : th.textSecondary;
        if (b.hoverT > 0.05f) tc = ThemeMgr::Lerp(tc, th.primary, b.hoverT);
        UI::DrawTextCenter(Bld(), b.label.c_str(), b.bounds, 13.0f, tc, 0.5f);
    };
    drawTopBtn(whatNewBtn);
    drawTopBtn(langBtn, true);

    // -------- Hero card --------
    Rectangle card = {
        (float)(sw / 2 - 460),
        (float)(sh / 2 - 290),
        920, 580
    };
    // Animated entry
    float t = Easing::OutBack(Easing::Clamp01(enterT));
    float scale = 0.96f + 0.04f * t;
    Rectangle cs = {
        card.x + card.width  * (1.0f - scale) * 0.5f,
        card.y + card.height * (1.0f - scale) * 0.5f + (1.0f - t) * 12.0f,
        card.width  * scale,
        card.height * scale
    };
    UI::DrawSoftShadow(cs, 0.04f, 30.0f, 0.16f * t);
    DrawRectangleRounded(cs, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(cs, 0.04f, 8, 1.0f, th.border);

    // -------- Left half: brand panel --------
    Rectangle left  = { cs.x, cs.y, cs.width / 2.0f, cs.height };
    Rectangle right = { cs.x + cs.width / 2.0f, cs.y, cs.width / 2.0f, cs.height };

    // Gradient panel: deep navy -> slightly lighter navy
    BeginScissorMode((int)left.x, (int)left.y, (int)left.width, (int)left.height);
    {
        // The card's left half has its own rounded corners on the left edge only.
        // We draw a rectangle that bleeds slightly past the right edge so it
        // tiles into the form panel cleanly.
        Rectangle fill = { left.x, left.y, left.width + 12, left.height };
        UI::DrawRoundedVerticalGradient(fill, 0.08f, th.primary,
            ThemeMgr::Lerp(th.primary, th.secondary, 0.5f));
    }
    // Logo crest (left side of brand panel)
    UI::DrawUniversityCrest({left.x + 60, left.y + 70}, 32.0f, WHITE, th.primary);

    UI::DrawTextLeft(Fnt(), I18n::T("app.subtitle"),
                     {left.x + 110, left.y + 50}, 13.0f,
                     ColorAlpha(WHITE, 0.85f));
    UI::DrawTextLeft(Bld(), "Student",
                     {left.x + 110, left.y + 70}, 18.0f, WHITE);

    // Big tagline
    UI::DrawTextLeft(Dis(), I18n::T("app.university"),
                     {left.x + 36, left.y + 160}, 32.0f, WHITE);
    UI::DrawTextLeft(Bld(),
                     I18n::Current() == Lang::BG
                         ? "Технологичен университет"
                         : "Technology University",
                     {left.x + 36, left.y + 204}, 17.0f,
                     ColorAlpha(WHITE, 0.9f));

    // Bullet feature list - sells the system below the tagline
    const char* features[] = {
        "login.feature_grades",
        "login.feature_curriculum",
        "login.feature_schedule",
        "login.feature_exams",
        "login.feature_docs",
    };
    float py = left.y + 270;
    for (const char* k : features) {
        // Real check icon in a soft circle
        DrawCircleV({left.x + 48, py + 8}, 11.0f, ColorAlpha(WHITE, 0.18f));
        UI::IconCheck({left.x + 48, py + 8}, 9.0f, WHITE, 2.2f);
        UI::DrawTextLeft(Fnt(), I18n::T(k),
                         {left.x + 70, py + 1}, 13.0f,
                         ColorAlpha(WHITE, 0.9f));
        py += 32;
    }
    EndScissorMode();

    // -------- Right half: form --------
    UI::DrawTextLeft(Dis(), I18n::T("login.title"),
                     {right.x + 50, right.y + 50}, 32.0f, th.textPrimary);
    UI::DrawTextLeft(Fnt(),
                     I18n::Current() == Lang::BG
                         ? "Влезте с факултетния си номер."
                         : "Enter your faculty number to continue.",
                     {right.x + 50, right.y + 92}, 13.0f, th.textSecondary);
    // Underline accent
    DrawRectangle((int)(right.x + 50), (int)(right.y + 130),
                  40, 3, th.primary);

    // Field labels
    UI::DrawTextLeft(Bld(), I18n::T("login.faculty_number"),
                     {right.x + 50, right.y + 158}, 12.0f, th.textMuted);
    UI::DrawTextLeft(Bld(), I18n::T("login.password"),
                     {right.x + 50, right.y + 248}, 12.0f, th.textMuted);

    facultyInput.Draw();
    passInput.Draw();

    // Forgot links: rendered as muted ghost-text
    auto drawForgot = [&](UI::Button& b){
        Color c = b.hoverT > 0.05f ? th.primary : th.textMuted;
        UI::DrawTextCenter(Fnt(), b.label.c_str(), b.bounds, 11.0f, c, 0.5f);
    };
    drawForgot(forgotFnBtn);
    drawForgot(forgotPassBtn);

    loginBtn.Draw();

    // "or" divider
    {
        float divY = right.y + 418;
        DrawLineEx({right.x + 50, divY}, {right.x + right.width / 2 - 22, divY},
                   1.0f, th.border);
        DrawLineEx({right.x + right.width / 2 + 22, divY}, {right.x + right.width - 50, divY},
                   1.0f, th.border);
        UI::DrawTextCenter(Fnt(), I18n::Current() == Lang::BG ? "или" : "or",
                           { right.x + right.width / 2 - 20, divY - 8, 40, 16 },
                           12.0f, th.textMuted, 0.5f);
    }

    // Google button (white-bg variant - draw manually to override SECONDARY look)
    {
        Rectangle b = googleBtn.bounds;
        bool hov = googleBtn.hoverT > 0.05f;
        Color bg = hov ? ThemeMgr::Lerp(th.surface, th.surfaceAlt, googleBtn.hoverT) : th.surface;
        UI::DrawSoftShadow(b, 0.30f, 6.0f, 0.05f);
        DrawRectangleRounded(b, 0.30f, 6, bg);
        UI::StrokeRoundedRect(b, 0.30f, 6, 1.0f, th.border);
        // Google G mark - multi-color
        UI::IconGoogleG({ b.x + 28, b.y + b.height * 0.5f }, 11.0f);
        UI::DrawTextCenter(Bld(), googleBtn.label.c_str(), b, 14.0f, th.textPrimary, 0.5f);
    }

    signupBtn.Draw();

    // Error banner
    if (errorFade > 0.01f && !errorMsg.empty()) {
        Rectangle eb = { right.x + 50, right.y + 158 - 26,
                          right.width - 100, 22 };
        Color bg = th.error; bg.a = (unsigned char)(40 * errorFade);
        DrawRectangleRounded(eb, 0.40f, 6, bg);
        Color tc = th.error; tc.a = (unsigned char)(255 * errorFade);
        UI::DrawTextCenter(Bld(), errorMsg.c_str(), eb, 11.0f, tc, 0.5f);
    }

    // -------- Welcome / what's new modal --------
    if (welcomeModal.IsVisible()) {
        Rectangle content = welcomeModal.BeginFrame();
        UI::DrawTextLeft(Dis(), I18n::T("common.welcome_modal_title"),
                         {content.x, content.y}, 24.0f, th.textPrimary);
        UI::DrawTextLeft(Fnt(), I18n::T("common.welcome_modal_intro"),
                         {content.x, content.y + 36}, 14.0f, th.textSecondary);

        const char* feats[] = {
            "login.feature_design",
            "login.feature_grades",
            "login.feature_curriculum",
            "login.feature_schedule",
            "login.feature_exams",
            "login.feature_docs",
        };
        float py2 = content.y + 76;
        for (const char* k : feats) {
            DrawCircleV({content.x + 14, py2 + 10}, 11.0f, ColorAlpha(th.success, 0.18f));
            UI::IconCheck({content.x + 14, py2 + 10}, 8.0f, th.success, 2.2f);
            UI::DrawTextLeft(Fnt(), I18n::T(k),
                             {content.x + 36, py2 + 4}, 13.0f, th.textPrimary);
            py2 += 32;
        }

        welcomeOkBtn.label = I18n::T("login.go_to_student");
        welcomeOkBtn.bounds = {
            content.x + content.width - 200,
            content.y + content.height - 50,
            200, 44
        };
        welcomeOkBtn.Draw();
        welcomeModal.EndFrame();
    }
}

// =====================================================================
//  SignupScreen
// =====================================================================

void SignupScreen::OnEnter(App& app) {
    (void)app;
    name.placeholder = "Mария Иванова";
    user.placeholder = "username";
    facultyNo.placeholder = "F23010";
    pass.placeholder  = "Парола (≥4)";
    pass.password = true;
    pass2.placeholder = "Повтори паролата";
    pass2.password = true;
    klass.placeholder = "Информатика";

    submitBtn.kind = UI::Button::PRIMARY;
    backBtn.kind   = UI::Button::GHOST;
}

void SignupScreen::OnShow(App& app) {
    (void)app;
    enterT = 0.0f;
    errorMsg.clear();
    errorFade = 0.0f;
    chosenRole = Role::STUDENT;
}

void SignupScreen::Update(App& app, float dt, Vector2 mouse) {
    enterT = Easing::Approach(enterT, 1.0f, 0.30f, dt);
    if (errorFade < 1.0f && !errorMsg.empty())
        errorFade = Easing::Approach(errorFade, 1.0f, 0.20f, dt);

    int sw = app.W(), sh = app.H();
    Rectangle card = {
        (float)(sw / 2 - 240),
        (float)(sh / 2 - 360),
        480, 720
    };
    float fx = card.x + 36;
    float fw = card.width - 72;

    name.bounds      = { fx,         card.y + 168, fw,         44 };
    user.bounds      = { fx,         card.y + 248, fw / 2 - 8, 44 };
    facultyNo.bounds = { fx + fw/2+8,card.y + 248, fw / 2 - 8, 44 };
    pass.bounds      = { fx,         card.y + 328, fw / 2 - 8, 44 };
    pass2.bounds     = { fx + fw/2+8,card.y + 328, fw / 2 - 8, 44 };
    klass.bounds     = { fx,         card.y + 460, fw,         44 };

    name.Update(dt, mouse);
    user.Update(dt, mouse);
    facultyNo.Update(dt, mouse);
    pass.Update(dt, mouse);
    pass2.Update(dt, mouse);
    if (chosenRole == Role::STUDENT) klass.Update(dt, mouse);

    submitBtn.label = I18n::T("signup.submit");
    submitBtn.bounds = { fx, card.y + card.height - 116, fw, 48 };
    submitBtn.Update(dt, mouse);

    backBtn.label = I18n::T("signup.have_account");
    backBtn.bounds = { fx, card.y + card.height - 56, fw, 36 };
    backBtn.Update(dt, mouse);

    if (backBtn.Clicked(mouse)) app.Goto(ScreenId::LOGIN);

    bool submit = submitBtn.Clicked(mouse) || IsKeyPressed(KEY_ENTER);
    if (submit) {
        if (name.value.empty() || user.value.empty() || pass.value.empty()) {
            errorMsg = I18n::T("signup.error_empty"); errorFade = 0.0f; return;
        }
        if (pass.value.size() < 4) {
            errorMsg = I18n::T("signup.error_short_pwd"); errorFade = 0.0f; return;
        }
        if (pass.value != pass2.value) {
            errorMsg = I18n::T("signup.error_mismatch"); errorFade = 0.0f; return;
        }
        if (chosenRole == Role::STUDENT && klass.value.empty()) {
            errorMsg = I18n::T("signup.error_program"); errorFade = 0.0f; return;
        }
        if (app.Data().FindUserByUsername(user.value)) {
            errorMsg = I18n::T("signup.error_taken"); errorFade = 0.0f; return;
        }
        if (!facultyNo.value.empty() && app.Data().FindUserByFacultyNumber(facultyNo.value)) {
            errorMsg = I18n::T("signup.error_fn_taken"); errorFade = 0.0f; return;
        }

        User u;
        u.role = chosenRole;
        u.fullName = name.value;
        u.username = user.value;
        u.facultyNumber = facultyNo.value;
        u.passwordEnc = Models::ObfuscatePassword(pass.value);
        u.className = klass.value;
        u.faculty = "Факултет по технически науки";
        u.currentSemester = 1; u.year = 1;
        // Auto-generate faculty number if blank for students
        if (u.role == Role::STUDENT && u.facultyNumber.empty()) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "F%05d", 23000 + (int)app.Data().Users().size() + 1);
            u.facultyNumber = buf;
        }

        int newId = app.Data().CreateUser(u);
        app.SetCurrentUser(newId);
        std::string greet = std::string(I18n::T("login.welcome_back")) + ", " + u.fullName + "!";
        app.Toast(greet, ThemeMgr::Current().success);
        if (u.role == Role::TEACHER) app.Goto(ScreenId::TEACHER_DASH);
        else                          app.Goto(ScreenId::STUDENT_DASH);
    }
}

void SignupScreen::Draw(App& app) {
    const Theme& th = ThemeMgr::Current();
    int sw = app.W(), sh = app.H();

    Rectangle card = {
        (float)(sw / 2 - 240),
        (float)(sh / 2 - 360),
        480, 720
    };
    // Animated entry
    float t = Easing::OutBack(Easing::Clamp01(enterT));
    float scale = 0.96f + 0.04f * t;
    Rectangle cs = {
        card.x + card.width  * (1.0f - scale) * 0.5f,
        card.y + card.height * (1.0f - scale) * 0.5f + (1.0f - t) * 12.0f,
        card.width  * scale,
        card.height * scale
    };
    UI::DrawSoftShadow(cs, 0.04f, 26.0f, 0.16f * t);
    DrawRectangleRounded(cs, 0.04f, 8, th.surface);
    UI::StrokeRoundedRect(cs, 0.04f, 8, 1.0f, th.border);

    UI::DrawTextLeft(Dis(), I18n::T("signup.title"),
                     {cs.x + 36, cs.y + 36}, 26.0f, th.textPrimary);
    DrawRectangle((int)(cs.x + 36), (int)(cs.y + 76), 40, 3, th.primary);

    // Role pills
    Rectangle rp = { cs.x + 36, cs.y + 100, cs.width - 72, 38 };
    DrawRectangleRounded(rp, 1.0f, 6, th.surfaceMuted);
    UI::StrokeRoundedRect(rp, 1.0f, 6, 1.0f, th.border);
    Rectangle pillT = { rp.x + 4, rp.y + 4, rp.width / 2 - 6, rp.height - 8 };
    Rectangle pillS = { rp.x + rp.width / 2 + 2, rp.y + 4, rp.width / 2 - 6, rp.height - 8 };
    Rectangle activePill = (chosenRole == Role::TEACHER) ? pillT : pillS;
    DrawRectangleRounded(activePill, 1.0f, 6, th.surface);
    UI::StrokeRoundedRect(activePill, 1.0f, 6, 1.0f, th.primary);

    UI::DrawTextCenter(Bld(), I18n::T("login.role.teacher"), pillT, 13.0f,
                       chosenRole == Role::TEACHER ? th.primary : th.textSecondary, 0.5f);
    UI::DrawTextCenter(Bld(), I18n::T("login.role.student"), pillS, 13.0f,
                       chosenRole == Role::STUDENT ? th.primary : th.textSecondary, 0.5f);
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, pillT)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) chosenRole = Role::TEACHER;
    }
    if (CheckCollisionPointRec(mouse, pillS)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) chosenRole = Role::STUDENT;
    }

    UI::DrawTextLeft(Bld(), I18n::T("signup.full_name"), {name.bounds.x, name.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(Bld(), I18n::T("signup.username"),  {user.bounds.x, user.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(Bld(), I18n::T("signup.faculty_number"), {facultyNo.bounds.x, facultyNo.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(Bld(), I18n::T("signup.password"),  {pass.bounds.x, pass.bounds.y - 18}, 12.0f, th.textMuted);
    UI::DrawTextLeft(Bld(), I18n::T("signup.confirm"),   {pass2.bounds.x, pass2.bounds.y - 18}, 12.0f, th.textMuted);

    name.Draw();
    user.Draw();
    facultyNo.Draw();
    pass.Draw();
    pass2.Draw();

    if (chosenRole == Role::STUDENT) {
        UI::DrawTextLeft(Bld(), I18n::T("signup.program"),
                         {klass.bounds.x, klass.bounds.y - 18}, 12.0f, th.textMuted);
        klass.Draw();
    }

    submitBtn.Draw();
    backBtn.Draw();

    if (errorFade > 0.01f && !errorMsg.empty()) {
        Rectangle eb = { cs.x + 36, cs.y + cs.height - 168, cs.width - 72, 30 };
        Color bg = th.error; bg.a = (unsigned char)(40 * errorFade);
        DrawRectangleRounded(eb, 0.40f, 6, bg);
        Color tc = th.error; tc.a = (unsigned char)(255 * errorFade);
        UI::DrawTextCenter(Bld(), errorMsg.c_str(), eb, 12.0f, tc, 0.5f);
    }
}

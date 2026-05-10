#include "app.h"
#include "theme.h"
#include "easing.h"
#include "models.h"
#include "i18n.h"

#include "screen_auth.h"
#include "screen_teacher.h"
#include "screen_student.h"
#include "screen_stats.h"

#include <cmath>
#include <cstdio>

namespace {
    const float kHeaderH = 80.0f;
}

App::App() = default;
App::~App() = default;

void App::Init() {
    ThemeMgr::Init();

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    particles.Init(8, sw, sh);

    data.Load("data/grademaster.dat");

    BuildScreens();

    // Sync theme toggle with current mode
    themeToggle.value = (ThemeMgr::Mode() == ThemeMode::DARK);

    GotoImmediate(ScreenId::LOGIN);
}

void App::Shutdown() {
    data.Save();
    ThemeMgr::Shutdown();
}

void App::BuildScreens() {
    screens.clear();
    auto add = [&](ScreenId id, std::unique_ptr<Screen> s){
        s->OnEnter(*this);
        screens[(int)id] = std::move(s);
    };
    add(ScreenId::LOGIN,          std::make_unique<LoginScreen>());
    add(ScreenId::SIGNUP,         std::make_unique<SignupScreen>());
    add(ScreenId::TEACHER_DASH,   std::make_unique<TeacherDashboard>());
    add(ScreenId::STUDENT_DASH,   std::make_unique<StudentDashboard>());
    add(ScreenId::STUDENT_DETAIL, std::make_unique<StudentDetailScreen>());
    add(ScreenId::STATISTICS,     std::make_unique<StatisticsScreen>());
}

Screen* App::GetScreen(ScreenId id) {
    auto it = screens.find((int)id);
    if (it == screens.end()) return nullptr;
    return it->second.get();
}

User* App::CurrentUser() {
    if (currentUserId == 0) return nullptr;
    return data.FindUserById(currentUserId);
}

void App::Logout() {
    currentUserId = 0;
    selectedStudentId = 0;
    // Navigate back to the login screen so the user actually sees they
    // logged out (the previous version just cleared state, which left the
    // current dashboard rendering with a null user pointer).
    Goto(ScreenId::LOGIN);
}

void App::Toast(const std::string& text, Color accent) {
    toasts.Push(text, accent);
}

void App::Goto(ScreenId id) {
    if (transitioning) {
        // Snap-finish the previous transition first
        if (Screen* cur = GetScreen(currentId)) cur->OnHide(*this);
        currentId = pendingId;
        if (Screen* now = GetScreen(currentId)) now->OnShow(*this);
    }
    pendingId = id;
    transitioning = true;
    transitionT = 0.0f;
    transitionSwapped = false;
}

void App::GotoImmediate(ScreenId id) {
    if (Screen* cur = GetScreen(currentId)) cur->OnHide(*this);
    currentId = id;
    pendingId = id;
    transitioning = false;
    transitionT = 0.0f;
    transitionSwapped = false;
    if (Screen* now = GetScreen(currentId)) now->OnShow(*this);
}

void App::UpdateHeader(float dt, Vector2 mouse) {
    int sw = GetScreenWidth();
    User* u = CurrentUser();
    bool loggedIn = (u != nullptr);

    // Place toggles at the top-right corner. When logged out (login/signup
    // screens), those screens manage their own top-bar - hide the global header
    // controls to avoid double widgets.
    if (!loggedIn) {
        notifPanelOpen = false; notifPanelT = 0.0f;
        // Off-screen so they don't capture clicks
        themeToggle.bounds = { -1000, -1000, 52, 28 };
        notifBellBounds   = { -1000, -1000, 40, 28 };
        return;
    }

    // Right-edge offsets within the 96px top header
    // Order from right edge:
    //   logout (handled by screen), theme toggle, notif bell
    // The student/teacher dashboards both reserve space on the right
    // for their own logout button. We sit to the LEFT of that.
    themeToggle.bounds = { (float)sw - 240.0f, 34.0f, 56, 30 };
    themeToggle.Update(dt, mouse);
    if (themeToggle.Clicked(mouse)) {
        ThemeMgr::Toggle();
        themeToggle.value = (ThemeMgr::Mode() == ThemeMode::DARK);
        if (User* uu = CurrentUser()) {
            uu->darkMode = themeToggle.value;
            data.UpdateUser(*uu);
        }
    }

    // Notification bell
    notifBellBounds = { (float)sw - 304.0f, 33.0f, 48, 32 };

    if (CheckCollisionPointRec(mouse, notifBellBounds)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            notifPanelOpen = !notifPanelOpen;
        }
    } else if (notifPanelOpen) {
        Rectangle panel = { (float)sw - 360.0f, 80.0f, 320, 360 };
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
            !CheckCollisionPointRec(mouse, panel) &&
            !CheckCollisionPointRec(mouse, notifBellBounds)) {
            notifPanelOpen = false;
        }
    }
    float target = notifPanelOpen ? 1.0f : 0.0f;
    notifPanelT = Easing::Approach(notifPanelT, target, 0.10f, dt);
}

void App::DrawHeader(float dt) {
    (void)dt;
    const Theme& th = ThemeMgr::Current();
    Font f  = ThemeMgr::GetFont();
    Font fb = ThemeMgr::GetFontBold();

    User* u = CurrentUser();
    if (!u) return;  // login/signup draw their own top bar

    // Theme toggle
    UI::DrawTextRight(f,
                      ThemeMgr::Mode() == ThemeMode::DARK
                          ? I18n::T("common.theme.dark")
                          : I18n::T("common.theme.light"),
                      { themeToggle.bounds.x - 80, themeToggle.bounds.y + 6, 70, 16 },
                      11.0f, th.textMuted);
    themeToggle.Draw();

    // Bell + unread badge
    Rectangle b = notifBellBounds;
    bool h = CheckCollisionPointRec(GetMousePosition(), b);
    Color bg = h ? th.surfaceAlt : th.surfaceMuted;
    DrawRectangleRounded(b, 0.40f, 6, bg);
    UI::StrokeRoundedRect(b, 0.40f, 6, 1.0f, th.border);
    UI::IconBell({ b.x + b.width * 0.5f, b.y + b.height * 0.5f }, 9.0f, th.textPrimary, 1.8f);

    int unread = 0;
    for (auto& n : data.Notifications()) if (n.userId == u->id && !n.read) unread++;
    if (unread > 0) {
        Vector2 dot = { b.x + b.width - 6, b.y + 4 };
        DrawCircleV(dot, 6.0f, th.error);
        char nb[8]; std::snprintf(nb, sizeof(nb), "%d", unread > 9 ? 9 : unread);
        Rectangle nbr = { dot.x - 6, dot.y - 6, 12, 12 };
        UI::DrawTextCenter(fb, nb, nbr, 9.0f, WHITE, 0.5f);
    }

    // Dropdown panel
    if (notifPanelT > 0.001f) {
        int sw = GetScreenWidth();
        Rectangle panel = { (float)sw - 360.0f, 80.0f, 320, 360 };
        float ease = Easing::OutBack(Easing::Clamp01(notifPanelT));
        panel.y = 80.0f - (1.0f - ease) * 12.0f;

        UI::DrawSoftShadow(panel, 0.10f, 22.0f, 0.20f * notifPanelT);
        Color cardC = th.surface;
        cardC.a = (unsigned char)(255 * notifPanelT);
        DrawRectangleRounded(panel, 0.10f, 8, cardC);
        UI::StrokeRoundedRect(panel, 0.10f, 8, 1.0f, ColorAlpha(th.border, notifPanelT));

        UI::DrawTextLeft(fb,
                         I18n::Current() == Lang::BG ? "Известия" : "Notifications",
                         {panel.x + 16, panel.y + 14}, 16.0f,
                         ColorAlpha(th.textPrimary, notifPanelT));

        // "Mark all read" link
        const char* mlbl = (I18n::Current() == Lang::BG) ? "Маркирай всички" : "Mark all read";
        Vector2 mlm = UI::MeasureText(fb, mlbl, 11.0f);
        Rectangle markR = { panel.x + panel.width - mlm.x - 16, panel.y + 14, mlm.x + 8, 20 };
        bool mh = CheckCollisionPointRec(GetMousePosition(), markR);
        Color mc = mh ? th.primaryHover : th.primary;
        mc.a = (unsigned char)(255 * notifPanelT);
        UI::DrawTextRight(fb, mlbl,
                          { markR.x, markR.y + 2, markR.width, markR.height },
                          11.0f, mc);
        if (mh) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                data.MarkAllRead(u->id);
            }
        }

        // List items
        BeginScissorMode((int)panel.x, (int)panel.y + 44,
                         (int)panel.width, (int)panel.height - 56);
        float py = panel.y + 50;
        int shown = 0;
        for (auto it = data.Notifications().rbegin(); it != data.Notifications().rend(); ++it) {
            if (it->userId != u->id) continue;
            if (shown++ >= 12) break;

            Rectangle row = { panel.x + 8, py, panel.width - 16, 56 };
            Color rowC = it->read ? th.surfaceMuted : th.primarySoft;
            rowC.a = (unsigned char)(255 * notifPanelT);
            DrawRectangleRounded(row, 0.30f, 6, rowC);

            // Type dot
            Color dc;
            switch (it->kind) {
                case NotificationKind::GRADE_ADDED:   dc = th.success; break;
                case NotificationKind::GRADE_UPDATED: dc = th.warning; break;
                case NotificationKind::INSIGHT:       dc = th.accent;  break;
                default:                              dc = th.primary; break;
            }
            DrawCircleV({row.x + 14, row.y + 14}, 4.0f, dc);

            std::string text = UI::Ellipsize(f, it->text, row.width - 36, 12.5f);
            Color tc = th.textPrimary; tc.a = (unsigned char)(255 * notifPanelT);
            UI::DrawTextLeft(f, text.c_str(), {row.x + 26, row.y + 8}, 12.5f, tc);

            std::string when = Models::RelativeTime(it->createdAt);
            Color wc = th.textMuted; wc.a = (unsigned char)(255 * notifPanelT);
            UI::DrawTextLeft(f, when.c_str(), {row.x + 26, row.y + 32}, 11.0f, wc);

            py += 64;
        }
        if (shown == 0) {
            Color tc = th.textMuted; tc.a = (unsigned char)(255 * notifPanelT);
            UI::DrawTextCenter(f,
                               I18n::Current() == Lang::BG ? "Няма нови известия." : "All caught up.",
                               {panel.x, panel.y + panel.height * 0.5f - 10, panel.width, 20},
                               13.0f, tc);
        }
        EndScissorMode();
    }
}

void App::Update() {
    float dt = GetFrameTime();
    if (dt > 0.1f) dt = 0.1f; // clamp big stalls

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    particles.Update(dt, sw, sh);
    toasts.Update(dt);

    // Transition timing
    if (transitioning) {
        transitionT += dt * 2.4f;
        if (!transitionSwapped && transitionT >= 0.5f) {
            // mid-point: hide old, show new
            if (Screen* cur = GetScreen(currentId)) cur->OnHide(*this);
            currentId = pendingId;
            if (Screen* now = GetScreen(currentId)) now->OnShow(*this);
            transitionSwapped = true;
        }
        if (transitionT >= 1.0f) {
            transitionT = 1.0f;
            transitioning = false;
        }
    }

    // Always update header (even mid-transition - looks more alive)
    UpdateHeader(dt, mouse);

    // Update screens. During a transition we still update (so animations of
    // outgoing/incoming screens keep ticking) but we don't deliver mouse to
    // the outgoing screen - feed it an off-screen position so clicks don't
    // double-fire as the new screen appears.
    Vector2 effectiveMouse = mouse;
    if (transitioning) effectiveMouse = { -10000, -10000 };

    if (Screen* cur = GetScreen(currentId)) cur->Update(*this, dt, effectiveMouse);
}

void App::Draw() {
    const Theme& th = ThemeMgr::Current();

    // Background gradient
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, th.background);
    UI::DrawVerticalGradient({0, 0, (float)sw, (float)sh}, th.background, th.backgroundAlt);

    // Animated particles behind everything
    particles.Draw(sw, sh);

    // Current screen
    if (Screen* cur = GetScreen(currentId)) cur->Draw(*this);

    // Transition fade overlay (full-screen alpha pulse around mid)
    if (transitioning) {
        // alpha = 1 at t=0.5, 0 at endpoints — symmetric peak
        float a = 1.0f - std::fabs(transitionT - 0.5f) * 2.0f; // 0..1..0
        a = Easing::OutCubic(Easing::Clamp01(a));
        Color ov = th.background;
        ov.a = (unsigned char)(220 * a);
        DrawRectangle(0, 0, sw, sh, ov);
    }

    // Header + toasts on top of everything
    DrawHeader(GetFrameTime());
    toasts.Draw(ThemeMgr::GetFont(), (float)sw, (float)sh);
}

// Student SIS - Bulgarian university student information system
// Entry point. Owns the raylib window lifecycle and delegates everything else
// to the App controller in app.{h,cpp}.

#include "raylib.h"
#include "app.h"

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    // Smooth visuals: 4x MSAA for rounded edges, vsync to avoid tearing,
    // high-DPI for crisp text on retina/4K displays. FLAG_WINDOW_RESIZABLE
    // lets the app respond gracefully to window resizes.
    SetConfigFlags(FLAG_MSAA_4X_HINT |
                   FLAG_VSYNC_HINT |
                   FLAG_WINDOW_HIGHDPI |
                   FLAG_WINDOW_RESIZABLE);

    InitWindow(1280, 800, "FutureCode - Student Information System");
    SetTargetFPS(60);
    SetExitKey(0); // don't let ESC kill the window - we use it for modal cancel

    // Minimum window size (rendering assumes ~1024 wide for layouts to look right)
    SetWindowMinSize(1024, 700);

    {
        App app;
        app.Init();

        while (!WindowShouldClose() && app.IsRunning()) {
            app.Update();

            BeginDrawing();
            app.Draw();
            EndDrawing();
        }

        app.Shutdown();
    }

    CloseWindow();
    return 0;
}

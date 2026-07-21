#include "game.hpp"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

namespace {
Game game;

void UpdateDrawFrame() {
    game.Update(GetFrameTime());
    game.Draw();
}
} // namespace

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Echo Glass - One Game a Week Jam");
    InitAudioDevice();
    SetTargetFPS(60);

    game.Init();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    game.Shutdown();
    CloseAudioDevice();
    CloseWindow();
#endif

    return 0;
}

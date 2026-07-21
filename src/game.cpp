#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr float PlayerSpeed = 230.0f;
constexpr float InteractionRange = 78.0f;
constexpr float PlaybackToneTime = 0.34f;
constexpr float PlaybackGapTime = 0.18f;
constexpr float PlaybackStepLength = PlaybackToneTime + PlaybackGapTime;
constexpr float EnemySpawnTime = 1.15f;
constexpr float ProjectileSpeed = 590.0f;
constexpr float ProjectileLife = 1.85f;
constexpr float FireDelay = 0.22f;
constexpr int EnemyHealth = 3;

Color FadeTo(Color color, float alpha) {
    return ColorAlpha(color, std::clamp(alpha, 0.0f, 1.0f));
}

Vector2 RectCenter(Rectangle rectangle) {
    return {rectangle.x + rectangle.width * 0.5f, rectangle.y + rectangle.height * 0.5f};
}

float Distance(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

Vector2 NormalizeOr(Vector2 vector, Vector2 fallback = {0.0f, 1.0f}) {
    const float length = std::sqrt(vector.x * vector.x + vector.y * vector.y);
    if (length <= 0.0001f) {
        return fallback;
    }
    return {vector.x / length, vector.y / length};
}

Vector2 RotatePoint(Vector2 point, float angle) {
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    return {
        point.x * cosine - point.y * sine,
        point.x * sine + point.y * cosine
    };
}

bool CircleIntersectsRectangle(Vector2 center, float radius, Rectangle rectangle) {
    const float nearestX = std::clamp(center.x, rectangle.x, rectangle.x + rectangle.width);
    const float nearestY = std::clamp(center.y, rectangle.y, rectangle.y + rectangle.height);
    const float dx = center.x - nearestX;
    const float dy = center.y - nearestY;
    return dx * dx + dy * dy <= radius * radius;
}
} // namespace

void Game::Init() {
    pillars_ = {{
        {{250.0f, 210.0f, 74.0f, 74.0f}, 329.63f, Color{58, 214, 255, 255}, '1', 0.0f},
        {{450.0f, 370.0f, 74.0f, 74.0f}, 440.00f, Color{126, 235, 94, 255}, '2', 0.0f},
        {{650.0f, 210.0f, 74.0f, 74.0f}, 554.37f, Color{191, 94, 255, 255}, '3', 0.0f},
        {{850.0f, 370.0f, 74.0f, 74.0f}, 659.25f, Color{255, 178, 57, 255}, '4', 0.0f},
        {{980.0f, 205.0f, 68.0f, 68.0f}, 783.99f, Color{255, 100, 170, 255}, '5', 0.0f},
        {{1050.0f, 370.0f, 68.0f, 68.0f}, 987.77f, Color{255, 103, 79, 255}, '6', 0.0f},
        {{1100.0f, 205.0f, 64.0f, 64.0f}, 1174.66f, Color{255, 232, 92, 255}, '7', 0.0f}
    }};

    audioReady_ = IsAudioDeviceReady();
    if (audioReady_) {
        for (std::size_t i = 0; i < pillars_.size(); ++i) {
            tones_[i] = MakeTone(pillars_[i].frequency, 0.23f);
        }
        successTone_ = MakeTone(880.0f, 0.55f, 0.42f);
        failTone_ = MakeTone(155.0f, 0.28f, 0.30f);
        shotTone_ = MakeTone(1046.50f, 0.10f, 0.24f);
    }

    currentLevel_ = 1;
    ConfigureLevel();
    ResetRound();
    scene_ = Scene::Intro;
}

void Game::Shutdown() {
    if (!audioReady_) {
        return;
    }

    for (Sound& tone : tones_) {
        UnloadSound(tone);
    }
    UnloadSound(successTone_);
    UnloadSound(failTone_);
    UnloadSound(shotTone_);
}

void Game::ConfigureLevel() {
    enemyCount_ = 0;

    if (currentLevel_ == 1) {
        pillarCount_ = 4;
        sequenceLength_ = 4;
        sequence_ = {2, 0, 3, 1, 4, 5, 6};

        pillars_[0].body = {250.0f, 210.0f, 74.0f, 74.0f};
        pillars_[1].body = {450.0f, 370.0f, 74.0f, 74.0f};
        pillars_[2].body = {650.0f, 210.0f, 74.0f, 74.0f};
        pillars_[3].body = {850.0f, 370.0f, 74.0f, 74.0f};
        return;
    }

    if (currentLevel_ == 2) {
        pillarCount_ = 5;
        sequenceLength_ = 5;
        sequence_ = {4, 1, 3, 0, 2, 5, 6};

        pillars_[0].body = {180.0f, 205.0f, 68.0f, 68.0f};
        pillars_[1].body = {380.0f, 380.0f, 68.0f, 68.0f};
        pillars_[2].body = {580.0f, 205.0f, 68.0f, 68.0f};
        pillars_[3].body = {780.0f, 380.0f, 68.0f, 68.0f};
        pillars_[4].body = {980.0f, 205.0f, 68.0f, 68.0f};
        return;
    }

    if (currentLevel_ == 3) {
        pillarCount_ = 6;
        sequenceLength_ = 6;
        sequence_ = {1, 5, 0, 4, 2, 3, 6};

        pillars_[0].body = {130.0f, 190.0f, 66.0f, 66.0f};
        pillars_[1].body = {310.0f, 370.0f, 66.0f, 66.0f};
        pillars_[2].body = {490.0f, 200.0f, 66.0f, 66.0f};
        pillars_[3].body = {670.0f, 380.0f, 66.0f, 66.0f};
        pillars_[4].body = {850.0f, 200.0f, 66.0f, 66.0f};
        pillars_[5].body = {1030.0f, 370.0f, 66.0f, 66.0f};
        enemyCount_ = 1;
        return;
    }

    // Room four: seven pillars, two hunters, and a mouse-aimed music-note gun.
    pillarCount_ = 7;
    sequenceLength_ = 7;
    sequence_ = {6, 2, 4, 0, 5, 1, 3};

    pillars_[0].body = {120.0f, 170.0f, 62.0f, 62.0f};
    pillars_[1].body = {275.0f, 350.0f, 62.0f, 62.0f};
    pillars_[2].body = {430.0f, 185.0f, 62.0f, 62.0f};
    pillars_[3].body = {590.0f, 365.0f, 62.0f, 62.0f};
    pillars_[4].body = {750.0f, 185.0f, 62.0f, 62.0f};
    pillars_[5].body = {905.0f, 350.0f, 62.0f, 62.0f};
    pillars_[6].body = {1060.0f, 170.0f, 62.0f, 62.0f};
    enemyCount_ = 2;
}

void Game::ResetRound() {
    player_ = {640.0f, 620.0f};
    aimDirection_ = {0.0f, -1.0f};
    health_ = MaxHealth;
    doorUnlocked_ = false;
    playbackActive_ = false;
    playbackIndex_ = 0;
    playerProgress_ = 0;
    playbackStepTimer_ = 0.0f;
    replayTimer_ = 1.0f;
    messageTimer_ = 0.0f;
    messageKind_ = MessageKind::None;
    wrongFlash_ = 0.0f;
    damageFlash_ = 0.0f;
    damageCooldown_ = 0.0f;
    fireCooldown_ = 0.0f;

    for (std::size_t i = 0; i < enemies_.size(); ++i) {
        Enemy& enemy = enemies_[i];
        enemy.active = static_cast<int>(i) < enemyCount_;
        enemy.position = i == 0 ? Vector2{1150.0f, 595.0f} : Vector2{130.0f, 595.0f};
        enemy.radius = 20.0f;
        enemy.speed = currentLevel_ == 4 ? 118.0f + static_cast<float>(i) * 8.0f : 108.0f;
        enemy.spawnTimer = enemy.active ? EnemySpawnTime + static_cast<float>(i) * 0.65f : 0.0f;
        enemy.hitFlash = 0.0f;
        enemy.health = EnemyHealth;
    }

    for (Projectile& projectile : projectiles_) {
        projectile.active = false;
    }

    for (Pillar& pillar : pillars_) {
        pillar.pulse = 0.0f;
    }
}

void Game::Update(float dt) {
    dt = std::min(dt, 0.05f);
    totalTime_ += dt;

    if (messageTimer_ > 0.0f) {
        messageTimer_ = std::max(0.0f, messageTimer_ - dt);
        if (messageTimer_ == 0.0f) {
            messageKind_ = MessageKind::None;
        }
    }

    wrongFlash_ = std::max(0.0f, wrongFlash_ - dt);
    damageFlash_ = std::max(0.0f, damageFlash_ - dt);
    damageCooldown_ = std::max(0.0f, damageCooldown_ - dt);
    fireCooldown_ = std::max(0.0f, fireCooldown_ - dt);

    for (Pillar& pillar : pillars_) {
        pillar.pulse = std::max(0.0f, pillar.pulse - dt * 2.8f);
    }
    for (Enemy& enemy : enemies_) {
        enemy.hitFlash = std::max(0.0f, enemy.hitFlash - dt);
    }

    if (scene_ == Scene::Intro) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            currentLevel_ = 1;
            ConfigureLevel();
            ResetRound();
            scene_ = Scene::Playing;
        }
        return;
    }

    if (scene_ == Scene::LevelComplete) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ++currentLevel_;
            ConfigureLevel();
            ResetRound();
            scene_ = Scene::Playing;
        }
        return;
    }

    if (scene_ == Scene::Won) {
        if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER)) {
            currentLevel_ = 1;
            ConfigureLevel();
            ResetRound();
            scene_ = Scene::Playing;
        }
        return;
    }

    if (IsKeyPressed(KEY_R)) {
        ResetRound();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    aimDirection_ = NormalizeOr({mouse.x - player_.x, mouse.y - player_.y}, aimDirection_);

    UpdatePlayback(dt);
    UpdatePlayer(dt);
    UpdateEnemies(dt);
    UpdateProjectiles(dt);

    if (currentLevel_ == 4 && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && fireCooldown_ <= 0.0f) {
        ShootNote();
    }

    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE)) {
        TryInteract();
    }
}

void Game::StartPlayback() {
    playbackActive_ = true;
    playbackIndex_ = 0;
    playbackStepTimer_ = 0.0f;
    ActivatePillar(static_cast<std::size_t>(sequence_[0]), false);
}

void Game::UpdatePlayback(float dt) {
    if (doorUnlocked_) {
        playbackActive_ = false;
        return;
    }

    if (!playbackActive_) {
        replayTimer_ -= dt;
        if (replayTimer_ <= 0.0f) {
            StartPlayback();
        }
        return;
    }

    playbackStepTimer_ += dt;
    if (playbackStepTimer_ < PlaybackStepLength) {
        return;
    }

    playbackStepTimer_ -= PlaybackStepLength;
    ++playbackIndex_;

    if (playbackIndex_ >= sequenceLength_) {
        playbackActive_ = false;
        replayTimer_ = 5.2f;
        return;
    }

    ActivatePillar(static_cast<std::size_t>(sequence_[playbackIndex_]), false);
}

void Game::UpdatePlayer(float dt) {
    Vector2 direction{};
    direction.x = static_cast<float>(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
                - static_cast<float>(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    direction.y = static_cast<float>(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
                - static_cast<float>(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));

    direction = NormalizeOr(direction, {0.0f, 0.0f});

    Vector2 next = player_;
    next.x += direction.x * PlayerSpeed * dt;
    if (CanPlayerOccupy(next)) {
        player_.x = next.x;
    }

    next = player_;
    next.y += direction.y * PlayerSpeed * dt;
    if (CanPlayerOccupy(next)) {
        player_.y = next.y;
    }
}

void Game::UpdateEnemies(float dt) {
    for (std::size_t i = 0; i < enemies_.size(); ++i) {
        Enemy& enemy = enemies_[i];
        if (!enemy.active) {
            continue;
        }

        if (enemy.spawnTimer > 0.0f) {
            enemy.spawnTimer = std::max(0.0f, enemy.spawnTimer - dt);
            continue;
        }

        Vector2 direction = NormalizeOr({player_.x - enemy.position.x, player_.y - enemy.position.y});
        const float wobble = std::sin(totalTime_ * (2.55f + static_cast<float>(i) * 0.25f) + static_cast<float>(i) * Pi)
                           * 0.24f;
        direction = RotatePoint(direction, wobble);

        if (enemyCount_ > 1) {
            const Enemy& other = enemies_[1U - i];
            if (other.active) {
                const float separation = Distance(enemy.position, other.position);
                if (separation < enemy.radius * 3.4f) {
                    const Vector2 away = NormalizeOr({enemy.position.x - other.position.x,
                                                      enemy.position.y - other.position.y}, direction);
                    direction = NormalizeOr({direction.x + away.x * 0.85f,
                                             direction.y + away.y * 0.85f}, direction);
                }
            }
        }

        bool moved = false;
        Vector2 next = enemy.position;
        next.x += direction.x * enemy.speed * dt;
        if (CanEnemyOccupy(next, enemy.radius)) {
            enemy.position.x = next.x;
            moved = true;
        }

        next = enemy.position;
        next.y += direction.y * enemy.speed * dt;
        if (CanEnemyOccupy(next, enemy.radius)) {
            enemy.position.y = next.y;
            moved = true;
        }

        if (!moved) {
            const Vector2 tangent{-direction.y, direction.x};
            next = {
                enemy.position.x + tangent.x * enemy.speed * dt,
                enemy.position.y + tangent.y * enemy.speed * dt
            };
            if (CanEnemyOccupy(next, enemy.radius)) {
                enemy.position = next;
            }
        }

        ResolveEnemyContact(enemy);
    }
}

void Game::UpdateProjectiles(float dt) {
    for (Projectile& projectile : projectiles_) {
        if (!projectile.active) {
            continue;
        }

        projectile.life -= dt;
        projectile.position.x += projectile.velocity.x * dt;
        projectile.position.y += projectile.velocity.y * dt;

        if (projectile.life <= 0.0f || ProjectileHitsObstacle(projectile.position, projectile.radius)) {
            projectile.active = false;
            continue;
        }

        for (Enemy& enemy : enemies_) {
            if (!enemy.active || enemy.spawnTimer > 0.0f) {
                continue;
            }

            if (Distance(projectile.position, enemy.position) > projectile.radius + enemy.radius) {
                continue;
            }

            projectile.active = false;
            --enemy.health;
            enemy.hitFlash = 0.18f;
            messageTimer_ = 0.45f;
            messageKind_ = MessageKind::EnemyHit;

            const Vector2 push = NormalizeOr(projectile.velocity);
            const Vector2 pushed{
                enemy.position.x + push.x * 18.0f,
                enemy.position.y + push.y * 18.0f
            };
            if (CanEnemyOccupy(pushed, enemy.radius)) {
                enemy.position = pushed;
            }

            if (enemy.health <= 0) {
                enemy.active = false;
                messageTimer_ = 0.85f;
                messageKind_ = MessageKind::EnemyDestroyed;
                if (audioReady_) {
                    PlaySound(successTone_);
                }
            }
            break;
        }
    }
}

void Game::ResolveEnemyContact(Enemy& enemy) {
    if (!enemy.active || enemy.spawnTimer > 0.0f || damageCooldown_ > 0.0f) {
        return;
    }

    if (Distance(player_, enemy.position) > PlayerRadius + enemy.radius + 2.0f) {
        return;
    }

    --health_;
    damageCooldown_ = 0.85f;
    damageFlash_ = 0.38f;
    messageTimer_ = 0.8f;
    messageKind_ = MessageKind::Hurt;

    if (audioReady_) {
        PlaySound(failTone_);
    }

    const Vector2 away = NormalizeOr({player_.x - enemy.position.x, player_.y - enemy.position.y});
    const Vector2 pushed{
        player_.x + away.x * 42.0f,
        player_.y + away.y * 42.0f
    };
    if (CanPlayerOccupy(pushed)) {
        player_ = pushed;
    }

    if (health_ <= 0) {
        ResetRound();
        damageFlash_ = 0.75f;
        messageTimer_ = 1.8f;
        messageKind_ = MessageKind::Respawned;
    }
}

void Game::ShootNote() {
    Projectile* slot = nullptr;
    for (Projectile& projectile : projectiles_) {
        if (!projectile.active) {
            slot = &projectile;
            break;
        }
    }

    if (slot == nullptr) {
        return;
    }

    slot->active = true;
    slot->radius = 6.0f;
    slot->life = ProjectileLife;
    slot->position = {
        player_.x + aimDirection_.x * (PlayerRadius + 13.0f),
        player_.y + aimDirection_.y * (PlayerRadius + 13.0f)
    };
    slot->velocity = {
        aimDirection_.x * ProjectileSpeed,
        aimDirection_.y * ProjectileSpeed
    };
    fireCooldown_ = FireDelay;

    if (audioReady_) {
        PlaySound(shotTone_);
    }
}

void Game::TryInteract() {
    if (NearDoor()) {
        if (doorUnlocked_) {
            scene_ = currentLevel_ < LevelCount ? Scene::LevelComplete : Scene::Won;
            if (audioReady_) {
                PlaySound(successTone_);
            }
        } else {
            messageTimer_ = 1.2f;
            messageKind_ = MessageKind::DoorLocked;
        }
        return;
    }

    const int nearby = NearbyPillar();
    if (nearby >= 0) {
        ActivatePillar(static_cast<std::size_t>(nearby), true);
    }
}

void Game::ActivatePillar(std::size_t index, bool playerActivated) {
    Pillar& pillar = pillars_[index];
    pillar.pulse = 1.0f;

    if (audioReady_) {
        PlaySound(tones_[index]);
    }

    if (!playerActivated || doorUnlocked_) {
        return;
    }

    const int expected = sequence_[playerProgress_];
    if (static_cast<int>(index) == expected) {
        ++playerProgress_;
        messageTimer_ = 0.55f;
        messageKind_ = MessageKind::Correct;

        if (playerProgress_ >= sequenceLength_) {
            playerProgress_ = sequenceLength_;
            doorUnlocked_ = true;
            playbackActive_ = false;
            messageTimer_ = 2.0f;
            messageKind_ = MessageKind::Unlocked;
            if (audioReady_) {
                PlaySound(successTone_);
            }
        }
        return;
    }

    playerProgress_ = 0;
    playbackActive_ = false;
    replayTimer_ = 0.8f;
    wrongFlash_ = 0.45f;
    messageTimer_ = 1.0f;
    messageKind_ = MessageKind::Wrong;
    if (audioReady_) {
        PlaySound(failTone_);
    }
}

bool Game::CanPlayerOccupy(Vector2 position) const {
    const Rectangle playerBounds{
        position.x - PlayerRadius,
        position.y - PlayerRadius,
        PlayerRadius * 2.0f,
        PlayerRadius * 2.0f
    };

    const Rectangle innerRoom{
        room_.x + 12.0f,
        room_.y + 12.0f,
        room_.width - 24.0f,
        room_.height - 24.0f
    };

    if (playerBounds.x < innerRoom.x ||
        playerBounds.y < innerRoom.y ||
        playerBounds.x + playerBounds.width > innerRoom.x + innerRoom.width ||
        playerBounds.y + playerBounds.height > innerRoom.y + innerRoom.height) {
        return false;
    }

    for (int i = 0; i < pillarCount_; ++i) {
        const Pillar& pillar = pillars_[static_cast<std::size_t>(i)];
        const Rectangle expanded{
            pillar.body.x - PlayerRadius,
            pillar.body.y - PlayerRadius,
            pillar.body.width + PlayerRadius * 2.0f,
            pillar.body.height + PlayerRadius * 2.0f
        };
        if (CheckCollisionPointRec(position, expanded)) {
            return false;
        }
    }

    if (!doorUnlocked_) {
        const Rectangle expandedDoor{
            glassDoor_.x - PlayerRadius,
            glassDoor_.y - PlayerRadius,
            glassDoor_.width + PlayerRadius * 2.0f,
            glassDoor_.height + PlayerRadius * 2.0f
        };
        if (CheckCollisionPointRec(position, expandedDoor)) {
            return false;
        }
    }

    return true;
}

bool Game::CanEnemyOccupy(Vector2 position, float radius) const {
    const Rectangle innerRoom{
        room_.x + 12.0f,
        room_.y + 12.0f,
        room_.width - 24.0f,
        room_.height - 24.0f
    };

    if (position.x - radius < innerRoom.x ||
        position.y - radius < innerRoom.y ||
        position.x + radius > innerRoom.x + innerRoom.width ||
        position.y + radius > innerRoom.y + innerRoom.height) {
        return false;
    }

    for (int i = 0; i < pillarCount_; ++i) {
        const Rectangle body = pillars_[static_cast<std::size_t>(i)].body;
        const Rectangle expanded{
            body.x - radius,
            body.y - radius,
            body.width + radius * 2.0f,
            body.height + radius * 2.0f
        };
        if (CheckCollisionPointRec(position, expanded)) {
            return false;
        }
    }

    if (!doorUnlocked_) {
        const Rectangle expandedDoor{
            glassDoor_.x - radius,
            glassDoor_.y - radius,
            glassDoor_.width + radius * 2.0f,
            glassDoor_.height + radius * 2.0f
        };
        if (CheckCollisionPointRec(position, expandedDoor)) {
            return false;
        }
    }

    return true;
}

bool Game::ProjectileHitsObstacle(Vector2 position, float radius) const {
    const Rectangle innerRoom{
        room_.x + 10.0f,
        room_.y + 10.0f,
        room_.width - 20.0f,
        room_.height - 20.0f
    };

    if (position.x - radius < innerRoom.x ||
        position.y - radius < innerRoom.y ||
        position.x + radius > innerRoom.x + innerRoom.width ||
        position.y + radius > innerRoom.y + innerRoom.height) {
        return true;
    }

    for (int i = 0; i < pillarCount_; ++i) {
        if (CircleIntersectsRectangle(position, radius, pillars_[static_cast<std::size_t>(i)].body)) {
            return true;
        }
    }

    return !doorUnlocked_ && CircleIntersectsRectangle(position, radius, glassDoor_);
}

int Game::NearbyPillar() const {
    int closest = -1;
    float closestDistance = InteractionRange;

    for (int i = 0; i < pillarCount_; ++i) {
        const float distance = Distance(player_, RectCenter(pillars_[static_cast<std::size_t>(i)].body));
        if (distance < closestDistance) {
            closestDistance = distance;
            closest = i;
        }
    }

    return closest;
}

bool Game::NearDoor() const {
    return Distance(player_, RectCenter(glassDoor_)) < 78.0f;
}

void Game::Draw() const {
    BeginDrawing();
    ClearBackground(Color{7, 8, 18, 255});

    DrawRoom();
    DrawDoor();
    DrawPillars();
    DrawProjectiles();
    DrawEnemies();
    DrawPlayer();
    DrawHud();

    if (wrongFlash_ > 0.0f) {
        DrawRectangle(0, 0, ScreenWidth, ScreenHeight, FadeTo(RED, wrongFlash_ * 0.28f));
    }
    if (damageFlash_ > 0.0f) {
        DrawRectangle(0, 0, ScreenWidth, ScreenHeight, FadeTo(RED, damageFlash_ * 0.32f));
    }

    if (scene_ == Scene::Intro) {
        DrawIntro();
    } else if (scene_ == Scene::LevelComplete) {
        DrawLevelComplete();
    } else if (scene_ == Scene::Won) {
        DrawWin();
    }

    EndDrawing();
}

void Game::DrawRoom() const {
    DrawRectangleGradientV(0, 0, ScreenWidth, ScreenHeight,
                           Color{8, 12, 27, 255}, Color{23, 12, 35, 255});
    DrawRectangleRec(room_, Color{12, 20, 35, 255});

    for (int x = static_cast<int>(room_.x); x <= static_cast<int>(room_.x + room_.width); x += 40) {
        DrawLine(x, static_cast<int>(room_.y), x, static_cast<int>(room_.y + room_.height), Color{23, 53, 68, 150});
    }
    for (int y = static_cast<int>(room_.y); y <= static_cast<int>(room_.y + room_.height); y += 40) {
        DrawLine(static_cast<int>(room_.x), y, static_cast<int>(room_.x + room_.width), y, Color{23, 53, 68, 150});
    }

    DrawRectangleLinesEx(room_, 8.0f, Color{74, 31, 106, 255});
    DrawRectangleLinesEx({room_.x + 8.0f, room_.y + 8.0f, room_.width - 16.0f, room_.height - 16.0f},
                         2.0f, Color{36, 188, 218, 100});

    for (int i = 0; i < 12; ++i) {
        const float x = 95.0f + static_cast<float>((i * 103) % 1090);
        const float y = 115.0f + static_cast<float>((i * 73) % 510);
        DrawTriangle({x, y}, {x + 6.0f, y + 18.0f}, {x - 5.0f, y + 14.0f}, Color{76, 220, 255, 38});
    }
}

void Game::DrawDoor() const {
    const Color glow = doorUnlocked_ ? Color{96, 255, 145, 255} : Color{74, 224, 255, 255};
    const float pulse = 0.5f + 0.5f * std::sin(totalTime_ * 3.0f);

    DrawRectangle(static_cast<int>(glassDoor_.x - 8.0f), static_cast<int>(glassDoor_.y - 6.0f),
                  static_cast<int>(glassDoor_.width + 16.0f), static_cast<int>(glassDoor_.height + 12.0f),
                  FadeTo(glow, 0.12f + pulse * 0.08f));
    DrawRectangleRec(glassDoor_, Color{18, 44, 59, 210});
    DrawRectangleLinesEx(glassDoor_, 4.0f, glow);

    for (int i = 1; i < 5; ++i) {
        const float x = glassDoor_.x + glassDoor_.width * static_cast<float>(i) / 5.0f;
        DrawLineEx({x, glassDoor_.y + 3.0f}, {x - 9.0f, glassDoor_.y + glassDoor_.height - 3.0f},
                   1.0f, FadeTo(glow, 0.45f));
    }

    const char* label = doorUnlocked_ ? "GLASS DOOR: OPEN" : "GLASS DOOR: LOCKED";
    const int labelWidth = MeasureText(label, 18);
    DrawText(label, static_cast<int>(glassDoor_.x + glassDoor_.width * 0.5f - labelWidth * 0.5f), 42, 18, glow);
}

void Game::DrawPillars() const {
    for (int i = 0; i < pillarCount_; ++i) {
        const Pillar& pillar = pillars_[static_cast<std::size_t>(i)];
        const Vector2 center = RectCenter(pillar.body);
        const bool expectedNow = !doorUnlocked_ && playerProgress_ < sequenceLength_
                              && sequence_[playerProgress_] == i;
        const float glow = std::max(pillar.pulse, expectedNow ? 0.08f : 0.0f);

        DrawRectangleRounded({pillar.body.x - 8.0f, pillar.body.y - 8.0f,
                              pillar.body.width + 16.0f, pillar.body.height + 16.0f},
                             0.18f, 6, FadeTo(pillar.color, glow * 0.22f));
        DrawRectangleRounded(pillar.body, 0.18f, 6, Color{27, 24, 43, 255});
        DrawRectangleRoundedLinesEx(pillar.body, 0.18f, 6, 3.0f,
                                    FadeTo(pillar.color, 0.65f + pillar.pulse * 0.35f));

        const float ringRadius = 16.0f + pillar.pulse * 17.0f;
        DrawCircleV(center, ringRadius, FadeTo(pillar.color, 0.18f + pillar.pulse * 0.35f));
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), ringRadius, pillar.color);

        const char label[2] = {pillar.label, '\0'};
        const int textWidth = MeasureText(label, 30);
        DrawText(label, static_cast<int>(center.x - textWidth * 0.5f), static_cast<int>(center.y - 15.0f),
                 30, RAYWHITE);
    }
}

void Game::DrawEnemies() const {
    for (std::size_t i = 0; i < enemies_.size(); ++i) {
        const Enemy& enemy = enemies_[i];
        if (!enemy.active) {
            continue;
        }

        float scale = 1.0f;
        if (enemy.spawnTimer > 0.0f) {
            scale = std::clamp(1.0f - enemy.spawnTimer / EnemySpawnTime, 0.08f, 1.0f);
            const float ring = enemy.radius + (1.0f - scale) * 45.0f;
            DrawCircleLines(static_cast<int>(enemy.position.x), static_cast<int>(enemy.position.y),
                            ring, FadeTo(Color{255, 74, 112, 255}, 1.0f - scale));
        }

        const float radius = enemy.radius * scale;
        const float angle = std::atan2(player_.y - enemy.position.y, player_.x - enemy.position.x) + Pi * 0.5f;
        const Vector2 topOffset = RotatePoint({0.0f, -radius * 1.18f}, angle);
        const Vector2 leftOffset = RotatePoint({-radius, radius * 0.82f}, angle);
        const Vector2 rightOffset = RotatePoint({radius, radius * 0.82f}, angle);

        const Vector2 top{enemy.position.x + topOffset.x, enemy.position.y + topOffset.y};
        const Vector2 left{enemy.position.x + leftOffset.x, enemy.position.y + leftOffset.y};
        const Vector2 right{enemy.position.x + rightOffset.x, enemy.position.y + rightOffset.y};

        const float pulse = 0.5f + 0.5f * std::sin(totalTime_ * 7.0f + static_cast<float>(i));
        const Color body = enemy.hitFlash > 0.0f ? Color{255, 246, 184, 255} : Color{235, 47, 91, 255};
        DrawCircleV(enemy.position, radius + 9.0f, FadeTo(Color{255, 50, 95, 255}, 0.12f + pulse * 0.08f));
        DrawTriangle(top, left, right, body);
        DrawTriangleLines(top, left, right, Color{255, 176, 194, 255});
        DrawCircleV(enemy.position, std::max(2.0f, radius * 0.18f), Color{255, 241, 196, 255});

        if (currentLevel_ == 4 && enemy.spawnTimer <= 0.0f) {
            for (int hp = 0; hp < EnemyHealth; ++hp) {
                const Color pip = hp < enemy.health ? Color{255, 110, 141, 255} : Color{68, 31, 48, 255};
                DrawCircle(static_cast<int>(enemy.position.x - 10.0f + static_cast<float>(hp) * 10.0f),
                           static_cast<int>(enemy.position.y - radius - 13.0f), 3.0f, pip);
            }
        }
    }
}

void Game::DrawProjectiles() const {
    for (const Projectile& projectile : projectiles_) {
        if (!projectile.active) {
            continue;
        }

        const Vector2 direction = NormalizeOr(projectile.velocity, {1.0f, 0.0f});
        const Vector2 normal{-direction.y, direction.x};
        const Vector2 stemStart{
            projectile.position.x + normal.x * 3.0f,
            projectile.position.y + normal.y * 3.0f
        };
        const Vector2 stemEnd{
            stemStart.x - direction.x * 17.0f,
            stemStart.y - direction.y * 17.0f
        };
        const Vector2 flagEnd{
            stemEnd.x + normal.x * 8.0f + direction.x * 5.0f,
            stemEnd.y + normal.y * 8.0f + direction.y * 5.0f
        };

        DrawCircleV(projectile.position, projectile.radius + 5.0f, Color{84, 229, 255, 45});
        DrawCircleV(projectile.position, projectile.radius, Color{113, 239, 255, 255});
        DrawLineEx(stemStart, stemEnd, 3.0f, Color{187, 250, 255, 255});
        DrawLineEx(stemEnd, flagEnd, 3.0f, Color{187, 250, 255, 255});
    }
}

void Game::DrawPlayer() const {
    const Color playerColor = damageCooldown_ > 0.0f && static_cast<int>(totalTime_ * 14.0f) % 2 == 0
        ? Color{255, 126, 139, 255}
        : Color{244, 246, 255, 255};

    const Vector2 barrelStart{
        player_.x + aimDirection_.x * 8.0f,
        player_.y + aimDirection_.y * 8.0f
    };
    const Vector2 barrelEnd{
        player_.x + aimDirection_.x * 27.0f,
        player_.y + aimDirection_.y * 27.0f
    };

    if (currentLevel_ == 4) {
        DrawLineEx(barrelStart, barrelEnd, 7.0f, Color{96, 225, 255, 255});
        DrawCircleV(barrelEnd, 4.0f, Color{210, 252, 255, 255});
    }

    DrawCircleV(player_, PlayerRadius + 5.0f, Color{67, 222, 255, 50});
    DrawCircleV(player_, PlayerRadius, playerColor);
    DrawCircleV({player_.x + aimDirection_.x * 5.0f, player_.y + aimDirection_.y * 5.0f},
                3.0f, Color{25, 34, 55, 255});
}

void Game::DrawHud() const {
    DrawRectangle(0, 0, ScreenWidth, 32, Color{5, 7, 14, 235});
    DrawText("ECHO GLASS", 18, 7, 20, Color{77, 225, 255, 255});
    DrawText(TextFormat("LEVEL %d / %d", currentLevel_, LevelCount), 160, 9, 15, Color{255, 216, 91, 255});

    const char* controls = currentLevel_ == 4
        ? "Move: WASD / arrows   Aim: mouse   Shoot: left click   Interact: E / Space   Restart: R"
        : "Move: WASD / arrows   Interact: E or Space   Restart: R";
    const int controlsWidth = MeasureText(controls, 14);
    DrawText(controls, ScreenWidth - controlsWidth - 18, 9, 14, Color{190, 196, 215, 255});

    const char* objective = nullptr;
    if (doorUnlocked_) {
        objective = "The pattern worked. Reach the glass door.";
    } else if (currentLevel_ == 1) {
        objective = "Repeat the four-note echo.";
    } else if (currentLevel_ == 2) {
        objective = "Repeat the longer five-note echo.";
    } else if (currentLevel_ == 3) {
        objective = "Repeat six notes while avoiding the triangle.";
    } else {
        objective = "Repeat seven notes. Aim with the mouse and shoot both hunters.";
    }
    const int objectiveWidth = MeasureText(objective, 16);
    DrawText(objective, ScreenWidth / 2 - objectiveWidth / 2, 691, 16,
             doorUnlocked_ ? Color{101, 255, 145, 255} : Color{223, 225, 236, 255});

    DrawHealthBar();

    const int nearbyPillar = NearbyPillar();
    if (scene_ == Scene::Playing && nearbyPillar >= 0) {
        const char* prompt = "[E] strike echo pillar";
        const int width = MeasureText(prompt, 18);
        DrawText(prompt, ScreenWidth / 2 - width / 2, 641, 18,
                 pillars_[static_cast<std::size_t>(nearbyPillar)].color);
    } else if (scene_ == Scene::Playing && NearDoor()) {
        const char* prompt = doorUnlocked_ ? "[E] escape" : "[E] inspect locked door";
        const int width = MeasureText(prompt, 18);
        DrawText(prompt, ScreenWidth / 2 - width / 2, 641, 18, doorUnlocked_ ? GREEN : SKYBLUE);
    }

    const float spacing = 34.0f;
    const float startX = static_cast<float>(ScreenWidth) * 0.5f
                       - static_cast<float>(sequenceLength_ - 1) * spacing * 0.5f;
    for (int i = 0; i < sequenceLength_; ++i) {
        const float x = startX + static_cast<float>(i) * spacing;
        Color color = Color{55, 63, 82, 255};
        if (i < playerProgress_) {
            color = Color{90, 240, 141, 255};
        }
        DrawCircle(static_cast<int>(x), 55, 7.0f, color);
    }

    if (messageTimer_ > 0.0f) {
        const char* message = "";
        Color color = RAYWHITE;

        switch (messageKind_) {
        case MessageKind::DoorLocked:
            message = TextFormat("The glass reacts to a %d-note code.", sequenceLength_);
            color = SKYBLUE;
            break;
        case MessageKind::Correct:
            message = "Correct note.";
            color = Color{125, 245, 151, 255};
            break;
        case MessageKind::Wrong:
            message = "Wrong echo - the room repeats the clue.";
            color = Color{255, 103, 121, 255};
            break;
        case MessageKind::Unlocked:
            message = "UNLOCKED";
            color = Color{90, 255, 141, 255};
            break;
        case MessageKind::Hurt:
            message = "Triangle contact: -1 HP";
            color = Color{255, 103, 121, 255};
            break;
        case MessageKind::Respawned:
            message = "0 HP - room restarted.";
            color = Color{255, 103, 121, 255};
            break;
        case MessageKind::EnemyHit:
            message = "The note cracked the triangle.";
            color = Color{113, 239, 255, 255};
            break;
        case MessageKind::EnemyDestroyed:
            message = "Triangle shattered by sound.";
            color = Color{255, 232, 92, 255};
            break;
        case MessageKind::None:
            break;
        }

        const int width = MeasureText(message, 24);
        DrawText(message, ScreenWidth / 2 - width / 2, 108, 24, color);
    }
}

void Game::DrawHealthBar() const {
    const Rectangle panel{16.0f, 686.0f, 238.0f, 27.0f};
    const Rectangle bar{52.0f, 692.0f, 186.0f, 15.0f};
    const float healthRatio = static_cast<float>(health_) / static_cast<float>(MaxHealth);

    DrawRectangleRounded(panel, 0.25f, 5, Color{5, 7, 14, 235});
    DrawRectangleRoundedLinesEx(panel, 0.25f, 5, 2.0f, Color{96, 31, 76, 255});
    DrawText("HP", 23, 691, 16, Color{255, 197, 208, 255});

    DrawRectangleRec(bar, Color{52, 22, 38, 255});
    DrawRectangle(static_cast<int>(bar.x), static_cast<int>(bar.y),
                  static_cast<int>(bar.width * healthRatio), static_cast<int>(bar.height),
                  Color{229, 55, 91, 255});
    DrawRectangleLinesEx(bar, 1.0f, Color{255, 151, 176, 255});

    for (int i = 1; i < MaxHealth; ++i) {
        const float x = bar.x + bar.width * static_cast<float>(i) / static_cast<float>(MaxHealth);
        DrawLine(static_cast<int>(x), static_cast<int>(bar.y),
                 static_cast<int>(x), static_cast<int>(bar.y + bar.height), Color{15, 11, 22, 180});
    }

    const char* value = TextFormat("%d/%d", health_, MaxHealth);
    const int width = MeasureText(value, 13);
    DrawText(value, static_cast<int>(bar.x + bar.width * 0.5f - width * 0.5f), 693, 13, RAYWHITE);
}

void Game::DrawIntro() const {
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{3, 5, 12, 215});
    DrawRectangleRounded({305.0f, 145.0f, 670.0f, 400.0f}, 0.05f, 8, Color{13, 16, 30, 245});
    DrawRectangleRoundedLinesEx({305.0f, 145.0f, 670.0f, 400.0f}, 0.05f, 8, 3.0f, Color{109, 51, 151, 255});

    DrawText("ECHO GLASS", 474, 181, 54, Color{77, 225, 255, 255});
    DrawText("A tiny four-room escape prototype", 463, 241, 20, Color{190, 196, 215, 255});

    DrawText("GENRE", 390, 295, 18, Color{77, 225, 255, 255});
    DrawText("Escape Room", 520, 295, 18, RAYWHITE);
    DrawText("THEME", 390, 329, 18, Color{126, 235, 94, 255});
    DrawText("Glass Door", 520, 329, 18, RAYWHITE);
    DrawText("WILDCARD", 390, 363, 18, Color{191, 94, 255, 255});
    DrawText("A helpful obstacle", 520, 363, 18, RAYWHITE);
    DrawText("INGREDIENT", 390, 397, 18, Color{255, 178, 57, 255});
    DrawText("A repeated sound", 520, 397, 18, RAYWHITE);

    DrawText("The pillars sing the answer. The last room gives you a note gun.", 377, 456, 19,
             Color{220, 223, 235, 255});
    DrawText("Press ENTER / SPACE / click to start", 457, 497, 20, Color{255, 216, 91, 255});
}

void Game::DrawLevelComplete() const {
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{2, 8, 12, 218});
    DrawRectangleRounded({390.0f, 225.0f, 500.0f, 255.0f}, 0.06f, 8, Color{10, 28, 31, 245});
    DrawRectangleRoundedLinesEx({390.0f, 225.0f, 500.0f, 255.0f}, 0.06f, 8, 4.0f, Color{96, 255, 145, 255});

    const char* title = TextFormat("ROOM %d CLEARED", currentLevel_);
    const int titleWidth = MeasureText(title, 40);
    DrawText(title, ScreenWidth / 2 - titleWidth / 2, 268, 40, Color{96, 255, 145, 255});

    const char* nextRoom = "";
    if (currentLevel_ == 1) {
        nextRoom = "The next room adds a fifth pillar.";
    } else if (currentLevel_ == 2) {
        nextRoom = "The next room adds a sixth pillar and a triangle hunter.";
    } else {
        nextRoom = "Final room: seven pillars, two hunters, and a music-note gun.";
    }
    const int nextWidth = MeasureText(nextRoom, 20);
    DrawText(nextRoom, ScreenWidth / 2 - nextWidth / 2, 340, 20, RAYWHITE);
    DrawText("Press ENTER / SPACE / click to continue", 458, 412, 18, Color{255, 216, 91, 255});
}

void Game::DrawWin() const {
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{2, 8, 12, 218});
    DrawRectangleRounded({390.0f, 240.0f, 500.0f, 220.0f}, 0.06f, 8, Color{10, 28, 31, 245});
    DrawRectangleRoundedLinesEx({390.0f, 240.0f, 500.0f, 220.0f}, 0.06f, 8, 4.0f, Color{96, 255, 145, 255});
    DrawText("YOU ESCAPED", 467, 279, 46, Color{96, 255, 145, 255});
    DrawText("All four echo rooms are open.", 489, 345, 20, RAYWHITE);
    DrawText("Press R or ENTER to play again", 480, 403, 18, Color{190, 196, 215, 255});
}

Sound Game::MakeTone(float frequency, float seconds, float volume) {
    constexpr unsigned int sampleRate = 44100;
    const unsigned int sampleCount = static_cast<unsigned int>(seconds * static_cast<float>(sampleRate));
    auto* samples = static_cast<std::int16_t*>(MemAlloc(sampleCount * sizeof(std::int16_t)));

    for (unsigned int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        const float fadeIn = std::min(1.0f, t / 0.015f);
        const float fadeOut = std::min(1.0f, (seconds - t) / 0.06f);
        const float envelope = std::max(0.0f, std::min(fadeIn, fadeOut));
        const float fundamental = std::sin(2.0f * Pi * frequency * t);
        const float harmonic = 0.28f * std::sin(2.0f * Pi * frequency * 2.01f * t);
        const float value = (fundamental + harmonic) * volume * envelope;
        samples[i] = static_cast<std::int16_t>(std::clamp(value, -1.0f, 1.0f) * 32767.0f);
    }

    Wave wave{};
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;

    Sound sound = LoadSoundFromWave(wave);
    MemFree(samples);
    return sound;
}

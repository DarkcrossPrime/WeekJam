#pragma once

#include <array>
#include <cstddef>

#include "raylib.h"

class Game {
public:
    void Init();
    void Shutdown();
    void Update(float dt);
    void Draw() const;

private:
    enum class Scene {
        Intro,
        Playing,
        LevelComplete,
        Won
    };

    enum class MessageKind {
        None,
        DoorLocked,
        Correct,
        Wrong,
        Unlocked,
        Hurt,
        Respawned,
        EnemyHit,
        EnemyDestroyed
    };

    struct Pillar {
        Rectangle body{};
        float frequency = 440.0f;
        Color color = WHITE;
        char label = '1';
        float pulse = 0.0f;
    };

    struct Enemy {
        Vector2 position{};
        float radius = 20.0f;
        float speed = 105.0f;
        float spawnTimer = 0.0f;
        float hitFlash = 0.0f;
        int health = 3;
        bool active = false;
    };

    struct Projectile {
        Vector2 position{};
        Vector2 velocity{};
        float life = 0.0f;
        float radius = 6.0f;
        bool active = false;
    };

    static constexpr int ScreenWidth = 1280;
    static constexpr int ScreenHeight = 720;
    static constexpr float PlayerRadius = 14.0f;
    static constexpr std::size_t MaxPillars = 7;
    static constexpr std::size_t MaxEnemies = 2;
    static constexpr std::size_t MaxProjectiles = 24;
    static constexpr int LevelCount = 4;
    static constexpr int MaxHealth = 10;

    Scene scene_ = Scene::Intro;
    MessageKind messageKind_ = MessageKind::None;
    Vector2 player_{640.0f, 620.0f};
    Vector2 aimDirection_{0.0f, -1.0f};
    std::array<Pillar, MaxPillars> pillars_{};
    std::array<Enemy, MaxEnemies> enemies_{};
    std::array<Projectile, MaxProjectiles> projectiles_{};
    std::array<int, MaxPillars> sequence_{2, 0, 3, 1, 4, 5, 6};
    std::array<Sound, MaxPillars> tones_{};
    Sound successTone_{};
    Sound failTone_{};
    Sound shotTone_{};

    int currentLevel_ = 1;
    int pillarCount_ = 4;
    int sequenceLength_ = 4;
    int enemyCount_ = 0;
    int health_ = MaxHealth;

    bool audioReady_ = false;
    bool doorUnlocked_ = false;
    bool playbackActive_ = false;
    int playbackIndex_ = 0;
    int playerProgress_ = 0;
    float playbackStepTimer_ = 0.0f;
    float replayTimer_ = 1.0f;
    float messageTimer_ = 0.0f;
    float wrongFlash_ = 0.0f;
    float damageFlash_ = 0.0f;
    float damageCooldown_ = 0.0f;
    float fireCooldown_ = 0.0f;
    float totalTime_ = 0.0f;

    Rectangle room_{40.0f, 70.0f, 1200.0f, 600.0f};
    Rectangle glassDoor_{580.0f, 70.0f, 120.0f, 32.0f};

    void ConfigureLevel();
    void ResetRound();
    void StartPlayback();
    void UpdatePlayback(float dt);
    void UpdatePlayer(float dt);
    void UpdateEnemies(float dt);
    void UpdateProjectiles(float dt);
    void ResolveEnemyContact(Enemy& enemy);
    void ShootNote();
    void TryInteract();
    void ActivatePillar(std::size_t index, bool playerActivated);
    bool CanPlayerOccupy(Vector2 position) const;
    bool CanEnemyOccupy(Vector2 position, float radius) const;
    bool ProjectileHitsObstacle(Vector2 position, float radius) const;
    int NearbyPillar() const;
    bool NearDoor() const;

    void DrawRoom() const;
    void DrawDoor() const;
    void DrawPillars() const;
    void DrawEnemies() const;
    void DrawProjectiles() const;
    void DrawPlayer() const;
    void DrawHud() const;
    void DrawHealthBar() const;
    void DrawIntro() const;
    void DrawLevelComplete() const;
    void DrawWin() const;

    static Sound MakeTone(float frequency, float seconds, float volume = 0.35f);
};

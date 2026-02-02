#pragma once
// --- CONSTANTS ---
const int MAX_BOSS_BULLETS = 20;

// --- STRUCTS ---
struct BossBullet {
    float x, y;
    float dirX, dirY;
    bool active;
};

struct Boss {
    // -- Physics (Existing) --
    float x, y;
    float w, h;
    int health;
    bool active;

    // -- AI & Combat (NEW) --
    float baseY;       // The Y level the boss hovers around
    float moveTimer;   // Controls the sine wave bobbing
    float shootTimer;  // Cooldown between shots

    // -- Ammo (NEW) --
    BossBullet bullets[MAX_BOSS_BULLETS];
};

// --- FUNCTIONS ---
void Boss_Initialize(Boss& boss);
void Boss_Update(Boss& boss, float dt, float playerX, float playerY);
void Boss_Draw(Boss& boss, AEGfxVertexList* mesh);
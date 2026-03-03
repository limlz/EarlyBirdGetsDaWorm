#pragma once
// --- CONSTANTS ---
const int MAX_BOSS_BULLETS = 20;

// --- STRUCTS ---
struct BossBullet {
    float x, y;
    float dirX, dirY;
    bool active;

    int hp; // <--- ADD THIS LINE!
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


    // Add these under your existing variables in Boss.hpp!
    int currentState;     // 0 = Normal, 1 = Spread, 2 = Shield
    float stateTimer;     // How long to stay in the current state
    bool shieldActive;    // Is the defense shield currently on?
};


// --- FUNCTIONS ---
bool Boss_CheckCollision(Boss& boss, float objX, float objY, float objW, float objH);
void Boss_DrawHealthBar(Boss& boss, AEGfxVertexList* mesh);
void Boss_Load();
void Boss_Unload();
void Boss_Initialize(Boss& boss);
void Boss_Update(Boss& boss, float dt, float playerX, float playerY);
void Boss_Draw(Boss& boss, AEGfxVertexList* mesh);
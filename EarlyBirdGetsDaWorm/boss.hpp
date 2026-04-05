#pragma once
// --- CONSTANTS ---
const int MAX_BOSS_BULLETS = 20;
const int BOSS_MAX_HEALTH  = 500;

// --- STRUCTS ---
struct BossBullet {
    float x, y;
    float dirX, dirY;
    bool active;
    bool is_scatter; // true = spread shot, uses scatter bullet texture
    int hp;
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

    // -- Horror Phase System --
    int phase;                  // 0=Paranoia, 1=Schizophrenia, 2=Insomnia, 3=Scotophobia
    float phaseTransitionTimer; // brief flash/shake on phase change

    // -- Flicker (all phases, more erratic at higher phases) --
    float flickerTimer;
    bool flickerVisible;

    // -- Horror Teleport --
    bool isVanished;         // true while mid-teleport vanish
    float vanishTimer;       // countdown during vanish
    float vanishReappearY;   // target Y position after vanish
};


// --- FUNCTIONS ---

/******************************************************************************************
    @brief  Tests AABB collision between the boss and an arbitrary rectangle.

    @param  boss    The boss entity to test against.
    @param  objX    X center of the object.
    @param  objY    Y center of the object.
    @param  objW    Width of the object.
    @param  objH    Height of the object.
    @return true if the rectangles overlap.
******************************************************************************************/
bool Boss_CheckCollision(Boss& boss, float objX, float objY, float objW, float objH);

/******************************************************************************************
    @brief  Renders the boss health bar UI element at a fixed screen position.

    @param  boss    The boss whose health to display.
    @param  mesh    The square mesh used for drawing.
******************************************************************************************/
void Boss_DrawHealthBar(Boss& boss, AEGfxVertexList* mesh);

/******************************************************************************************
    @brief  Draws the interact/dodge prompt near the boss when the player is in range.

    @param  boss    The boss entity.
    @param  mesh    The square mesh used for drawing.
******************************************************************************************/
void Boss_DrawInteractPrompt(Boss& boss, AEGfxVertexList* mesh);

/******************************************************************************************
    @brief  Loads all boss-related textures and assets into memory.
******************************************************************************************/
void Boss_Load();

/******************************************************************************************
    @brief  Unloads all boss-related textures and frees GPU resources.
******************************************************************************************/
void Boss_Unload();

/******************************************************************************************
    @brief  Resets the boss to its default state with full health, initial position,
            and phase 0.

    @param  boss    The boss entity to initialize.
******************************************************************************************/
void Boss_Initialize(Boss& boss);

/******************************************************************************************
    @brief  Runs the boss AI logic: hovering, state machine transitions, bullet spawning,
            horror phase progression, and flicker timing.

    @param  boss    The boss entity to update.
    @param  dt      Frame delta time in seconds.
    @param  playerX The player's current X position (for aiming).
    @param  playerY The player's current Y position (for aiming).
******************************************************************************************/
void Boss_Update(Boss& boss, float dt, float playerX, float playerY);

/******************************************************************************************
    @brief  Renders the boss sprite, shield, and active bullets.

    @param  boss    The boss entity to draw.
    @param  mesh    The square mesh used for drawing.
******************************************************************************************/
void Boss_Draw(Boss& boss, AEGfxVertexList* mesh);

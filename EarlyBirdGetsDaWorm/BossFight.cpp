#include "pch.hpp"
#include "BossDebug.hpp"
#include "Animation.hpp"

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;

// Textures for level background and player character
static AEGfxTexture* wallBgTexture = nullptr;
static AEGfxTexture* player_walk1 = nullptr;
static AEGfxTexture* player_walk2 = nullptr;
static AEGfxTexture* player_crouch = nullptr;
static AEGfxTexture* player_arm_gun = nullptr;
static AEGfxTexture* gun_tex        = nullptr;

// --- PLAYER PHYSICS SETTINGS ---
static f32 playerX, playerY;
float playerVelY = 0.0f;
float playerVelX = 0.0f;
bool isGrounded = false;
bool isCrouching = false; // Flag to track whether the player is currently crouching
const float GRAVITY = 1500.0f;
const float JUMP_FORCE = 850.0f;
const float GROUND_Y = -230.0f; // Adjusted so the bottom of the 360-tall player sits exactly on the -350 line (top of the black bar)
int jumpCount = 0;
const int MAX_JUMPS = 2;
bool facingRight = true;

// --- PLAYER HEALTH ---
const int MAX_PLAYER_HP = 20;
int playerHP = MAX_PLAYER_HP;
bool isPlayerHit = false;
float hitTimer = 0.0f;
static float vignetteAlpha = 0.0f; // Set to 1.0f to flash the damage vignette overlay
static float lagHpPercent  = 1.0f; // Trails behind hpPercent for the ghost-bar effect

// --- PLAYER ANIMATION ---
static Animation playerWalkAnim = { 0, 0.0f, 0.15f, 2, true }; // loop 2 frames

// --- WEAPON: HOLY WATER GUN ---
static float fireCooldown = 0.0f;
const float FIRE_RATE = 0.02f; // Fires a particle every 0.02 seconds for a continuous stream
static float bossDodgeCooldown = 0.0f;

// --- ARM / GUN ---
static float armAngle = 0.0f; // angle from shoulder to mouse (radians)
const float ARM_LENGTH = 45.0f;
const float ARM_WIDTH  = 24.0f;
const float GUN_LENGTH  = 32.0f;
const float GUN_WIDTH   = 10.0f;
const float GUN1_LENGTH = 85.0f;  // draw size of the separate gun1.png sprite
const float GUN1_WIDTH  = 32.0f;

// --- BOSS INSTANCE ---
static Boss myBoss;

// --- BOSS FIGHT FADE-IN SETTINGS ---
static bool isFadingIn = true;
static float fadeInAlpha = 1.0f; // Start completely black with an alpha of 1.0
const float FADE_IN_SPEED = 1.0f; // Takes 1 second to fade in

// --- VICTORY SEQUENCE VARIABLES ---
static bool isVictorySequence = false;
static float victoryAlpha = 0.0f;
static float victoryHoldTimer = 0.0f;
static s8 victoryFontId    = -1;
static s8 victorySubFontId = -1;

// --- HORROR ATMOSPHERE ---
static float horrorTime        = 0.0f;
static float bossShakeX        = 0.0f;
static float bossShakeY        = 0.0f;
static float bossShakeTimer    = 0.0f;
static float whisperTimer      = 0.0f;
static float heartbeatTimer    = 0.0f;
static float heartbeatInterval = 1.5f;
static float whisperTextTimer  = 0.0f;
static float whisperTextAlpha  = 0.0f;
static float whisperTextX      = 0.0f;
static float whisperTextY      = 0.0f;
static float whisperTextScale  = 0.7f;
static int   whisperTextIndex  = 0;
static float shieldDarkenAlpha = 0.0f;
static float bgmVolume         = 0.25f;
static float victoryDarkTimer  = 0.0f;
static bool  victoryHeartbeatPlayed = false;
static bool  victoryReleasePlayed   = false;

static const char* WHISPER_TEXTS[] = {
    "HELP ME", "DON'T LOOK", "IT'S BEHIND YOU",
    "YOU CAN'T SAVE THEM", "WRONG PATIENT", "LET ME OUT",
    "CAN YOU HEAR IT", "NOT REAL"
};
static const int NUM_WHISPER_TEXTS = 8;

void Player_DrawHealthBar(AEGfxVertexList* mesh)
{
    float hpPercent = static_cast<float>(playerHP) / static_cast<float>(MAX_PLAYER_HP);
    if (hpPercent < 0.0f) hpPercent = 0.0f;

    // BOTTOM LEFT — horizontal bar sitting on the bottom black bar
    const float barCenterX = -520.0f;
    const float barCenterY = -390.0f;
    const float barW       = 280.0f;
    const float barH       =  18.0f;
    const float pad        =   3.0f;
    const float barLeft    = barCenterX - barW * 0.5f;

    // Shake when hit: sine/cosine driven by hitTimer so it oscillates fast then stops
    float shakeX = isPlayerHit ? sinf(hitTimer * 90.0f) * 5.0f : 0.0f;
    float shakeY = isPlayerHit ? cosf(hitTimer * 80.0f) * 3.0f : 0.0f;
    float cx = barCenterX + shakeX;
    float cy = barCenterY + shakeY;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Outer dark border (shakes with the bar)
    DrawSquareMesh(mesh, cx, cy, barW + pad * 2.0f, barH + pad * 2.0f, 0x111111FF);
    // Dark background trough
    DrawSquareMesh(mesh, cx, cy, barW, barH, 0x1A0000FF);

    // Ghost (lag) bar — white, fades slowly after taking damage, drawn BEHIND actual fill
    if (lagHpPercent > hpPercent) {
        float lagFillW = barW * lagHpPercent;
        float lagFillX = (barLeft + lagFillW * 0.5f) + shakeX;
        AEGfxSetTransparency(0.55f);
        DrawSquareMesh(mesh, lagFillX, cy, lagFillW, barH, 0xFFFFFFFF);
        AEGfxSetTransparency(1.0f);
    }

    // Actual HP fill — bright red, fills left-to-right
    if (playerHP > 0) {
        float fillW = barW * hpPercent;
        float fillX = (barLeft + fillW * 0.5f) + shakeX;
        DrawSquareMesh(mesh, fillX, cy, fillW, barH, 0x740404FF);

        // Thin gloss stripe along the top edge
        float hiH = barH * 0.22f;
        float hiY = cy + (barH - hiH) * 0.5f;
        AEGfxSetTransparency(0.25f);
        DrawSquareMesh(mesh, fillX, hiY, fillW, hiH, 0xFF6666FF);
        AEGfxSetTransparency(1.0f);
    }

    // "HP" label to the left of the bar
    if (victoryFontId >= 0) {
        float ndcX = (barLeft - 52.0f) / 800.0f;
        float ndcY = (barCenterY - 7.0f) / 450.0f;
        AEGfxPrint(victoryFontId, "HP", ndcX, ndcY, 0.50f,
                   0.80f, 0.20f, 0.20f, 1.0f);
    }
}

// ---------------------------------------------------------
// GAME STATE FUNCTIONS
// ---------------------------------------------------------

void Boss_Fight_Load()
{
    // Load the font for the victory text (size 35)
    victoryFontId    = AEGfxCreateFont(Assets::Fonts::Buggy, 35);
    victorySubFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    wallBgTexture = LoadTextureChecked(Assets::Background::WallBg);

    // Load player textures
    player_walk1   = LoadTextureChecked(Assets::Boss_Fight::PlayerShooting1);
    player_walk2   = LoadTextureChecked(Assets::Boss_Fight::PlayerShooting2);
    player_crouch  = LoadTextureChecked(Assets::Boss_Fight::PlayerCrouch); // Placeholder for crouch texture
    player_arm_gun = LoadTextureChecked(Assets::Boss_Fight::PlayerArmGun);
    gun_tex        = LoadTextureChecked(Assets::Boss_Fight::Gun1);

    Boss_Load();
    BossDebug_Load();
    // Note: PauseMenu textures are owned by Game_Load() and survive the boss fight transition.
    // Calling PauseMenu_Load/Unload here would free them and leave Game state with null texture pointers.
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
}

void Boss_Fight_Initialize()
{
    Bullets_Initialize();
    PauseMenu_Initialize();
    Boss_Initialize(myBoss);
    myBoss.health = BOSS_MAX_HEALTH; // explicit reset every entry

    playerX = -600.0f;
    playerY = GROUND_Y;
    playerVelY = 0.0f;
    playerVelX = 0.0f;
    isGrounded = true;
    isCrouching = false;
    jumpCount = 0;
    facingRight = true;
    playerHP = MAX_PLAYER_HP;
    isPlayerHit = false;
    lagHpPercent = 1.0f;

    // Reset the victory sequence variables
    isVictorySequence = false;
    victoryAlpha = 0.0f;
    victoryHoldTimer = 0.0f;

    isFadingIn = true;
    fadeInAlpha = 1.0f;

    fireCooldown = 0.0f;
    bossDodgeCooldown = 0.0f;
    armAngle = 0.0f;

    // Start boss fight music
    AudioManager_StopBGM();
    AudioManager_PlayBGM(SoundID::SFX_BOSS_FIGHT, 0.25f);

    // Reset horror atmosphere
    horrorTime = 0.0f;
    bossShakeX = bossShakeY = bossShakeTimer = 0.0f;
    whisperTimer = 5.0f + static_cast<float>(RandInt(0, 50)) / 10.0f;
    heartbeatTimer = 1.5f;
    heartbeatInterval = 1.5f;
    whisperTextTimer = 3.0f + static_cast<float>(RandInt(0, 30)) / 10.0f;
    whisperTextAlpha = 0.0f;
    whisperTextIndex = 0;
    shieldDarkenAlpha = 0.0f;
    bgmVolume = 0.25f;
    victoryDarkTimer = 0.0f;
    victoryHeartbeatPlayed = false;
    victoryReleasePlayed = false;

    // Auto-open the guide to the Boss Fight instructions page
    Guide_BossFight();
}

void Boss_Fight_Update()
{
    float dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());

    BossDebug_Update(myBoss);
    PauseMenu_Update(dt);
    if (PauseMenu_IsPaused()) return;

    Guide_Update(false, dt, false);
    if (IsGuideActive()) return;

    if (myBoss.health <= 0 && !isVictorySequence) {
        isVictorySequence = true;
        victoryDarkTimer = 0.0f;
        victoryHeartbeatPlayed = false;
        victoryReleasePlayed = false;
        AudioManager_StopBGM();

        if (!Settings_IsHorrorLessEnabled()) {
            bossShakeTimer = 0.6f;
            AudioManager_PlaySFX(SoundID::SFX_SCREAMING, 0.07f, 0.4f);
        } else {
            AudioManager_PlaySFX(SoundID::SFX_SCREAMING, 0.05f);
        }
    }

    // Run the victory animation and block all gameplay input
    if (isVictorySequence) {
        bool isHorrorVictory = !Settings_IsHorrorLessEnabled();
        victoryDarkTimer += dt;

        if (isHorrorVictory) {
            if (victoryDarkTimer < 1.5f) {
                victoryAlpha = victoryDarkTimer / 1.5f;
                if (victoryDarkTimer > 0.8f && !victoryHeartbeatPlayed) {
                    AudioManager_PlaySFX(SoundID::SFX_HEARTBEAT, 0.3f, 0.6f);
                    victoryHeartbeatPlayed = true;
                }
            } else if (victoryDarkTimer < 2.0f) {
                victoryAlpha = 1.0f;
            } else {
                victoryAlpha = 1.0f;
                if (!victoryReleasePlayed) {
                    AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.15f, 0.7f);
                    AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.1f, 1.2f);
                    victoryReleasePlayed = true;
                }
                victoryHoldTimer += dt;
                if (victoryHoldTimer > 3.0f) {
                    if (Game_IsPresentationMode()) {
                        next = PRESENTATION_ENDGAME_STATE;
                    } else {
                        Player_GenerateMission();
                        Timer_SetPaused(false);
                        next = GAME_STATE;
                    }
                }
            }
        } else {
            victoryAlpha += 1.0f * dt;
            if (victoryAlpha >= 1.0f) {
                victoryAlpha = 1.0f;
                victoryHoldTimer += dt;
                if (victoryHoldTimer > 3.0f) {
                    if (Game_IsPresentationMode()) {
                        next = PRESENTATION_ENDGAME_STATE;
                    } else {
                        Player_GenerateMission();
                        Timer_SetPaused(false);
                        next = GAME_STATE;
                    }
                }
            }
        }
        return;
    }

    if (isFadingIn) {
        fadeInAlpha -= FADE_IN_SPEED * dt;
        if (fadeInAlpha <= 0.0f) {
            fadeInAlpha = 0.0f;
            isFadingIn = false; // Fade-in complete
        }
    }

    // --- DETERMINE CROUCH STATE ---
    isCrouching = AEInputCheckCurr(AEVK_LCTRL);

    // --- ARM: TRACK MOUSE ---
    {
        s32 cursorX, cursorY;
        AEInputGetCursorPosition(&cursorX, &cursorY);
        float worldMouseX = static_cast<float>(cursorX) - (AEGfxGetWindowWidth()  / 2.0f);
        float worldMouseY = (AEGfxGetWindowHeight() / 2.0f) - static_cast<float>(cursorY);

        // Flip to face whichever side the gun is pointing
        facingRight = (worldMouseX >= playerX);

        float drawY     = isCrouching ? playerY - 30.0f : playerY;
        float shoulderX = playerX - 7.0f;
        float shoulderY = drawY + 20.0f;
        armAngle = atan2f(worldMouseY - shoulderY, worldMouseX - shoulderX);
    }


    // Only allow movement, animation, and shooting if NOT crouching
    if (!isCrouching) {

        // --- PLAYER ANIMATION LOGIC ---
        if (AEInputCheckCurr(AEVK_A) || AEInputCheckCurr(AEVK_D)) {
            playerWalkAnim.Update(dt);
        }
        else {
            playerWalkAnim.Reset();
        }

        if (AEInputCheckTriggered(AEVK_SPACE)) {
            if (isGrounded || jumpCount < MAX_JUMPS) {
                playerVelY = JUMP_FORCE;
                isGrounded = false;
                jumpCount++;
                // Horror-less mode uses a silly WEOW instead of a grunt
                if (Settings_IsHorrorLessEnabled())
                    AudioManager_PlaySFX(SoundID::SFX_WEOW, 0.3f);
                else
                    AudioManager_PlaySFX(SoundID::SFX_JUMP_GRUNT, 0.25f);
            }
        }

        // --- HOLY WATER GUN LOGIC ---
        // Decrease timers
        fireCooldown -= dt;
        bossDodgeCooldown -= dt;

        // Check if the button is HELD DOWN for rapid fire (AEInputCheckCurr)
        if (AEInputCheckCurr(AEVK_LBUTTON)) {

            // Only fire if the cooldown timer has finished
            if (fireCooldown <= 0.0f) {
                fireCooldown = FIRE_RATE; // Reset timer
                if (Bullets_HasAmmo())
                    AudioManager_PlaySFX(SoundID::SFX_WATER_GUN_SQUIRT, 0.12f, 0.7f);

                // Bullet spawns from the gun tip
                float drawY     = isCrouching ? playerY - 30.0f : playerY;
                float shoulderX = playerX - 7.0f;
                float shoulderY = drawY + 20.0f;
                const float GRIP_OFFSET = 10.0f; // must match the draw offset in Boss_Fight_Draw
                float startX = shoulderX + cosf(armAngle) * (ARM_LENGTH + GUN1_LENGTH) - sinf(armAngle) * GRIP_OFFSET;
                float startY = shoulderY + sinf(armAngle) * (ARM_LENGTH + GUN1_LENGTH) + cosf(armAngle) * GRIP_OFFSET;

                float dirX = cosf(armAngle);
                float dirY = sinf(armAngle);

                // Add a slight randomized spread to simulate spraying liquid
                float spreadX = ((RandInt(0, 99) - 50) / 1000.0f); // Random spread value between -0.05 and 0.05
                float spreadY = ((RandInt(0, 99) - 50) / 1000.0f);
                dirX += spreadX;
                dirY += spreadY;

                // Re-normalize direction vector after adding spread
                float length = sqrtf((dirX * dirX) + (dirY * dirY));
                if (length > 0.0001f) { dirX /= length; dirY /= length; }

                Fire_Bullet(startX, startY, dirX, dirY);

                // Dodge check moved to Bullets.cpp hit detection (Boss_Fight_TryDodge)
            }
        }
    }
    else {
        // Force idle frame and lock timers when crouching
        playerWalkAnim.Reset();
        fireCooldown -= dt; // Keep decrementing cooldowns so shooting is possible immediately after standing
        bossDodgeCooldown -= dt;
    }

    // --- MOMENTUM & MOVEMENT LOGIC ---
    float targetVelX = 0.0f;

    // Only accept left/right input if NOT crouching
    if (!isCrouching) {
        if (AEInputCheckCurr(AEVK_D)) { targetVelX = PLAYER_SPEED; }
        if (AEInputCheckCurr(AEVK_A)) { targetVelX = -PLAYER_SPEED; }
    }

    if (isGrounded) {
        // On the ground: Tight controls. No sliding physics
        if (isCrouching) {
            playerVelX = 0.0f; // Instant stop when crouching on the ground
        }
        else {
            playerVelX = targetVelX; // Instant start/stop when running
        }
    }
    else {
        // In the air: Smoothly retain momentum (crouching mid-air keeps forward velocity)
        playerVelX += (targetVelX - playerVelX) * 2.0f * dt;
    }

    // Apply velocity to move the player
    playerX += playerVelX * dt;

    // Adjusted X clamp to account for the new 140 width (half width is 70)
    if (playerX < -753.33f) { playerX = -753.33f; playerVelX = 0.0f; } // Kill horizontal velocity against walls
    if (playerX > 753.33f) { playerX = 753.33f; playerVelX = 0.0f; }

    // --- GRAVITY & CEILING LIMITS ---
    playerVelY -= GRAVITY * dt;
    playerY += playerVelY * dt;

    // Adjusted ceiling clamp for the new 360 height (half height is 180). Top bar bottom is at 350.
    if (playerY > 230.0f) {
        playerY = 230.0f;
        if (playerVelY > 0.0f) playerVelY = 0.0f;
    }

    if (playerY <= GROUND_Y) {
        playerY = GROUND_Y; playerVelY = 0.0f; isGrounded = true; jumpCount = 0;
    }
    else {
        isGrounded = false;
    }

    // Fast fall logic acts independently, allowing plummeting mid-air and crouching upon landing
    if (AEInputCheckCurr(AEVK_LCTRL)) { playerVelY -= 1000.0f * dt; }

    // ----------------------------------------------------
    // --- UPDATE BOSS ---
    // ----------------------------------------------------
    int preUpdateState = myBoss.currentState;
    bool preUpdateShield = myBoss.shieldActive;

    Boss_Update(myBoss, dt, playerX, playerY);

    // Trigger screen shake on spread attack or shield activation
    if (!Settings_IsHorrorLessEnabled()) {
        if (myBoss.currentState == 1 && preUpdateState != 1)
            bossShakeTimer = 0.25f;
        if (myBoss.shieldActive && !preUpdateShield)
            bossShakeTimer = 0.3f;
        if (myBoss.phaseTransitionTimer > 0.3f && bossShakeTimer <= 0.0f)
            bossShakeTimer = 0.4f;
    }

    if (myBoss.active) {
        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (myBoss.bullets[i].active) {
                // Adjust player physical hitbox dynamically based on crouch state
                float currentHitboxY = isCrouching ? playerY - 15.0f : playerY;
                float currentHitboxH = isCrouching ? 105.0f : 120.0f;

                // Expanded hitbox to fully match the new 140x360 dimensions
                if (abs(myBoss.bullets[i].x - playerX) < 46.66f &&
                    abs(myBoss.bullets[i].y - currentHitboxY) < currentHitboxH)
                {
                    if (!BossDebug_IsGodMode()) {
                        playerHP--;
                        if (Game_IsPlaytestMode() && playerHP < MAX_PLAYER_HP / 4)
                            playerHP = MAX_PLAYER_HP / 4;
                    }
                    myBoss.bullets[i].active = false;
                    isPlayerHit = true;
                    hitTimer = 0.2f;  
                    vignetteAlpha = 1.0f; // Hook: set to 1.0f to trigger vignette / cracked-glass overlay
                    AudioManager_PlaySFX(SoundID::SFX_EXHALE, 0.35f);  // Player hurt reaction

                    if (playerHP <= 0 && !BossDebug_IsGodMode()) {
                        Player_StopSounds(); // prevent walking SFX persisting into endgame
                        AudioManager_StopBGM(); // stop boss fight music before endgame
                        currentEndReason = EndGameReason::REASON_DIED_TO_BOSS;
                        Analytics::LogRun(currentEndReason, Game_GetCurrentDay(), Player_GetPatientsHandled(), IsGhostPatient(),
                            static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), Game_GetTimeSurvived());
                        next = ENDGAME_STATE;
                    }
                }
            }
        }
    }

    if (isPlayerHit) {
        hitTimer -= dt;
        if (hitTimer <= 0.0f) isPlayerHit = false;
    }

    // Decay vignette overlay (~0.4 s fade-out)
    if (vignetteAlpha > 0.0f) {
        vignetteAlpha -= dt * 2.5f;
        if (vignetteAlpha < 0.0f) vignetteAlpha = 0.0f;
    }

    // Playtest mode: hard floor at 25% HP — enforced every frame so no damage path can bypass it
    if (Game_IsPlaytestMode() && !isVictorySequence && playerHP < MAX_PLAYER_HP / 4)
        playerHP = MAX_PLAYER_HP / 4;

    // Lag (ghost) bar slowly catches up to actual HP
    {
        float currentHpPct = static_cast<float>(playerHP) / static_cast<float>(MAX_PLAYER_HP);
        if (lagHpPercent > currentHpPct) {
            lagHpPercent -= dt * 0.45f;
            if (lagHpPercent < currentHpPct) lagHpPercent = currentHpPct;
        }
        else {
            lagHpPercent = currentHpPct; // snap up if HP is restored
        }
    }

    // ====================================================
    // HORROR ATMOSPHERE UPDATE
    // ====================================================
    horrorTime += dt;
    float hpPct = static_cast<float>(myBoss.health) / static_cast<float>(BOSS_MAX_HEALTH);
    bool isHorror = !Settings_IsHorrorLessEnabled();

    // Screen shake decay
    if (bossShakeTimer > 0.0f) {
        bossShakeTimer -= dt;
        float intensity = bossShakeTimer * 40.0f;
        bossShakeX = sinf(horrorTime * 90.0f) * intensity;
        bossShakeY = cosf(horrorTime * 80.0f) * intensity * 0.7f;
    } else {
        bossShakeX = bossShakeY = 0.0f;
    }

    // Shield darken effect
    if (isHorror) {
        float shieldTarget = myBoss.shieldActive ? 0.6f : 0.0f;
        shieldDarkenAlpha += (shieldTarget - shieldDarkenAlpha) * 4.0f * dt;
    }

    // Dynamic BGM volume — fades as boss weakens
    {
        float targetVol = 0.08f + 0.17f * hpPct;
        bgmVolume += (targetVol - bgmVolume) * 0.5f * dt;
        AudioManager_SetBGMVolume(bgmVolume);
    }

    // Sound anomalies per phase (horror only)
    if (isHorror) {
        whisperTimer -= dt;
        if (whisperTimer <= 0.0f) {
            switch (myBoss.phase) {
            case 0:
                AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.15f, 0.85f + static_cast<float>(RandInt(0, 15)) / 100.0f);
                whisperTimer = 5.0f + static_cast<float>(RandInt(0, 50)) / 10.0f;
                break;
            case 1:
                AudioManager_PlaySFX(SoundID::SFX_SCREAMING, 0.12f, 0.6f + static_cast<float>(RandInt(0, 80)) / 100.0f);
                whisperTimer = 3.0f + static_cast<float>(RandInt(0, 40)) / 10.0f;
                break;
            case 2:
                whisperTimer = 99.0f;
                break;
            case 3:
                AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.2f, 1.0f, -1);
                whisperTimer = 99.0f;
                break;
            }
        }

        // Phase 2+ heartbeat
        if (myBoss.phase >= 2) {
            heartbeatTimer -= dt;
            if (heartbeatTimer <= 0.0f) {
                AudioManager_PlaySFX(SoundID::SFX_HEARTBEAT, 0.25f);
                heartbeatInterval = heartbeatInterval * 0.92f;
                if (heartbeatInterval < 0.4f) heartbeatInterval = 0.4f;
                heartbeatTimer = heartbeatInterval;
            }
        }
    }

    // Whisper text overlay (horror only)
    if (isHorror) {
        whisperTextTimer -= dt;
        if (whisperTextTimer <= 0.0f && whisperTextAlpha <= 0.0f) {
            whisperTextIndex = RandInt(0, NUM_WHISPER_TEXTS - 1);
            whisperTextAlpha = 0.6f + static_cast<float>(RandInt(0, 30)) / 100.0f;
            whisperTextX = -0.6f + static_cast<float>(RandInt(0, 120)) / 100.0f;
            whisperTextY = -0.4f + static_cast<float>(RandInt(0, 80)) / 100.0f;
            whisperTextScale = 0.5f + static_cast<float>(RandInt(0, 50)) / 100.0f;
            float interval = (myBoss.phase >= 3) ? 2.5f : (myBoss.phase >= 1) ? 5.0f : 7.0f;
            float variance = (myBoss.phase >= 3) ? 1.5f : 3.0f;
            whisperTextTimer = interval + static_cast<float>(RandInt(0, static_cast<int>(variance * 10))) / 10.0f;
        }
        if (whisperTextAlpha > 0.0f) {
            whisperTextAlpha -= dt * 1.5f;
            if (whisperTextAlpha < 0.0f) whisperTextAlpha = 0.0f;
        }
    }

    Bullets_Update(dt, 0.0f, myBoss);
}

void Boss_Fight_Draw()
{
    bool isHorror = !Settings_IsHorrorLessEnabled();
    float shakeOX = bossShakeX;
    float shakeOY = bossShakeY;

    // Background with subtle UV drift in later phases
    {
        float uvX = 0.0f, uvY = 0.0f;
        if (isHorror && myBoss.phase >= 2) {
            uvX = sinf(horrorTime * 0.3f) * 0.015f;
            uvY = cosf(horrorTime * 0.2f) * 0.01f;
        }
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(wallBgTexture, uvX, uvY);
        AEMtx33 bgScale, bgTrans, bgXform;
        AEMtx33Scale(&bgScale, 1600.0f, 900.0f);
        AEMtx33Trans(&bgTrans, shakeOX, shakeOY);
        AEMtx33Concat(&bgXform, &bgTrans, &bgScale);
        AEGfxSetTransform(bgXform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Darkness overlay — base 0.2 always, grows with HP loss
    if (isHorror) {
        float hpPct2 = static_cast<float>(myBoss.health) / static_cast<float>(BOSS_MAX_HEALTH);
        float baseAlpha = 0.20f + (1.0f - hpPct2) * 0.45f;
        float pulseSpeed = 1.0f + myBoss.phase * 2.5f;
        float pulse = 0.05f * sinf(horrorTime * pulseSpeed);
        float overlayAlpha = baseAlpha + pulse + shieldDarkenAlpha;
        if (overlayAlpha > 0.88f) overlayAlpha = 0.88f;
        DrawBlackOverlay(squareMesh, overlayAlpha);
    }

    // Lighting tint per phase (all phases including 0)
    if (isHorror) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        float tintR = 0.0f, tintG = 0.0f, tintB = 0.0f, tintA = 0.0f;
        switch (myBoss.phase) {
        case 0: // Paranoia: sickly desaturated yellow
            tintR = 0.12f; tintG = 0.08f; tintB = 0.0f;
            tintA = 0.10f + 0.03f * sinf(horrorTime * 0.7f);
            break;
        case 1: // Schizophrenia: shifting purple
            tintR = 0.15f + 0.1f * sinf(horrorTime * 3.0f);
            tintG = 0.03f;
            tintB = 0.2f + 0.08f * cosf(horrorTime * 4.0f);
            tintA = 0.18f;
            break;
        case 2: // Insomnia: cold flickering white
            tintR = 0.25f; tintG = 0.25f; tintB = 0.3f;
            tintA = 0.10f + 0.08f * fabsf(sinf(horrorTime * 7.0f));
            break;
        case 3: // Scotophobia: deep purple
            tintR = 0.08f; tintG = 0.0f; tintB = 0.12f;
            tintA = 0.28f;
            break;
        }
        if (tintA > 0.01f) {
            u32 tintColor = (static_cast<u32>(tintR * 255) << 24) |
                            (static_cast<u32>(tintG * 255) << 16) |
                            (static_cast<u32>(tintB * 255) << 8) | 0xFF;
            AEGfxSetTransparency(tintA);
            DrawSquareMesh(squareMesh, 0.0f, 0.0f, 1600.0f, 900.0f, tintColor);
            AEGfxSetTransparency(1.0f);
        }
    }

    // Phase transition flash
    if (myBoss.phaseTransitionTimer > 0.0f) {
        float flashAlpha = myBoss.phaseTransitionTimer * 2.0f;
        if (flashAlpha > 1.0f) flashAlpha = 1.0f;
        DrawBlackOverlay(squareMesh, flashAlpha * 0.85f);
    }


    // --- DRAW TEXTURED PLAYER ---
    if (player_walk1 && player_walk2 && player_crouch)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

        // Tint red if hit — multiply removes G/B, add pushes R to full bright red
        if (isPlayerHit) {
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
            AEGfxSetColorToAdd(0.6f, 0.0f, 0.0f, 0.0f);
        }
        else {
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        }

        AEMtx33 scale, trans, transform;

        // Flip texture horizontally based on facing direction
        float dirScale = facingRight ? 70.0f : -70.0f;

        // Dynamically adjust visual scaling and Y position based on crouch state
        float drawHeight = isCrouching ? 210.0f : 240.0f;
        float drawY = isCrouching ? playerY - 15.0f : playerY;

        AEMtx33Scale(&scale, dirScale, drawHeight);
        AEMtx33Trans(&trans, playerX + shakeOX, drawY + shakeOY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);

        // Select texture based on crouch state first, then fallback to walk animation
        AEGfxTexture* currentTex = (playerWalkAnim.currentFrame == 0) ? player_walk1 : player_walk2;
        AEGfxTextureSet(currentTex, 0.0f, 0.0f);

        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // Reset tint and render mode
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    }
    else
    {
        // Fallback to drawing a colored rectangle if texture fails to load
        float drawHeight = isCrouching ? 210.0f : 240.0f;
        float drawY = isCrouching ? playerY - 15.0f : playerY;
        u32 playerColor = isPlayerHit ? 0xFF0000FF : COLOR_WHITE;
        DrawSquareMesh(squareMesh, playerX, drawY, 70.0f, drawHeight, playerColor);
    }

    // Shared shoulder position used by both gun and arm draws
    {
        float drawY     = isCrouching ? playerY - 30.0f : playerY;
        float shoulderX = playerX - 7.0f;
        float shoulderY = drawY + 20.0f;

        // --- DRAW GUN FIRST (behind arm) ---
        // Gun center is shifted perpendicular to the aim ("above" the barrel axis) so the
        // grip, which hangs below the barrel, aligns with the arm's hand position.
        if (gun_tex)
        {
            const float GRIP_OFFSET = 10.0f; // perpendicular shift so grip meets the hand
            float gunCX = shoulderX + cosf(armAngle) * (ARM_LENGTH + GUN1_LENGTH * 0.5f) - sinf(armAngle) * GRIP_OFFSET;
            float gunCY = shoulderY + sinf(armAngle) * (ARM_LENGTH + GUN1_LENGTH * 0.5f) + cosf(armAngle) * GRIP_OFFSET;
            float gunH  = facingRight ? GUN1_WIDTH : -GUN1_WIDTH;
            if (isPlayerHit) {
                AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
                AEGfxSetColorToAdd(0.6f, 0.0f, 0.0f, 0.0f);
            }
            DrawTextureMeshRotated(squareMesh, gun_tex, gunCX, gunCY, GUN1_LENGTH, gunH, armAngle, 1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        }

        // --- DRAW ARM ON TOP OF GUN ---
        // Arm overlays the grip area of the gun so it looks like the player is holding it.
        if (player_arm_gun)
        {
            const float TOTAL_LENGTH = ARM_LENGTH + GUN_LENGTH; // texture spans shoulder -> hand tip
            float texCX    = shoulderX + cosf(armAngle) * (TOTAL_LENGTH * 0.5f);
            float texCY    = shoulderY + sinf(armAngle) * (TOTAL_LENGTH * 0.5f);
            float texHeight = facingRight ? ARM_WIDTH : -ARM_WIDTH;
            if (isPlayerHit) {
                AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
                AEGfxSetColorToAdd(0.6f, 0.0f, 0.0f, 0.0f);
            }
            DrawTextureMeshRotated(squareMesh, player_arm_gun, texCX, texCY, TOTAL_LENGTH, texHeight, armAngle, 1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // Arena encroachment — bars grow taller in later phases
    {
        float barHeight = 100.0f;
        if (isHorror) barHeight += myBoss.phase * 20.0f;
        DrawSquareMesh(squareMesh, shakeOX, -400.0f + shakeOY, 1600.0f, barHeight, COLOR_BLACK);
        DrawSquareMesh(squareMesh, shakeOX,  400.0f + shakeOY, 1600.0f, barHeight, COLOR_BLACK);
    }

    Bullets_Draw(0.0f);
    Boss_Draw(myBoss, squareMesh);

    // Second darkness pass after sprites — darkens everything including player/boss
    if (isHorror) {
        float hpPct2 = static_cast<float>(myBoss.health) / static_cast<float>(BOSS_MAX_HEALTH);
        float afterAlpha = 0.12f + (1.0f - hpPct2) * 0.25f + shieldDarkenAlpha * 0.4f;
        if (afterAlpha > 0.5f) afterAlpha = 0.5f;
        DrawBlackOverlay(squareMesh, afterAlpha);
    }

    Player_DrawHealthBar(squareMesh);
    Bullets_DrawAmmoUI();
    Boss_DrawHealthBar(myBoss, squareMesh);
    // Guide clipboard icon anchored just right of the boss HP bar (bar right edge ≈ 679)
    Guide_DrawIconAt(739.0f, 375.0f);

    // Persistent pulsing vignette (horror only)
    if (isHorror) {
        float hpPct2 = static_cast<float>(myBoss.health) / static_cast<float>(BOSS_MAX_HEALTH);
        float pulseBase = 0.03f + (1.0f - hpPct2) * 0.08f;
        float pulseAlpha = pulseBase + 0.03f * sinf(horrorTime * 2.0f);
        if (pulseAlpha > 0.01f) {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(pulseAlpha);
            DrawSquareMesh(squareMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0x550A0020);
            AEGfxSetTransparency(1.0f);
        }
    }

    // Damage vignette — auto-decays; set vignetteAlpha = 1.0f to re-trigger
    if (vignetteAlpha > 0.0f) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(vignetteAlpha * 0.55f);
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0xFF0A0020); // purple-red
        AEGfxSetTransparency(1.0f);
    }

    if (fadeInAlpha > 0.0f)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(fadeInAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- WHISPER TEXT OVERLAYS (horror only) ---
    if (isHorror && whisperTextAlpha > 0.0f && victoryFontId >= 0 && !isVictorySequence) {
        AEGfxSetTransparency(whisperTextAlpha * 0.7f);
        AEGfxPrintWithGlow(victoryFontId, WHISPER_TEXTS[whisperTextIndex],
            whisperTextX, whisperTextY, whisperTextScale,
            0.6f, 0.2f, 0.8f, 1.0f,
            0.3f, 0.0f, 0.4f, 0.8f,
            0.005f);
        AEGfxSetTransparency(1.0f);
    }

    // --- DRAW VICTORY SEQUENCE ---
    if (isVictorySequence) {
        // 1. Fade in a dark teal overlay (matches the hospital/success tone)
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(victoryAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.04f, 0.06f, 1.0f);  // very dark teal instead of pure black
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // 2. Draw text once the screen is fully faded
        if (victoryAlpha >= 1.0f && victoryFontId >= 0) {
            AEGfxSetTransparency(1.0f);

            // Centering helpers — NDC width per character scales with font pt size.
            // Size 35 title  → ~0.042f/char   (calibrated from Endgame's 0.036f at size 30)
            // Size 20 subtitle → ~0.024f/char  (0.036f * 20/30)
            auto titleX = [](std::string_view text) -> float {
                return -(static_cast<float>(text.size()) * 0.042f * 0.5f);
            };
            auto subX = [](std::string_view text) -> float {
                return -(static_cast<float>(text.size()) * 0.024f * 0.5f);
            };

            // --- Title: "PATIENT SUBDUED" with green glow ---
            AEGfxPrintWithGlow(victoryFontId, "PATIENT SUBDUED",
                titleX("PATIENT SUBDUED"), 0.22f, 1.0f,
                0.3f, 1.0f, 0.65f, 1.0f,    // bright teal-green text
                0.0f, 0.45f, 0.25f, 0.9f,   // dark teal glow
                0.004f);

            // --- Subtitle ---
            if (victorySubFontId >= 0) {
                AEGfxPrintWithGlow(victorySubFontId, "The patient has been successfully contained.",
                    subX("The patient has been successfully contained."), 0.02f, 1.0f,
                    0.85f, 0.95f, 0.9f, 1.0f,   // soft white-teal text
                    0.0f, 0.2f, 0.15f, 0.5f,    // subtle green glow
                    0.003f);

                // --- Blinking prompt ---
                const char* promptText = Settings_IsHorrorLessEnabled()
                    ? "Returning to ward..."
                    : "...silence restored.";
                float blinkAlpha = (sinf(victoryHoldTimer * 4.0f) + 1.0f) * 0.5f;
                AEGfxSetTransparency(blinkAlpha);
                AEGfxPrintWithGlow(victorySubFontId, promptText,
                    subX(promptText), -0.3f, 1.0f,
                    0.5f, 0.8f, 0.65f, 1.0f,   // dim teal
                    0.0f, 0.2f, 0.1f, 0.4f,
                    0.002f);
                AEGfxSetTransparency(1.0f);
            }
        }
    }
    AEGfxSetTransparency(1.0f);

    // --- BOSS DEBUG OVERLAY ---
    {
        int active_count = 0;
        for (int i = 0; i < MAX_BOSS_BULLETS; i++)
            if (myBoss.bullets[i].active) active_count++;

        BossDebugInfo dbg{};
        dbg.player_x           = playerX;
        dbg.player_y           = playerY;
        dbg.player_vel_x       = playerVelX;
        dbg.player_vel_y       = playerVelY;
        dbg.player_hp          = playerHP;
        dbg.player_hp_max      = MAX_PLAYER_HP;
        dbg.is_grounded        = isGrounded;
        dbg.is_crouching       = isCrouching;
        dbg.boss_health        = myBoss.health;
        dbg.boss_max_health    = BOSS_MAX_HEALTH;
        dbg.boss_x             = myBoss.x;
        dbg.boss_y             = myBoss.y;
        dbg.boss_base_y        = myBoss.baseY;
        dbg.boss_state         = myBoss.currentState;
        dbg.boss_state_timer   = myBoss.stateTimer;
        dbg.boss_shield_active = myBoss.shieldActive;
        dbg.boss_dodge_cooldown = bossDodgeCooldown;
        dbg.active_bullet_count = active_count;
        BossDebug_Draw(dbg);
    }

    PauseMenu_Draw(squareMesh);
    Guide_Draw();
}

void Boss_Fight_Free() {
    AudioManager_StopBGM(); // Ensure boss music stops when leaving the fight
}

void Boss_Fight_Unload() {
    BossDebug_Unload();
    Boss_Unload();
    FreeMeshSafe(squareMesh);
    FreeMeshSafe(circleMesh);
    Bullets_Free();

    // Clean up textures to prevent memory leaks
    UnloadTextureSafe(wallBgTexture);
    UnloadTextureSafe(player_walk1);
    UnloadTextureSafe(player_walk2);
    UnloadTextureSafe(player_crouch);
    UnloadTextureSafe(player_arm_gun);
    UnloadTextureSafe(gun_tex);

    if (victoryFontId >= 0) {
        AEGfxDestroyFont(victoryFontId);
        victoryFontId = -1;
    }
    if (victorySubFontId >= 0) {
        AEGfxDestroyFont(victorySubFontId);
        victorySubFontId = -1;
    }
}

void Boss_Fight_TryDodge()
{
    // Called from bullet hit detection so the chance is per bullet that LANDS, not per bullet fired.

    // Play water impact sound on every confirmed hit, throttled to avoid spam
    static float impactSoundCooldown = 0.0f;
    impactSoundCooldown -= static_cast<f32>(AEFrameRateControllerGetFrameTime());
    if (impactSoundCooldown <= 0.0f) {
        AudioManager_PlaySFX(SoundID::SFX_WATER_IMPACT, 0.25f);
        impactSoundCooldown = 0.1f; // Cap at ~10 impact sounds per second
    }

    // Only dodge when the boss is active, unshielded, not vanished, and the cooldown has expired.
    if (!myBoss.active || myBoss.shieldActive || myBoss.isVanished || bossDodgeCooldown > 0.0f)
        return;

    if (RandInt(0, 99) < 15) // 15% chance per hit
    {
        if (!Settings_IsHorrorLessEnabled()) {
            // Horror teleport: vanish, then reappear with burst
            myBoss.isVanished = true;
            myBoss.vanishTimer = (myBoss.phase >= 3) ? 0.7f : 0.4f;
            myBoss.vanishReappearY = (myBoss.baseY > 0) ? -200.0f : 200.0f;
            bossDodgeCooldown = (myBoss.phase >= 3) ? 1.5f : 3.0f;
            AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.3f, 0.8f + static_cast<float>(RandInt(0, 40)) / 100.0f);
            bossShakeTimer = 0.15f;
        } else {
            myBoss.baseY = (myBoss.baseY > 0) ? -200.0f : 200.0f;
            bossDodgeCooldown = 3.0f;
            AudioManager_PlaySFX(SoundID::SFX_DODGE, 0.3f);
        }
    }
}
#include "pch.hpp"
#include "BossDebug.hpp"

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;

// Textures for level background and player character
static AEGfxTexture* wallBgTexture = nullptr;
static AEGfxTexture* player_walk1 = nullptr;
static AEGfxTexture* player_walk2 = nullptr;
static AEGfxTexture* player_crouch = nullptr;
static AEGfxTexture* player_arm_gun = nullptr;

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
int playerHP = 10;
const int MAX_PLAYER_HP = 10;
bool isPlayerHit = false;
float hitTimer = 0.0f;
static float vignetteAlpha = 0.0f; // Set to 1.0f to flash the damage vignette overlay
static float lagHpPercent  = 1.0f; // Trails behind hpPercent for the ghost-bar effect

// --- PLAYER ANIMATION VARIABLES ---
static int currentPlayerFrame = 0;
static float playerAnimTimer = 0.0f;
const float PLAYER_ANIM_SPEED = 0.15f; // Time in seconds per frame

// --- WEAPON: HOLY WATER GUN ---
static float fireCooldown = 0.0f;
const float FIRE_RATE = 0.02f; // Fires a particle every 0.02 seconds for a continuous stream
static float bossDodgeCooldown = 0.0f;

// --- ARM / GUN ---
static float armAngle = 0.0f; // angle from shoulder to mouse (radians)
const float ARM_LENGTH = 45.0f;
const float ARM_WIDTH  = 24.0f;
const float GUN_LENGTH = 32.0f;
const float GUN_WIDTH  = 10.0f;

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
static s8 victoryFontId = -1;

void Player_DrawHealthBar(AEGfxVertexList* mesh)
{
    float hpPercent = (float)playerHP / (float)MAX_PLAYER_HP;
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
        DrawSquareMesh(mesh, fillX, cy, fillW, barH, 0xCC0000FF);

        // Thin gloss stripe along the top edge
        float hiH = barH * 0.22f;
        float hiY = cy + (barH - hiH) * 0.5f;
        AEGfxSetTransparency(0.25f);
        DrawSquareMesh(mesh, fillX, hiY, fillW, hiH, 0xFF6666FF);
        AEGfxSetTransparency(1.0f);
    }

    // "HP" label to the left of the bar
    if (victoryFontId >= 0) {
        float ndcX = (barLeft - 36.0f) / 800.0f;
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
    victoryFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 35);
    wallBgTexture = LoadTextureChecked(Assets::Background::WallBg);

    // Load player textures
    player_walk1   = LoadTextureChecked(Assets::Boss_Fight::PlayerShooting1);
    player_walk2   = LoadTextureChecked(Assets::Boss_Fight::PlayerShooting2);
    player_crouch  = LoadTextureChecked(Assets::Boss_Fight::PlayerCrouch); // Placeholder for crouch texture
    player_arm_gun = LoadTextureChecked(Assets::Boss_Fight::PlayerArmGun);

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
    AudioManager_PlayBGM(BGM_MAIN_MENU, 0.4f);

    // Auto-open the guide to the Boss Fight instructions page
    Guide_BossFight();
}

void Boss_Fight_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    BossDebug_Update(myBoss);
    PauseMenu_Update(dt);
    if (PauseMenu_IsPaused()) return;

    Guide_Update(false, dt, false);
    if (IsGuideActive()) return;

    if (myBoss.health <= 0 && !isVictorySequence) {
        isVictorySequence = true;
        AudioManager_StopBGM();                        // Cut boss music on defeat
        AudioManager_PlaySFX(SFX_SCREAMING, 0.5f);    // Boss defeat scream
    }

    // Run the victory animation and block all gameplay input
    if (isVictorySequence) {
        victoryAlpha += 1.0f * dt; // Fade the screen to black over 1 second
        if (victoryAlpha >= 1.0f) {
            victoryAlpha = 1.0f;
            victoryHoldTimer += dt;

            // Show the text for 3 seconds, then return to the main game
            if (victoryHoldTimer > 3.0f) {

                Player_GenerateMission();
                Timer_SetPaused(false);

                next = GAME_STATE;
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
        float worldMouseX = (float)cursorX - (AEGfxGetWindowWidth()  / 2.0f);
        float worldMouseY = (AEGfxGetWindowHeight() / 2.0f) - (float)cursorY;

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
            playerAnimTimer += dt;
            if (playerAnimTimer >= PLAYER_ANIM_SPEED) {
                playerAnimTimer = 0.0f;
                currentPlayerFrame = (currentPlayerFrame == 0) ? 1 : 0; // Toggle frame between 0 and 1 to create the walking effect
            }
        }
        else {
            currentPlayerFrame = 0; // Reset to default standing frame when idle
            playerAnimTimer = 0.0f;
        }

        if (AEInputCheckTriggered(AEVK_SPACE)) {
            if (isGrounded || jumpCount < MAX_JUMPS) {
                playerVelY = JUMP_FORCE;
                isGrounded = false;
                jumpCount++;
                // Horror-less mode uses a silly WEOW instead of a grunt
                if (Settings_IsHorrorLessEnabled())
                    AudioManager_PlaySFX(SFX_WEOW, 0.6f);
                else
                    AudioManager_PlaySFX(SFX_JUMP_GRUNT, 0.5f);
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
                    AudioManager_PlaySFX(SFX_WATER_GUN_SQUIRT, 0.12f, 0.7f);

                // Bullet spawns from the gun tip
                float drawY     = isCrouching ? playerY - 30.0f : playerY;
                float shoulderX = playerX - 7.0f;
                float shoulderY = drawY + 20.0f;
                float startX = shoulderX + cosf(armAngle) * (ARM_LENGTH + GUN_LENGTH);
                float startY = shoulderY + sinf(armAngle) * (ARM_LENGTH + GUN_LENGTH);

                float dirX = cosf(armAngle);
                float dirY = sinf(armAngle);

                // Add a slight randomized spread to simulate spraying liquid
                float spreadX = ((rand() % 100 - 50) / 1000.0f); // Random spread value between -0.05 and 0.05
                float spreadY = ((rand() % 100 - 50) / 1000.0f);
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
        currentPlayerFrame = 0;
        playerAnimTimer = 0.0f;
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
    Boss_Update(myBoss, dt, playerX, playerY);

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
                    if (!BossDebug_IsGodMode()) playerHP--;
                    myBoss.bullets[i].active = false;
                    isPlayerHit = true;
                    hitTimer = 0.2f;
                    vignetteAlpha = 1.0f; // Hook: set to 1.0f to trigger vignette / cracked-glass overlay
                    AudioManager_PlaySFX(SFX_EXHALE, 0.7f);  // Player hurt reaction

                    if (playerHP <= 0 && !BossDebug_IsGodMode()) {
                        Player_StopSounds(); // prevent walking SFX persisting into endgame
                        AudioManager_StopBGM(); // stop boss fight music before endgame
                        currentEndReason = REASON_DIED_TO_BOSS;
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

    // Lag (ghost) bar slowly catches up to actual HP
    {
        float currentHpPct = (float)playerHP / (float)MAX_PLAYER_HP;
        if (lagHpPercent > currentHpPct) {
            lagHpPercent -= dt * 0.45f;
            if (lagHpPercent < currentHpPct) lagHpPercent = currentHpPct;
        }
        else {
            lagHpPercent = currentHpPct; // snap up if HP is restored
        }
    }

    Bullets_Update(dt, 0.0f, myBoss);
}

void Boss_Fight_Draw()
{
    DrawTextureMesh(squareMesh, wallBgTexture, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);


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
        AEMtx33Trans(&trans, playerX, drawY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);

        // Select texture based on crouch state first, then fallback to walk animation
        AEGfxTexture* currentTex = isCrouching ? player_crouch : ((currentPlayerFrame == 0) ? player_walk1 : player_walk2);
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

    // --- DRAW ARM AND GUN (single texture) ---
    if (player_arm_gun)
    {
        const float TOTAL_LENGTH = ARM_LENGTH + GUN_LENGTH; // texture spans shoulder -> gun tip

        float drawY     = isCrouching ? playerY - 30.0f : playerY;
        float shoulderX = playerX - 7.0f; // fixed offset so shoulder stays inside the body on both sides
        float shoulderY = drawY + 20.0f;

        // The texture pivot is its center, so offset half the total length along the aim direction
        float texCX = shoulderX + cosf(armAngle) * (TOTAL_LENGTH * 0.5f);
        float texCY = shoulderY + sinf(armAngle) * (TOTAL_LENGTH * 0.5f);

        // V-flip when facing left: V-flip + 180° arm rotation = correct mirrored arm without upside-down artifact
        float texHeight = facingRight ? ARM_WIDTH : -ARM_WIDTH;

        if (isPlayerHit) {
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
            AEGfxSetColorToAdd(0.6f, 0.0f, 0.0f, 0.0f);
        }
        DrawTextureMeshRotated(squareMesh, player_arm_gun, texCX, texCY, TOTAL_LENGTH, texHeight, armAngle, 1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    DrawSquareMesh(squareMesh, 0.0f, -400.0f, 1600.0f, 100.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, 400.0f, 1600.0f, 100.0f, COLOR_BLACK);

    Bullets_Draw(0.0f);
    Boss_Draw(myBoss, squareMesh);
    Player_DrawHealthBar(squareMesh);
    Bullets_DrawAmmoUI();
    Boss_DrawHealthBar(myBoss, squareMesh);
    // Guide clipboard icon anchored just right of the boss HP bar (bar right edge ≈ 679)
    Guide_DrawIconAt(739.0f, 375.0f);

    // Damage vignette — auto-decays; set vignetteAlpha = 1.0f to re-trigger (e.g., cracked-glass overlay)
    if (vignetteAlpha > 0.0f) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(vignetteAlpha * 0.55f);
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0xFF110000);
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

    // --- DRAW VICTORY SEQUENCE ---
    if (isVictorySequence) {
        // 1. Draw black overlay screen
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(victoryAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // 2. Draw victory text once the screen is fully black
        if (victoryAlpha >= 1.0f && victoryFontId >= 0) {
            AEGfxSetTransparency(1.0f);

            // -1.0f adjusts the X position
            AEGfxPrint(victoryFontId, "Patient Successfully Subdued", -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
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

    if (victoryFontId >= 0) {
        AEGfxDestroyFont(victoryFontId);
        victoryFontId = -1;
    }
}

void Boss_Fight_TryDodge()
{
    // Called from bullet hit detection so the chance is per bullet that LANDS, not per bullet fired.

    // Play water impact sound on every confirmed hit, throttled to avoid spam
    static float impactSoundCooldown = 0.0f;
    impactSoundCooldown -= (f32)AEFrameRateControllerGetFrameTime();
    if (impactSoundCooldown <= 0.0f) {
        AudioManager_PlaySFX(SFX_WATER_IMPACT, 0.5f);
        impactSoundCooldown = 0.1f; // Cap at ~10 impact sounds per second
    }

    // Only dodge when the boss is active, unshielded, and the cooldown has expired.
    if (!myBoss.active || myBoss.shieldActive || bossDodgeCooldown > 0.0f)
        return;

    if (rand() % 100 < 15) // 15% chance per hit
    {
        myBoss.baseY = (myBoss.baseY > 0) ? -200.0f : 200.0f;
        bossDodgeCooldown = 3.0f; // Longer cooldown so the boss has time to reach the new position
        AudioManager_PlaySFX(SFX_DODGE, 0.6f); // Boss dodge whoosh
    }
}
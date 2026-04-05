#include "pch.hpp"
#include "Animation.hpp"

// --- Boss Textures ---
static AEGfxTexture* bossTex[3]          = { nullptr, nullptr, nullptr };
static AEGfxTexture* bossShieldTex[3]    = { nullptr, nullptr, nullptr };
static AEGfxTexture* bossBulletFrames[2]        = { nullptr, nullptr };
static AEGfxTexture* bossScatterBulletFrames[2] = { nullptr, nullptr };

// --- Boss Animation ---
static Animation bossAnim   = { 0, 0.0f, 0.35f, 3, true, 1, true };  // ping-pong 3 frames
static Animation bulletAnim = { 0, 0.0f, 0.1f,  2, true, 1, false }; // loop 2 frames

// --- Boss HUD ---
static s8    bossHudFontId    = -1;
static float interactBobTimer = 0.0f;



void Boss_Load()
{
    bossTex[0] = LoadTextureChecked(Assets::Boss_Fight::Boss1);
    bossTex[1] = LoadTextureChecked(Assets::Boss_Fight::Boss2);
    bossTex[2] = LoadTextureChecked(Assets::Boss_Fight::Boss3);
    bossShieldTex[0] = LoadTextureChecked(Assets::Boss_Fight::Shield1);
    bossShieldTex[1] = LoadTextureChecked(Assets::Boss_Fight::Shield2);
    bossShieldTex[2] = LoadTextureChecked(Assets::Boss_Fight::Shield3);
    bossBulletFrames[0]        = LoadTextureChecked(Assets::Boss_Fight::Bullet1);
    bossBulletFrames[1]        = LoadTextureChecked(Assets::Boss_Fight::Bullet2);
    bossScatterBulletFrames[0] = LoadTextureChecked(Assets::Boss_Fight::ScatterBullet1);
    bossScatterBulletFrames[1] = LoadTextureChecked(Assets::Boss_Fight::ScatterBullet2);
    bossHudFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 26);
}

void Boss_Initialize(Boss& boss)
{
    boss.active = true;
    boss.health = BOSS_MAX_HEALTH;

    // Overall bounds (used mainly for drawing the shield and fallback square)
    boss.w = 240.0f;
    boss.h = 480.0f;

    boss.x = 600.0f;
    boss.y = 0.0f;
    boss.baseY = 0.0f;

    boss.moveTimer = 0.0f;
    boss.shootTimer = 2.0f;

    boss.currentState = 0;
    boss.stateTimer = 4.0f;
    boss.shieldActive = false;

    // Horror phase system
    boss.phase = 0;
    boss.phaseTransitionTimer = 0.0f;
    boss.flickerTimer = 0.0f;
    boss.flickerVisible = true;
    boss.isVanished = false;
    boss.vanishTimer = 0.0f;
    boss.vanishReappearY = 0.0f;

    // Reset static animation state so replays start from frame 0
    bossAnim.Reset();
    bulletAnim.Reset();

    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        boss.bullets[i].active = false;
    }
}

void Boss_Update(Boss& boss, float dt, float playerX, float playerY)
{
    // --- HORROR PHASE COMPUTATION ---
    float hpPercent = static_cast<float>(boss.health) / static_cast<float>(BOSS_MAX_HEALTH);
    int newPhase = 0;
    if (hpPercent <= 0.25f)      newPhase = 3; // Scotophobia
    else if (hpPercent <= 0.50f) newPhase = 2; // Insomnia
    else if (hpPercent <= 0.75f) newPhase = 1; // Schizophrenia
    else                         newPhase = 0; // Paranoia

    if (newPhase != boss.phase) {
        boss.phase = newPhase;
        boss.phaseTransitionTimer = 0.5f;
        if (!Settings_IsHorrorLessEnabled())
            AudioManager_PlaySFX(SoundID::SFX_EXHALE, 0.15f, 0.5f);
    }
    if (boss.phaseTransitionTimer > 0.0f)
        boss.phaseTransitionTimer -= dt;

    // --- HORROR TELEPORT (vanish phase) ---
    if (boss.isVanished) {
        boss.vanishTimer -= dt;
        if (boss.vanishTimer <= 0.0f) {
            boss.isVanished = false;
            boss.baseY = boss.vanishReappearY;
            boss.y = boss.vanishReappearY;
            boss.flickerVisible = true;

            // Immediate 3-shot burst on reappearance
            float handX = boss.x - 120.0f;
            float handY = boss.y + 20.0f;
            int fired = 0;
            for (int i = 0; i < MAX_BOSS_BULLETS && fired < 3; i++) {
                if (!boss.bullets[i].active) {
                    boss.bullets[i].active = true;
                    boss.bullets[i].is_scatter = false;
                    boss.bullets[i].hp = 15;
                    boss.bullets[i].x = handX;
                    boss.bullets[i].y = handY;
                    float diffX = playerX - handX;
                    float diffY = playerY - handY;
                    float spread = (fired - 1) * 0.15f;
                    float angle = atan2f(diffY, diffX) + spread;
                    boss.bullets[i].dirX = cosf(angle) * 700.0f;
                    boss.bullets[i].dirY = sinf(angle) * 700.0f;
                    fired++;
                }
            }
        }
        // While vanished, still advance bullets
        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (boss.bullets[i].active) {
                boss.bullets[i].x += boss.bullets[i].dirX * dt;
                boss.bullets[i].y += boss.bullets[i].dirY * dt;
                if (boss.bullets[i].x < -1000 || boss.bullets[i].x > 1000 || boss.bullets[i].y < -1000 || boss.bullets[i].y > 1000)
                    boss.bullets[i].active = false;
            }
        }
        bulletAnim.Update(dt);
        return;
    }

    // --- BOSS SPRITE FLICKER (all phases, horror only) ---
    if (!Settings_IsHorrorLessEnabled()) {
        boss.flickerTimer -= dt;
        if (boss.flickerTimer <= 0.0f) {
            boss.flickerVisible = !boss.flickerVisible;
            float maxOn  = (boss.phase >= 3) ? 0.08f : (boss.phase >= 2) ? 0.12f : (boss.phase >= 1) ? 0.22f : 0.5f;
            float maxOff = (boss.phase >= 3) ? 0.18f : (boss.phase >= 2) ? 0.12f : (boss.phase >= 1) ? 0.07f : 0.05f;
            boss.flickerTimer = boss.flickerVisible
                ? (0.02f + static_cast<float>(RandInt(0, 99)) / 100.0f * maxOn)
                : (0.01f + static_cast<float>(RandInt(0, 99)) / 100.0f * maxOff);
        }
    } else {
        boss.flickerVisible = true;
    }

    // Boss body animation — ping-pong: 0→1→2→1→0→1→2…
    bossAnim.Update(dt);

    // Bullet sprite animation
    bulletAnim.Update(dt);

    boss.moveTimer += dt * 2.0f;
    float hoverSpeed = boss.shieldActive ? 4.0f : 2.0f;
    float hoverOffset = sinf(boss.moveTimer * hoverSpeed) * 100.0f;

    float targetY = boss.baseY + hoverOffset;
    boss.y += (targetY - boss.y) * 4.0f * dt;

    // Prevent clipping: Ceiling is 350, Floor is -350. Boss half-height is 240.
    if (boss.y > 110.0f) boss.y = 110.0f;
    if (boss.y < -110.0f) boss.y = -110.0f;

    boss.stateTimer -= dt;

    static bool prevShieldActive = false;

    if (boss.stateTimer <= 0.0f) {
        boss.currentState = RandInt(0, 2);

        if (boss.currentState == 0) {
            boss.stateTimer = 4.0f;
            boss.shieldActive = false;
        }
        else if (boss.currentState == 1) {
            boss.stateTimer = 3.0f;
            boss.shieldActive = false;
        }
        else if (boss.currentState == 2) {
            boss.stateTimer = 3.5f;
            boss.shieldActive = true;
            boss.shootTimer = 0.5f;
            AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.3f); // Eerie whisper when shield activates
        }
    }

    // Play shield break sound when the boss drops its shield
    if (prevShieldActive && !boss.shieldActive) {
        AudioManager_PlaySFX(SoundID::SFX_SHIELD_BREAK, 0.10f);
    }
    prevShieldActive = boss.shieldActive;

    // --- NEW: CALCULATE HAND POSITION ---
    // This targets the left arm stretching out towards the player
    float handX = boss.x - 120.0f;
    float handY = boss.y + 20.0f;

    boss.shootTimer -= dt;

    // State 0: Normal Targeted Shot (From the Hand!)
    if (boss.currentState == 0 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.0f;
        // Eerie whisper for normal shot — less repetitive than constant screaming
        if (!Settings_IsHorrorLessEnabled())
            AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.08f, 0.9f + static_cast<float>(RandInt(0, 20)) / 100.0f);

        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active    = true;
                boss.bullets[i].is_scatter = false;
                boss.bullets[i].hp        = 15;
                boss.bullets[i].x         = handX; // Spawn at Hand
                boss.bullets[i].y = handY;

                float diffX = playerX - handX;
                float diffY = playerY - handY;
                float length = sqrtf(diffX * diffX + diffY * diffY);

                if (length != 0) {
                    boss.bullets[i].dirX = (diffX / length) * 600.0f;
                    boss.bullets[i].dirY = (diffY / length) * 600.0f;
                }
                else {
                    boss.bullets[i].dirX = 0.0f;
                    boss.bullets[i].dirY = 0.0f;
                }
                break;
            }
        }
    }
    // State 1: Spread Attack (From the Hand!)
    else if (boss.currentState == 1 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.5f;
        // Spread attack: whisper cue instead of overlapping screams
        if (!Settings_IsHorrorLessEnabled())
            AudioManager_PlaySFX(SoundID::SFX_VOICE_WHISPER, 0.06f, 0.7f);

        float diffX = playerX - handX;
        float diffY = playerY - handY;
        float baseAngle = atan2f(diffY, diffX);

        int bulletsFired = 0;
        for (int i = 0; i < MAX_BOSS_BULLETS && bulletsFired < 5; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active     = true;
                boss.bullets[i].is_scatter = true;
                boss.bullets[i].hp         = 10;
                boss.bullets[i].x          = handX; // Spawn at Hand
                boss.bullets[i].y = handY;

                float spreadAngle = baseAngle + ((bulletsFired - 2) * 0.2f);

                boss.bullets[i].dirX = cosf(spreadAngle) * 500.0f;
                boss.bullets[i].dirY = sinf(spreadAngle) * 500.0f;

                bulletsFired++;
            }
        }
    }
    // State 2: Shield Phase
    else if (boss.currentState == 2 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 0.5f;
        if (boss.health < BOSS_MAX_HEALTH) {
            boss.health = (std::min)(boss.health + 15, BOSS_MAX_HEALTH);
        }
    }

    // Advance interact-prompt bob animation
    interactBobTimer += dt;

    // --- UPDATE BOSS BULLETS ---
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            boss.bullets[i].x += boss.bullets[i].dirX * dt;
            boss.bullets[i].y += boss.bullets[i].dirY * dt;

            if (boss.bullets[i].x < -1000 || boss.bullets[i].x > 1000 || boss.bullets[i].y < -1000 || boss.bullets[i].y > 1000) {
                boss.bullets[i].active = false;
            }
        }
    }
}

void Boss_DrawHealthBar(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    float hpPercent = static_cast<float>(boss.health) / static_cast<float>(BOSS_MAX_HEALTH);
    if (hpPercent < 0.0f) hpPercent = 0.0f;

    // TOP CENTER — ~84% of the 1600px world-space screen width
    const float barWidth  = 1350.0f;
    const float barHeight =  28.0f;
    const float startX    =   0.0f;   // horizontally centered
    const float startY    = 375.0f;   // just inside the top black bar
    const float pad       =   4.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Purple pulse timer — advances while shielded, resets otherwise
    static float shieldPulseTimer = 0.0f;
    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());
    if (boss.shieldActive) shieldPulseTimer += dt;
    else                   shieldPulseTimer  = 0.0f;

    // pulse: 0.0 = dark purple, 1.0 = bright purple
    float pulse = (sinf(shieldPulseTimer * 3.5f) + 1.0f) * 0.5f;

    // Build pulsing purple fill color (RGBA)
    u32 pr = static_cast<u32>(0x44 + static_cast<int>(pulse * (0xAA - 0x44)));
    u32 pb = static_cast<u32>(0x66 + static_cast<int>(pulse * (0xFF - 0x66)));
    u32 purpleColor = (pr << 24) | (0x00u << 16) | (pb << 8) | 0xFFu;

    // Outer frame: pulses between dark/bright purple when shielded, near-black otherwise
    u32 borderColor = boss.shieldActive ? purpleColor : 0xFF0D0008u;
    DrawSquareMesh(mesh, startX, startY, barWidth + pad * 2.0f, barHeight + pad * 2.0f, borderColor);
    // Dark corrupted background trough
    DrawSquareMesh(mesh, startX, startY, barWidth, barHeight, 0xFF1A0012);

    if (boss.health > 0) {
        float fillW = barWidth * hpPercent;
        float fillX = startX - barWidth * 0.5f + fillW * 0.5f;

        // Fill: pulsing purple when shielded, blood-crimson otherwise
        u32 fillColor = boss.shieldActive ? purpleColor : 0xCC0000FFu;
        DrawSquareMesh(mesh, fillX, startY, fillW, barHeight, fillColor);

        // Thin glossy highlight stripe along the top edge
        float hiH = barHeight * 0.22f;
        float hiY = startY + (barHeight - hiH) * 0.5f;
        AEGfxSetTransparency(0.30f);
        u32 glossColor = boss.shieldActive ? 0xDD88FFFFu : 0xFF4444FFu;
        DrawSquareMesh(mesh, fillX, hiY, fillW, hiH, glossColor);
        AEGfxSetTransparency(1.0f);
    }

    // Boss name — printed ABOVE the bar
    // Bar outer top edge = (375 + 14 + 4) = 393 world Y → NDC ≈ 0.873
    // Tune x to horizontally center the string for your font; 0.85 scale on size-26 font
    if (bossHudFontId >= 0) {
        AEGfxPrint(bossHudFontId, "THE SHADOW", -0.185f, 0.895f, 0.85f,
                   0.85f, 0.55f, 0.70f, 1.0f);
    }
}

// Draws a static [G] interact prompt at top-right, level with the boss HP bar.
// interactBobTimer (advanced in Boss_Update) drives a gentle atmospheric bob.
void Boss_DrawInteractPrompt(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    // Fixed anchor: just right of the boss HP bar's right edge
    // Bar width = 1350, half = 675, + 4 pad + ~50 margin = 729
    const float baseCX = 729.0f;
    const float baseCY = 375.0f; // same world Y as the boss HP bar

    // Gentle sine bob (amplitude 5 px) — purely atmospheric, not position-tracking
    float bob = sinf(interactBobTimer * 2.1f) * 5.0f;
    float cy  = baseCY + bob;

    // Tattered-clipboard background
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.85f);
    DrawSquareMesh(mesh, baseCX, cy, 80.0f, 32.0f, 0xFF2A1E10);           // dark sepia body
    AEGfxSetTransparency(0.35f);
    DrawSquareMesh(mesh, baseCX, cy + 12.0f, 80.0f, 4.0f, 0xFF6B4F2A);   // worn top stripe
    AEGfxSetTransparency(1.0f);

    // "[G]" label — NDC = worldPos / half-screen-extent (800 wide, 450 tall)
    if (bossHudFontId >= 0) {
        float ndcX = (baseCX - 26.0f) / 800.0f;
        float ndcY = (cy     -  7.0f) / 450.0f;
        AEGfxPrint(bossHudFontId, "[G]", ndcX, ndcY, 0.70f,
                   0.90f, 0.82f, 0.65f, 1.0f); // parchment-white
    }
}

void Boss_Draw(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    // Skip drawing boss body while vanished (mid-teleport)
    if (!boss.isVanished) {
        AEGfxTexture* activeTex = boss.shieldActive
            ? bossShieldTex[bossAnim.currentFrame]
            : bossTex[bossAnim.currentFrame];

        float flickerAlpha = boss.flickerVisible ? 1.0f : 0.12f;

        // Shield phase: additive purple glow (horror only)
        if (boss.shieldActive && !Settings_IsHorrorLessEnabled())
            AEGfxSetColorToAdd(0.15f, 0.0f, 0.25f, 0.0f);

        if (activeTex) {
            DrawTextureMesh(mesh, activeTex, boss.x, boss.y, boss.w, boss.h, flickerAlpha);
        }
        else {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetTransparency(flickerAlpha);
            DrawSquareMesh(mesh, boss.x, boss.y, boss.w, boss.h, 0xFFFF00FF);
            AEGfxSetTransparency(1.0f);
        }
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // =================================================================
    // DEBUG: UNCOMMENT THIS BLOCK TO SEE THE COMPOUND HITBOXES!
    // Feel free to tweak the numbers in Boss_CheckCollision to fit perfectly
    // =================================================================
    
    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    //AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    //AEGfxSetTransparency(0.4f);
    //DrawSquareMesh(mesh, boss.x + 20.0f, boss.y + 110.0f, 100.0f, 180.0f, 0xFF00FFFF); // Head/Torso
    //DrawSquareMesh(mesh, boss.x - 70.0f, boss.y - 30.0f, 140.0f,  50.0f, 0xFF00FFFF); // Reaching Arm
    //DrawSquareMesh(mesh, boss.x + 70.0f, boss.y - 60.0f, 60.0f, 150.0f, 0xFF00FFFF); // Skirt/Legs
    //DrawSquareMesh(mesh, boss.x + 90.0f, boss.y - 120.0f, 50.0f, 240.0f, 0xFF00FFFF); // Dangling Arm
    //AEGfxSetTransparency(1.0f);
    

    // Draw Boss Bullets
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            AEGfxTexture* currentTex = boss.bullets[i].is_scatter
                ? bossScatterBulletFrames[bulletAnim.currentFrame]
                : bossBulletFrames[bulletAnim.currentFrame];
            if (currentTex) {
                float angle = atan2f(boss.bullets[i].dirY, boss.bullets[i].dirX);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                // Red tint on bullets during shield phase (horror only)
                if (boss.shieldActive && !Settings_IsHorrorLessEnabled())
                    AEGfxSetColorToAdd(0.5f, 0.0f, 0.0f, 0.0f);
                else
                    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
                AEGfxSetTransparency(1.0f);
                AEGfxTextureSet(currentTex, 0.0f, 0.0f);

                AEMtx33 scale, rot, trans, temp, finalTransform;
                AEMtx33Scale(&scale, 50.0f, 50.0f);
                AEMtx33Rot(&rot, angle);
                AEMtx33Trans(&trans, boss.bullets[i].x, boss.bullets[i].y);

                AEMtx33Concat(&temp, &rot, &scale);
                AEMtx33Concat(&finalTransform, &trans, &temp);

                AEGfxSetTransform(finalTransform.m);
                AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
            }
            else {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                DrawSquareMesh(mesh, boss.bullets[i].x, boss.bullets[i].y, 50.0f, 50.0f, 0xFFFFFFFF);
            }
        }
    }
}

// --- NEW: COMPOUND COLLISION MESH CHECK ---
bool Boss_CheckCollision(Boss& boss, float objX, float objY, float objW, float objH)
{
    if (!boss.active || boss.isVanished) return false;


    // Define the 4 custom hitboxes that wrap tightly around the monster's drawing
    // 1. Head & Torso
    float h1X = boss.x + 20.0f, h1Y = boss.y + 80.0f, h1W = 100.0f, h1H = 240.0f;
    // 2. Reaching Arm (Left)
    float h2X = boss.x - 70.0f, h2Y = boss.y + 20.0f, h2W = 140.0f, h2H = 50.0f;
    // 3. Lower Body / Skirt
    float h3X = boss.x + 60.0f, h3Y = boss.y - 80.0f, h3W = 120.0f, h3H = 160.0f;
    // 4. Dangling Arm (Right)
    float h4X = boss.x + 90.0f, h4Y = boss.y - 120.0f, h4W = 50.0f, h4H = 240.0f;

    // AABB collision logic function
    auto checkAABB = [&](float x1, float y1, float w1, float h1) {
        return (fabsf(x1 - objX) < (w1 + objW) / 2.0f) &&
            (fabsf(y1 - objY) < (h1 + objH) / 2.0f);
        };

    // If the bullet touches ANY of these 4 boxes, it's a hit!
    return checkAABB(h1X, h1Y, h1W, h1H) ||
        checkAABB(h2X, h2Y, h2W, h2H) ||
        checkAABB(h3X, h3Y, h3W, h3H) ||
        checkAABB(h4X, h4Y, h4W, h4H);
}

void Boss_Unload()
{
    for (int i = 0; i < 3; i++) {
        UnloadTextureSafe(bossTex[i]);
        UnloadTextureSafe(bossShieldTex[i]);
    }
    UnloadTextureSafe(bossBulletFrames[0]);
    UnloadTextureSafe(bossBulletFrames[1]);
    UnloadTextureSafe(bossScatterBulletFrames[0]);
    UnloadTextureSafe(bossScatterBulletFrames[1]);
    if (bossHudFontId >= 0) { AEGfxDestroyFont(bossHudFontId); bossHudFontId = -1; }
}
#include "pch.hpp"

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================
const f32 FRAME_WIDTH = 100.0f;
const f32 FRAME_HEIGHT = 130.0f;
const f32 FRAME_SPACING = DIST_BETWEEN_DOORS * 2.0f; // Distance between each frame on the wall
const f32 OFFSET = 50.0f;

const int FRAMES_PERLVL = 6;     // Total number of frames generated per floor
const int TOTAL_DESIGNS = 5;     // Total number of base poster designs available
const int MAX_FRAME_STATES = 5;  // Maximum number of glitch variations per poster design

// ============================================================================
// RENDER & STATE DATA
// ============================================================================

// Defines how many valid texture states exist for each of the 5 designs.
static int DesignStateCounts[TOTAL_DESIGNS] = { 5, 5, 5, 3, 3 };

// 2D Array holding every loaded texture: AllFrameDesigns[DesignID][GlitchState]
static AEGfxTexture* AllFrameDesigns[TOTAL_DESIGNS][MAX_FRAME_STATES]{};

// Enums mapping to the first dimension of AllFrameDesigns
enum class PosterType {
    POSTER_EVERYDAY = 0,
    POSTER_HOPE,
    POSTER_RECOVERY,
    POSTER_SIGNS,
    POSTER_COLOR
};

static AEGfxVertexList* frameMesh = nullptr;

// 2D Array storing the physical state and layout of every frame on every floor
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL]{};

// ============================================================================
// LOGIC HELPERS
// ============================================================================

// Checks the player's current illness. If the patient is a GHOST, 
// it extracts the fake illness the ghost is pretending to have to fool the player.
static ILLNESSES Frames_GetEffectiveIllness() {
    ILLNESSES current = Player_GetTrueDiagnosis();
    if (current == ILLNESSES::GHOST) {
        return Player_GetAnomalyIllness();
    }
    return current;
}

// Checks if the Central Pool or Player module has granted an extra anomaly modifier.
// This usually returns 1 if the patient is a ghost, increasing anomaly intensity, and 0 if human.
static int Frames_GetExtraAnomalyCount() {
    if (Player_GetTrueDiagnosis() == ILLNESSES::GHOST) {
        return Player_GetGhostExtraAnomalies();
    }
    return 0;
}

// ----------------------------------------------------------------------------
// ANOMALY TEXTURE MAPPING
// Dictionary lookup: Tells the game exactly which texture index a specific poster 
// should switch to based on the current illness. Returns 0 if it shouldn't react.
// ----------------------------------------------------------------------------
static int GetSpecificGlitchState(ILLNESSES illness, PosterType designID) {
    int illIdx = static_cast<int>(illness);
    int posterIdx = static_cast<int>(designID);

    // Bounds checking to prevent out-of-range array access crashes
    if (illIdx < 0 || illIdx >= 11 || posterIdx < 0 || posterIdx >= 5) return 0;

    return Config::posterGlitch[illIdx][posterIdx];
}

// ----------------------------------------------------------------------------
// ANOMALY TIMINGS
// Calculates how many seconds a frame spends glitching, or how many seconds it waits.
// ----------------------------------------------------------------------------
static float GetFrameTimerByIllness(ILLNESSES illness, bool isGlitchingPhase, int extraAnomalyCount) {
    const auto& ft = Config::frameTimesTable[static_cast<int>(illness)];

    float t;
    if (isGlitchingPhase) {
        // Grab fixed glitch duration from config
        t = ft.glitchDuration;
    }
    else {
        // Calculate random wait time between waitMin and (waitMin + waitRandMax)
        t = ft.waitMin;
        if (ft.waitRandMax > 0.0f)
            t += static_cast<float>(RandInt(0, static_cast<int>(ft.waitRandMax * 100) - 1)) / 100.0f;
    }

    // GHOST MODIFIER: Speeds up pacing by applying a multiplier if a ghost is present
    if (extraAnomalyCount >= 1) {
        t *= isGlitchingPhase ? Config::frameGhostGlitchSpeedMult : Config::frameGhostWaitSpeedMult;
        // Clamp to a minimum threshold so it doesn't break the state machine
        if (t < Config::frameGhostMinTimer) t = Config::frameGhostMinTimer;
    }
    return t;
}

// Safely resets all frames across all floors back to their perfectly normal, original physical states.
static void Frames_ResetAll() {
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];

            // Revert to normal state and default dimensions/positions
            currentFrame.currentState = 0;
            currentFrame.designID = (frame + level) % TOTAL_DESIGNS; // Stagger designs to avoid repetition
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
        }
    }
}

// ============================================================================
// SYSTEM LIFECYCLE (LOAD / INIT / UNLOAD)
// ============================================================================

void Frames_Load() {
    Frames_Unload(); // Ensure clean slate before loading

    // Dictionary of all file paths for the frame textures
    const char* framePaths[TOTAL_DESIGNS][MAX_FRAME_STATES] = {
        { Assets::Frame_Anomaly::EverydayNormal, Assets::Frame_Anomaly::EverydayPlain, Assets::Frame_Anomaly::EverydayEye, Assets::Frame_Anomaly::EverydayHaunt, Assets::Frame_Anomaly::EverydayNegative },
        { Assets::Frame_Anomaly::HopeNormal, Assets::Frame_Anomaly::HopePlain, Assets::Frame_Anomaly::HopeTear, Assets::Frame_Anomaly::HopeFear, Assets::Frame_Anomaly::HopeEye },
        { Assets::Frame_Anomaly::RecoveryNormal, Assets::Frame_Anomaly::RecoveryPlain, Assets::Frame_Anomaly::RecoveryScary, Assets::Frame_Anomaly::RecoverySinking, Assets::Frame_Anomaly::RecoveryMemLoss },
        { Assets::Frame_Anomaly::SignsNormal, Assets::Frame_Anomaly::SignsBlood, Assets::Frame_Anomaly::SignsBeware, nullptr, nullptr },
        { Assets::Frame_Anomaly::ColorNormal, Assets::Frame_Anomaly::ColorFaces, Assets::Frame_Anomaly::ColorBloody, nullptr, nullptr }
    };

    // Load textures into the 2D array, ignoring nullptrs
    for (int design = 0; design < TOTAL_DESIGNS; design++) {
        for (int state = 0; state < DesignStateCounts[design]; state++) {
            if (framePaths[design][state] != nullptr) {
                AllFrameDesigns[design][state] = LoadTextureChecked(framePaths[design][state]);
            }
        }
    }
}

void Frames_Initialize() {
    // Populate the levelMap with default frame objects
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];
            currentFrame.entity = HUMAN;
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
            currentFrame.currentState = 0;
            currentFrame.designID = (frame + level) % TOTAL_DESIGNS;
        }
    }

    // Create the square mesh used for drawing all frames if it doesn't exist
    if (frameMesh == nullptr) frameMesh = CreateSquareMesh(0xFFFFFFFF);
}

void Frames_Unload() {
    // Free mesh memory
    if (frameMesh) {
        FreeMeshSafe(frameMesh);
        frameMesh = nullptr;
    }

    // Unload all textures from the GPU
    for (int design = 0; design < TOTAL_DESIGNS; design++) {
        for (int state = 0; state < MAX_FRAME_STATES; state++) {
            if (AllFrameDesigns[design][state] != nullptr) {
                UnloadTextureSafe(AllFrameDesigns[design][state]);
                AllFrameDesigns[design][state] = nullptr;
            }
        }
    }
}

// ============================================================================
// UPDATE LOGIC (STATE MACHINE & MATH)
// ============================================================================

void Frames_Update(float dt, float camX) {
    // Abort early and reset if no patient is carried or if the Central Pool disabled frames
    if (!Player_HasPatient() || !CentralPool_IsSystemActive(AnomalyFiles::Frames_isActive)) {
        Frames_ResetAll();
        return;
    }

    const ILLNESSES effIllness = Frames_GetEffectiveIllness();
    const int extraAnoms = Frames_GetExtraAnomalyCount();

    // Static variables persist across frames to maintain the state machine
    static int timerState = STATE_NORMAL;
    static float timer = 2.0f;
    static float animTime = 0.0f;

    // Boolean map determining if a frame will jitter (true) or stay statically in place (false) during a glitch
    static bool frameJitterMap[NUM_OF_FLOOR][FRAMES_PERLVL]{};

    // Tracks total elapsed time for smooth sin/cos wave calculations (e.g., AIW Syndrome breathing)
    static float totalElapsedTime = 0.0f;
    totalElapsedTime += dt;

    // -------------------------------------------------------------
    // 1. STATE MACHINE TIMER LOGIC
    // -------------------------------------------------------------
    timer -= dt;

    // Track how long we've been in the glitching state specifically for animation math
    if (timerState == STATE_GLITCHING) animTime += dt;
    else animTime = 0.0f;

    switch (timerState) {
    case STATE_NORMAL:
        // Normal waiting phase is over. Transition to glitching phase.
        if (timer <= 0.0f) {
            timerState = STATE_GLITCHING;
            timer = GetFrameTimerByIllness(effIllness, true, extraAnoms);

            // Initialize the glitch states and roll the physical behaviors for this specific glitch phase
            for (int level = 0; level < NUM_OF_FLOOR; ++level) {
                for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
                    FrameAnomaly& f = levelMap[level][frame];

                    // Assign texture based on Illness
                    f.currentState = GetSpecificGlitchState(effIllness, static_cast<PosterType>(f.designID));

                    // 50% chance for Event 1 (Static) or Event 2 (Jitter/Physical Movement)
                    frameJitterMap[level][frame] = (RandInt(0, 1) == 0);
                }
            }
        }
        break;

    case STATE_GLITCHING:
        // Glitch phase is over. Transition to a brief 0.1s cooldown phase to snap things back.
        if (timer <= 0.0f) {
            timerState = STATE_COOLDOWN;
            timer = 0.1f;
        }
        break;

    case STATE_COOLDOWN:
        // Reset everything to normal, EXCEPT for OCD and AIW, which have continuous physical offsets.
        if (effIllness != ILLNESSES::AIW_SYNDROME && effIllness != ILLNESSES::OCD) {
            Frames_ResetAll();
        }

        // Cooldown over. Transition back to normal waiting phase.
        if (timer <= 0.0f) {
            timerState = STATE_NORMAL;
            timer = GetFrameTimerByIllness(effIllness, false, extraAnoms);
        }
        break;
    }

    // -------------------------------------------------------------
    // 2. CONTINUOUS PHYSICAL EFFECTS (APPLIED EVERY FRAME)
    // -------------------------------------------------------------
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& f = levelMap[level][frame];

            // Calculate base world coordinates (Fixed: Cast to float for precision)
            float baseX = (float)frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2.0f);
            float baseY = 0.0f;

            // Absolute distance from the camera center (used for proximity effects)
            float distToPlayer = fabsf(baseX + camX);

            // --- SPECIFIC ILLNESS BEHAVIORS ---

            if (effIllness == ILLNESSES::PARANOIA) {
                // PARANOIA (Weeping Angel): If the player is too close, the frame snaps to normal, gaslighting them.
                if (distToPlayer < Config::frameParanoiaCloseRange) {
                    f.currentState = 0; // Hide anomaly texture
                    f.posX = baseX;
                    f.posY = baseY;
                }
                else if (f.currentState > 0) {
                    // Calculate panic from 0.0 (calm/close) to 1.0 (freaking out/far away)
                    float panicSpeed = (distToPlayer - Config::frameParanoiaCloseRange) / Config::frameParanoiaPanicRange;
                    panicSpeed = AEClamp(panicSpeed, 0.05f, 1.0f);

                    if (frameJitterMap[level][frame]) {
                        // Probability to pick a NEW random position scales with distance.
                        // If it fails the roll, it keeps its previous offset, creating a sluggish stutter as you approach!
                        if (RandInt(0, 99) < (int)(panicSpeed * 100.0f)) {
                            int intensity = (int)(Config::frameParanoiaIntensityScale * panicSpeed);
                            if (intensity < Config::frameParanoiaMinIntensity) intensity = Config::frameParanoiaMinIntensity;

                            f.posX = baseX + (float)(RandInt(0, intensity - 1) - (intensity / 2));
                            f.posY = baseY + (float)(RandInt(0, intensity - 1) - (intensity / 2));
                        }
                    }
                    else {
                        // Static Event (Event 1): Just sit perfectly still
                        f.posX = baseX;
                        f.posY = baseY;
                    }
                }
            }
            else if (effIllness == ILLNESSES::MANIA) {
                // MANIA (Hyperactive Drift): Slowly drifts in one of 4 directions
                if (f.currentState > 0) {
                    int dir = (level + frame) % 4; // Assign a drift direction based on frame ID
                    float drift = animTime * Config::frameManiaDriftSpeed;

                    f.posX = baseX + ((dir == 0) ? -drift : (dir == 1) ? drift : 0.0f);
                    f.posY = baseY + ((dir == 2) ? drift : (dir == 3) ? -drift : 0.0f);

                    // Add brief rapid stutters on top of the smooth drift to simulate hyperactivity
                    if (RandInt(0, 99) < Config::frameManiaSutterChance) {
                        f.posX += (float)(RandInt(0, Config::frameManiaSutterRange - 1) - (Config::frameManiaSutterRange / 2));
                        f.posY += (float)(RandInt(0, Config::frameManiaSutterRange - 1) - (Config::frameManiaSutterRange / 2));
                    }
                }
            }
            else if (effIllness == ILLNESSES::DEPRESSION) {
                // DEPRESSION (Melting/Sinking): Frame slowly stretches downwards over time.
                if (f.currentState > 0) {
                    // Cap stretch at 1.0 to prevent infinite stretching
                    float stretch = AEClamp(animTime / Config::frameDepressionStretchDuration, 0.0f, 1.0f);

                    f.height = FRAME_HEIGHT + (stretch * Config::frameDepressionMaxHeightIncrease);
                    f.posY = baseY - (stretch * Config::frameDepressionYOffset); // Move down as it stretches
                    f.posX = baseX;
                }
            }
            else if (effIllness == ILLNESSES::DEMENTIA) {
                // DEMENTIA (Persistent Confusion): Constant moderate jittering
                if (f.currentState > 0) {
                    f.posX = baseX + (float)(RandInt(0, Config::frameDementiaJitterRange - 1) - (Config::frameDementiaJitterRange / 2));
                    f.posY = baseY + (float)(RandInt(0, Config::frameDementiaJitterRange - 1) - (Config::frameDementiaJitterRange / 2));
                }
            }
            else if (effIllness == ILLNESSES::SCHIZOPHRENIA) {
                // SCHIZOPHRENIA (Aggressive Hallucination): Violent, high-range jittering
                if (f.currentState > 0) {
                    f.posX = baseX + (float)(RandInt(0, Config::frameSchizophreniJitterRange - 1) - (Config::frameSchizophreniJitterRange / 2));
                    f.posY = baseY + (float)(RandInt(0, Config::frameSchizophreniJitterRange - 1) - (Config::frameSchizophreniJitterRange / 2));
                }
            }
            else if (effIllness == ILLNESSES::AIW_SYNDROME) {
                // ALICE IN WONDERLAND (Spatial Distortion): Uses Sine/Cosine waves for a continuous floating/breathing effect
                if (timerState == STATE_NORMAL) {
                    f.currentState = 0; // Look normal, but float

                    // Offset the wave using 'frame' index so they don't all breathe perfectly in sync
                    f.posX = baseX + (sinf(totalElapsedTime * Config::frameAIWOscillationFreq + (float)frame) * Config::frameAIWOscillationAmp);
                    f.posY = baseY + (cosf(totalElapsedTime * Config::frameAIWOscillationFreq + (float)frame) * Config::frameAIWOscillationAmp);
                }
                else {
                    // When glitching, snap perfectly into place and change texture
                    f.posX = baseX;
                    f.posY = baseY;
                    f.currentState = GetSpecificGlitchState(effIllness, static_cast<PosterType>(f.designID));
                }
            }
            else if (effIllness == ILLNESSES::INSOMNIA) {
                // INSOMNIA (Exhaustion Distortion): Follows the player with its "eyes" by flipping its width
                f.width = (baseX + camX > 0.0f) ? -FRAME_WIDTH : FRAME_WIDTH;

                if (distToPlayer > Config::frameInsomniaFarDist) {
                    // If far away, stretch wildly either vertically or horizontally based on the jitter map
                    if (frameJitterMap[level][frame]) f.height = FRAME_HEIGHT * Config::frameInsomniaStretchMult;
                    else                              f.width *= Config::frameInsomniaStretchMult;
                }
                else {
                    // Close proximity snaps it out of the hallucination
                    f.height = FRAME_HEIGHT;
                    f.width = (baseX + camX > 0.0f) ? -FRAME_WIDTH : FRAME_WIDTH;
                }
            }
            else if (effIllness == ILLNESSES::OCD) {
                // OCD (Imperfection/Misalignment): Frame becomes persistently misaligned/off-center
                if (timerState == STATE_GLITCHING) {
                    f.currentState = GetSpecificGlitchState(effIllness, static_cast<PosterType>(f.designID));

                    // Offset is determined by whether the frame/level ID is even or odd, creating a checkerboard offset
                    float annoyingOffsetX = (frame % 2 == 0) ? Config::frameOCDOffsetXEven : Config::frameOCDOffsetXOdd;
                    float annoyingOffsetY = (level % 2 == 0) ? Config::frameOCDOffsetYEven : Config::frameOCDOffsetYOdd;

                    f.posX = baseX + annoyingOffsetX;
                    f.posY = baseY + annoyingOffsetY;
                }
                else {
                    // Snap back to perfect alignment
                    f.currentState = 0;
                    f.posX = baseX;
                    f.posY = baseY;
                }
            }
            else if (effIllness == ILLNESSES::SCOTOPHOBIA) {
                // SCOTOPHOBIA (Peripheral Gaze): Only glitches when on the edges of the screen (far from camera center)
                if (distToPlayer < Config::frameScotophobiaCloseRange) {
                    // Looked at directly: act perfectly normal
                    f.currentState = 0;
                    f.posX = baseX;
                    f.posY = baseY;
                }
                else if (f.currentState > 0) {
                    // In peripheral vision: jitter
                    f.posX = baseX + (float)(RandInt(0, Config::frameScotophobiaJitterRange - 1) - (Config::frameScotophobiaJitterRange / 2));
                    f.posY = baseY + (float)(RandInt(0, Config::frameScotophobiaJitterRange - 1) - (Config::frameScotophobiaJitterRange / 2));
                }
            }
            else {
                // Safety Fallback for unhandled states to prevent frames from disappearing
                f.posX = baseX;
                f.posY = baseY;
                f.width = FRAME_WIDTH;
                f.height = FRAME_HEIGHT;
            }

            // --- GHOST OVERRIDE ---
            // If the patient is a ghost, add a chance for a violent micro-stutter on top of ALL other logic.
            if (f.currentState > 0 && extraAnoms >= 1 && RandInt(0, 99) < Config::frameGhostStutterChance) {
                f.posX += (float)(RandInt(0, Config::frameGhostStutterRange - 1) - (Config::frameGhostStutterRange / 2));
                f.posY += (float)(RandInt(0, Config::frameGhostStutterRange - 1) - (Config::frameGhostStutterRange / 2));
            }
        }
    }
}

// ============================================================================
// DRAW
// ============================================================================

void Frames_Draw(int currentLevel, f32 camX) {
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;
    if (frameMesh == nullptr) return;

    ILLNESSES effIllness = Frames_GetEffectiveIllness();

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];
        AEGfxTexture* Tex = AllFrameDesigns[currentFrame.designID][currentFrame.currentState];

        if (Tex) {
            const f32 drawX = currentFrame.posX + camX; // Apply camera offset

            // Draw the frame
            DrawTextureMesh(frameMesh, Tex, drawX, currentFrame.posY, currentFrame.width, currentFrame.height, 1.0f);

            float halfW = currentFrame.width * 0.5f;

            // --- VIEWPORT CULLING & JOURNAL REPORTING ---
            // Check if the frame is currently glitching AND is physically visible on the screen.
            // If it is, report the anomaly to the Journal system.
            if (currentFrame.currentState > 0 &&
                drawX + halfW > -SCREEN_WIDTH_HALF &&
                drawX - halfW < SCREEN_WIDTH_HALF)
            {
                switch (effIllness)
                {
                case ILLNESSES::PARANOIA:      Journal_ReportAnomaly(ANOMALYID::Frame_Shakes);      break;
                case ILLNESSES::MANIA:         Journal_ReportAnomaly(ANOMALYID::Frame_Mania);       break;
                case ILLNESSES::DEPRESSION:    Journal_ReportAnomaly(ANOMALYID::Frame_Depression);  break;
                case ILLNESSES::DEMENTIA:      Journal_ReportAnomaly(ANOMALYID::Frame_Shakes);      break;
                case ILLNESSES::SCHIZOPHRENIA: Journal_ReportAnomaly(ANOMALYID::Frame_Shakes);      break;
                case ILLNESSES::AIW_SYNDROME:  Journal_ReportAnomaly(ANOMALYID::Frame_AIW);         break;
                case ILLNESSES::INSOMNIA:      Journal_ReportAnomaly(ANOMALYID::Frame_Insomnia);    break;
                case ILLNESSES::OCD:           Journal_ReportAnomaly(ANOMALYID::Frame_OCD);         break;
                case ILLNESSES::SCOTOPHOBIA:   Journal_ReportAnomaly(ANOMALYID::Frame_Shakes);      break;
                default: break;
                }
            }
        }
    }
}
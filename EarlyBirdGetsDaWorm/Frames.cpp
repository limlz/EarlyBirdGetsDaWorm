#include "pch.hpp"

// --- Config ---
const f32 FRAME_WIDTH = 100.0f;
const f32 FRAME_HEIGHT = 130.0f;
const f32 FRAME_SPACING = DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET = 50.0f;

const int FRAMES_PERLVL = 6;
const int TOTAL_DESIGNS = 5;
const int MAX_FRAME_STATES = 5;

// --- Render & State Data ---
static int DesignStateCounts[TOTAL_DESIGNS] = { 5, 5, 5, 3, 3 };
static AEGfxTexture* AllFrameDesigns[TOTAL_DESIGNS][MAX_FRAME_STATES]{};

enum PosterType {
    POSTER_EVERYDAY = 0,
    POSTER_HOPE,
    POSTER_RECOVERY,
    POSTER_SIGNS,
    POSTER_COLOR
};

static AEGfxVertexList* frameMesh = nullptr;
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL]{};

/******************************* FRAMES LOGIC **********************************/

// Checks the player's current illness. If the patient is a GHOST, 
// it extracts the fake illness the ghost is pretending to have.
static ILLNESSES Frames_GetEffectiveIllness() {
    ILLNESSES current = Player_GetTrueDiagnosis();
    if (current == ILLNESSES::GHOST) {
        return Player_GetAnomalyIllness();
    }
    return current;
}

// Checks if the Central Pool or Player module has granted an extra anomaly modifier.
// This usually returns 1 if the patient is a ghost, and 0 if human.
static int Frames_GetExtraAnomalyCount() {
    if (Player_GetTrueDiagnosis() == ILLNESSES::GHOST) {
        return Player_GetGhostExtraAnomalies();
    }
    return 0;
}

/******************************************** ANOMALY TEXTURE MAPPING   *******************************************/
// Dictionary lookup: Tells the game exactly which texture index a specific poster 
// should switch to based on the current illness. Returns 0 if it shouldn't react.
// The integers correspond to the indices in the `framePaths` array in Frames_Load().
/******************************************** ANOMALY TEXTURE MAPPING   *******************************************/
static int GetSpecificGlitchState(ILLNESSES illness, int designID) {
    switch (illness) {
    case ILLNESSES::PARANOIA:
        if (designID == POSTER_EVERYDAY) return 2; // EverydayEye: Subtle eyes watching the player
        if (designID == POSTER_HOPE)     return 4; // HopeEye: Large intrusive eyeball
        if (designID == POSTER_SIGNS)    return 1; // SignsBlood: Bloody warning messages
        break;
    case ILLNESSES::MANIA:
		if (designID == POSTER_EVERYDAY) return 3; // EverydayHaunt: Ghost Silhouette
        if (designID == POSTER_COLOR)    return 2; // ColorBloody: Waves shaped to look like screaming bloody faces
		if (designID == POSTER_RECOVERY) return 2; // RecoveryScary: Scribbled over with frantic, aggressive lines
        break;
    case ILLNESSES::DEPRESSION:
		if (designID == POSTER_EVERYDAY) return 4; // EverydayNegative: Hopelessness quotes
		if (designID == POSTER_HOPE)     return 2; // HopeTear: Tear in the poster
        if (designID == POSTER_RECOVERY) return 3; // RecoverySinking: Visual representation of drowning/sinking
        break;
    case ILLNESSES::DEMENTIA:
        if (designID == POSTER_HOPE)     return 1; // HopePlain: Details erased
        if (designID == POSTER_EVERYDAY) return 1; // EverydayPlain: Details erased
        if (designID == POSTER_RECOVERY) return 4; // RecoveryMemLoss: Severely degraded image
        if (designID == POSTER_COLOR)    return 1; // ColorFaces: Confusing/jumbled faces
        break;
    case ILLNESSES::SCHIZOPHRENIA:
        if (designID == POSTER_SIGNS)    return 2; // SignsBeware: Threatening hallucinations
        if (designID == POSTER_EVERYDAY) return 4; // EverydayNegative: Hopelessness quotes
        if (designID == POSTER_HOPE)     return 4; // HopeEye: Large intrusive eyeball
        break;
    case ILLNESSES::AIW_SYNDROME:
        if (designID == POSTER_RECOVERY) return 3; // RecoverySinking: Visual representation of drowning/sinking
        if (designID == POSTER_HOPE)     return 2; // HopeTear: Tear in the poster
        break;
    case ILLNESSES::INSOMNIA:
        if (designID == POSTER_EVERYDAY) return 1; // EverydayPlain: Details erased
		if (designID == POSTER_HOPE)     return 3; // HopeFear: Scribbled over with frantic, aggressive lines
        if (designID == POSTER_COLOR)    return 1; // ColorFaces: Confusing/jumbled faces
        if (designID == POSTER_SIGNS)    return 1; // SignsBlood: Bloody warning messages
        break;
    case ILLNESSES::OCD:
        if (designID == POSTER_RECOVERY) return 1; // RecoveryPlain: Details erased
        if (designID == POSTER_HOPE)     return 1; // HopePlain: Details erased
        if (designID == POSTER_EVERYDAY) return 3; // EverydayHaunt: Ghost Silhouette
        break;
    case ILLNESSES::SCOTOPHOBIA:
        if (designID == POSTER_SIGNS)    return 2; // SignsBeware: Threatening hallucinations
        if (designID == POSTER_HOPE)     return 3; // HopeFear: Scribbled over with frantic, aggressive lines
        if (designID == POSTER_COLOR)    return 2; // ColorBloody: Waves shaped to look like screaming bloody faces
        break;
    }
    return 0; // Return 0 (Normal State) if this poster design doesn't react to this illness
}

/******************************************** ANOMALY TIMINGS   *******************************************/
// Calculates how many seconds a frame spends glitching, or how many seconds it waits before glitching again.
// MANIA/SCHIZO: Quick bursts (2s), frequent waits (2s-7s).
// DEPRESSION/DEMENTIA: Long bursts (4s), long waits (6s-10s).
// AIW/INSOMNIA/OCD: Fixed, persistent rhythms.
/******************************************** ANOMALY TIMINGS   *******************************************/
static float GetFrameTimerByIllness(ILLNESSES illness, bool isGlitchingPhase, int extraAnomalyCount) {
    const auto& ft = Config::frameTimesTable[static_cast<int>(illness)];

    float t;
    if (isGlitchingPhase) {
        t = ft.glitchDuration;
    } else {
        t = ft.waitMin;
        if (ft.waitRandMax > 0.0f)
            t += (float)(rand() % (int)(ft.waitRandMax * 100)) / 100.0f;
    }

    // GHOST MODIFIER: speeds up pacing to make the environment feel frantic
    if (extraAnomalyCount >= 1) {
        t *= isGlitchingPhase ? Config::frameGhostGlitchSpeedMult : Config::frameGhostWaitSpeedMult;
        if (t < Config::frameGhostMinTimer) t = Config::frameGhostMinTimer;
    }
    return t;
}

// Safely resets all frames across all floors back to their perfectly normal, original states
static void Frames_ResetAll() {
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];
            currentFrame.currentState = 0;

            currentFrame.designID = (frame + level) % TOTAL_DESIGNS;
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
        }
    }
}

/***************************************** LOAD ******************************************/
void Frames_Load() {
    Frames_Unload();

    const char* framePaths[TOTAL_DESIGNS][MAX_FRAME_STATES] = {
        { Assets::Frame_Anomaly::EverydayNormal, Assets::Frame_Anomaly::EverydayPlain, Assets::Frame_Anomaly::EverydayEye, Assets::Frame_Anomaly::EverydayHaunt, Assets::Frame_Anomaly::EverydayNegative },
        { Assets::Frame_Anomaly::HopeNormal, Assets::Frame_Anomaly::HopePlain, Assets::Frame_Anomaly::HopeTear, Assets::Frame_Anomaly::HopeFear, Assets::Frame_Anomaly::HopeEye },
        { Assets::Frame_Anomaly::RecoveryNormal, Assets::Frame_Anomaly::RecoveryPlain, Assets::Frame_Anomaly::RecoveryScary, Assets::Frame_Anomaly::RecoverySinking, Assets::Frame_Anomaly::RecoveryMemLoss },
        { Assets::Frame_Anomaly::SignsNormal, Assets::Frame_Anomaly::SignsBlood, Assets::Frame_Anomaly::SignsBeware, nullptr, nullptr },
        { Assets::Frame_Anomaly::ColorNormal, Assets::Frame_Anomaly::ColorFaces, Assets::Frame_Anomaly::ColorBloody, nullptr, nullptr }
    };

    for (int design = 0; design < TOTAL_DESIGNS; design++) {
        for (int state = 0; state < DesignStateCounts[design]; state++) {
            if (framePaths[design][state] != nullptr) {
                AllFrameDesigns[design][state] = LoadTextureChecked(framePaths[design][state]);
            }
        }
    }
}

/*************************************** INITIALIZE **************************************/
void Frames_Initialize() {
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
    if (frameMesh == nullptr) frameMesh = CreateSquareMesh(0xFFFFFFFF);
}

/******************************************** ANOMALY DESCRIPTIONS   *******************************************/
// 1. PARANOIA:     Glitching becomes slower as the player approaches; snaps to normal if too close (Weeping Angel).
// 2. MANIA:        Brief, hyperactive glitches where the frame slowly drifts in one direction.
// 3. DEPRESSION:   Frame slowly stretches downwards over time, like it's melting or heavy.
// 4. DEMENTIA:     Frame locks into a scary state and jitters continuously for several seconds.
// 5. SCHIZOPHRENIA:Violent, aggressive jittering for a brief period.
// 6. AIW_SYNDROME: Frame gently breathes (scales) and offsets in a wavy pattern when "normal".
// 7. INSOMNIA:     Frame stretches drastically when far away, flips to face player, acts normal when close.
// 8. OCD:          Frame becomes persistently misaligned/off-center, swapping states every few seconds.
// 9. SCOTOPHOBIA:  Frame glitches in peripheral vision but snaps to normal when looked at directly.
// 10. GHOST:       Frames have a constant 15% chance to micro-stutter regardless of current behavior.
/******************************************** ANOMALY DESCRIPTIONS   *******************************************/

/***************************************** UPDATE ****************************************/
void Frames_Update(float dt, float camX) {
    if (!Player_HasPatient() || !CentralPool_IsSystemActive(AnomalyFiles::Frames_isActive)) {
        Frames_ResetAll();
        return;
    }

    const ILLNESSES effIllness = Frames_GetEffectiveIllness();
    const int extraAnoms = Frames_GetExtraAnomalyCount();

    // Report the anomaly to the journal
    switch (effIllness)
    {
    case ILLNESSES::PARANOIA:      Journal_ReportAnomaly(ANOMALYID::Frame_Glitch);  break;
    case ILLNESSES::MANIA:         Journal_ReportAnomaly(ANOMALYID::Frame_Shift);   break;
    case ILLNESSES::DEPRESSION:    Journal_ReportAnomaly(ANOMALYID::Frame_Distort); break;
    case ILLNESSES::DEMENTIA:      Journal_ReportAnomaly(ANOMALYID::Frame_Glitch);  break;
    case ILLNESSES::SCHIZOPHRENIA: Journal_ReportAnomaly(ANOMALYID::Frame_Glitch);  break;
    case ILLNESSES::AIW_SYNDROME:  Journal_ReportAnomaly(ANOMALYID::Frame_Distort); break;
    case ILLNESSES::INSOMNIA:      Journal_ReportAnomaly(ANOMALYID::Frame_Distort); break;
    case ILLNESSES::OCD:           Journal_ReportAnomaly(ANOMALYID::Frame_Shift);   break;
    case ILLNESSES::SCOTOPHOBIA:   Journal_ReportAnomaly(ANOMALYID::Frame_Glitch);  break;
    default: break;
    }

    static int timerState = STATE_NORMAL;
    static float timer = 2.0f;
    static float animTime = 0.0f;

    // Boolean map determining if a frame will jitter (true) or stay statically in place (false) during a glitch
    static bool frameJitterMap[NUM_OF_FLOOR][FRAMES_PERLVL]{};

    // Tracks total elapsed time for smooth sin/cos wave calculations (e.g., AIW Syndrome breathing)
    static float totalElapsedTime = 0.0f;
    totalElapsedTime += dt;

    // --- 1. TIMER LOGIC ---
    timer -= dt;
    if (timerState == STATE_GLITCHING) animTime += dt;
    else animTime = 0.0f;

    switch (timerState) {
    case STATE_NORMAL:
        // Normal phase is over. Transition to glitching phase.
        if (timer <= 0.0f) {
            timerState = STATE_GLITCHING;
            timer = GetFrameTimerByIllness(effIllness, true, extraAnoms);

            // Initialize the glitch states and roll the physical behaviors for this phase
            for (int level = 0; level < NUM_OF_FLOOR; ++level) {
                for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
                    FrameAnomaly& f = levelMap[level][frame];
                    f.currentState = GetSpecificGlitchState(effIllness, f.designID);

                    // 50% chance for Event 1 (Static) or Event 2 (Jitter/Physical Movement)
                    frameJitterMap[level][frame] = (rand() % 2 == 0);
                }
            }
        }
        break;

    case STATE_GLITCHING:
        // Glitch phase is over. Transition to the brief 0.1s cooldown phase to snap things back.
        if (timer <= 0.0f) {
            timerState = STATE_COOLDOWN;
            timer = 0.1f;
        }
        break;

    case STATE_COOLDOWN:
        // DO NOT reset everything for OCD or AIW, let their physical offset persist continuously
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

    // --- 2. CONTINUOUS PHYSICAL EFFECTS ---
    // This loop applies continuous math (vibration, stretching, drifting) every single frame
    // based on the specific illness currently active.
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& f = levelMap[level][frame];

            float baseX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            float baseY = 0.0f;
            float distToPlayer = fabs(baseX + camX); // Absolute distance from screen center

            // -------------------------------------------------------------
            // SPECIFIC ILLNESS BEHAVIORS
            // -------------------------------------------------------------

            if (effIllness == ILLNESSES::PARANOIA) {
                // --- PARANOIA: THE WEEPING ANGEL ---
                // If the player is too close, the frame acts completely normal, gaslighting them.
                if (distToPlayer < Config::frameParanoiaCloseRange) {
                    f.currentState = 0; // Hide anomaly!
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
                        if ((rand() % 100) < (panicSpeed * 100)) {
                            int intensity = (int)(Config::frameParanoiaIntensityScale * panicSpeed);
                            if (intensity < Config::frameParanoiaMinIntensity) intensity = Config::frameParanoiaMinIntensity;

                            f.posX = baseX + ((rand() % intensity) - (intensity / 2));
                            f.posY = baseY + ((rand() % intensity) - (intensity / 2));
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
                // --- MANIA: HYPERACTIVE DRIFT ---
                if (f.currentState > 0) {
                    int dir = (level + frame) % 4;
                    float drift = animTime * Config::frameManiaDriftSpeed;

                    f.posX = baseX + ((dir == 0) ? -drift : (dir == 1) ? drift : 0.0f);
                    f.posY = baseY + ((dir == 2) ? drift : (dir == 3) ? -drift : 0.0f);

                    // Add brief rapid stutters on top of the smooth drift to simulate hyperactivity
                    if ((rand() % 100) < Config::frameManiaSutterChance) {
                        f.posX += ((rand() % Config::frameManiaSutterRange) - (Config::frameManiaSutterRange / 2));
                        f.posY += ((rand() % Config::frameManiaSutterRange) - (Config::frameManiaSutterRange / 2));
                    }
                }
            }
            else if (effIllness == ILLNESSES::DEPRESSION) {
                // --- DEPRESSION: MELTING/SINKING ---
                if (f.currentState > 0) {
                    float stretch = AEClamp(animTime / Config::frameDepressionStretchDuration, 0.0f, 1.0f);
                    f.height = FRAME_HEIGHT + (stretch * Config::frameDepressionMaxHeightIncrease);
                    f.posY   = baseY - (stretch * Config::frameDepressionYOffset);
                    f.posX   = baseX;
                }
            }
            else if (effIllness == ILLNESSES::DEMENTIA) {
                // --- DEMENTIA: PERSISTENT CONFUSION ---
                if (f.currentState > 0) {
                    f.posX = baseX + ((rand() % Config::frameDementiaJitterRange) - (Config::frameDementiaJitterRange / 2));
                    f.posY = baseY + ((rand() % Config::frameDementiaJitterRange) - (Config::frameDementiaJitterRange / 2));
                }
            }
            else if (effIllness == ILLNESSES::SCHIZOPHRENIA) {
                // --- SCHIZOPHRENIA: AGGRESSIVE HALLUCINATION ---
                if (f.currentState > 0) {
                    f.posX = baseX + ((rand() % Config::frameSchizophreniJitterRange) - (Config::frameSchizophreniJitterRange / 2));
                    f.posY = baseY + ((rand() % Config::frameSchizophreniJitterRange) - (Config::frameSchizophreniJitterRange / 2));
                }
            }
            else if (effIllness == ILLNESSES::AIW_SYNDROME) {
                // --- ALICE IN WONDERLAND: SPATIAL DISTORTION ---
                // Frame gently breathes and floats off-center when normal, snaps perfectly in place when glitching
                if (timerState == STATE_NORMAL) {
                    f.currentState = 0;
                    f.posX = baseX + (sinf(totalElapsedTime * Config::frameAIWOscillationFreq + frame) * Config::frameAIWOscillationAmp);
                    f.posY = baseY + (cosf(totalElapsedTime * Config::frameAIWOscillationFreq + frame) * Config::frameAIWOscillationAmp);
                }
                else {
                    f.posX = baseX;
                    f.posY = baseY;
                    f.currentState = GetSpecificGlitchState(effIllness, f.designID);
                }
            }
            else if (effIllness == ILLNESSES::INSOMNIA) {
                // --- INSOMNIA: EXHAUSTION DISTORTION ---
                // The frame's width flips negative based on player position so it "faces" the player
                f.width = (baseX + camX > 0) ? -FRAME_WIDTH : FRAME_WIDTH;

                if (distToPlayer > Config::frameInsomniaFarDist) {
                    // If far away, stretch wildly either vertically or horizontally
                    if (frameJitterMap[level][frame]) f.height  = FRAME_HEIGHT * Config::frameInsomniaStretchMult;
                    else                              f.width  *= Config::frameInsomniaStretchMult;
                }
                else {
                    // Close proximity snaps it out of the hallucination
                    f.height = FRAME_HEIGHT;
                    f.width  = (baseX + camX > 0) ? -FRAME_WIDTH : FRAME_WIDTH;
                }
            }
            else if (effIllness == ILLNESSES::OCD) {
                // --- OCD: IMPERFECTION/MISALIGNMENT ---
                if (timerState == STATE_GLITCHING) {
                    f.currentState = GetSpecificGlitchState(effIllness, f.designID);

                    float annoyingOffsetX = (frame % 2 == 0) ? Config::frameOCDOffsetXEven : Config::frameOCDOffsetXOdd;
                    float annoyingOffsetY = (level % 2 == 0) ? Config::frameOCDOffsetYEven : Config::frameOCDOffsetYOdd;
                    f.posX = baseX + annoyingOffsetX;
                    f.posY = baseY + annoyingOffsetY;
                }
                else {
                    f.currentState = 0;
                    f.posX = baseX;
                    f.posY = baseY;
                }
            }
            else if (effIllness == ILLNESSES::SCOTOPHOBIA) {
                // --- SCOTOPHOBIA: PERIPHERAL GAZE MECHANIC ---
                if (distToPlayer < Config::frameScotophobiaCloseRange) {
                    f.currentState = 0;
                    f.posX = baseX;
                    f.posY = baseY;
                }
                else if (f.currentState > 0) {
                    f.posX = baseX + ((rand() % Config::frameScotophobiaJitterRange) - (Config::frameScotophobiaJitterRange / 2));
                    f.posY = baseY + ((rand() % Config::frameScotophobiaJitterRange) - (Config::frameScotophobiaJitterRange / 2));
                }
            }
            else {
                // Safety Fallback for unhandled states
                f.posX = baseX;
                f.posY = baseY;
                f.width = FRAME_WIDTH;
                f.height = FRAME_HEIGHT;
            }

            // --- GHOST OVERRIDE ---
            // If the patient is a ghost, add a chance for a violent micro-stutter on top of all other logic.
            if (f.currentState > 0 && extraAnoms >= 1 && (rand() % 100) < Config::frameGhostStutterChance) {
                f.posX += (rand() % Config::frameGhostStutterRange) - (Config::frameGhostStutterRange / 2);
                f.posY += (rand() % Config::frameGhostStutterRange) - (Config::frameGhostStutterRange / 2);
            }
        }
    }
}

/***************************************** DRAW ******************************************/
void Frames_Draw(int currentLevel, f32 camX) {
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;
    if (frameMesh == nullptr) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];

        AEGfxTexture* Tex = AllFrameDesigns[currentFrame.designID][currentFrame.currentState];

        if (Tex) {
            const f32 drawX = currentFrame.posX + camX;
            DrawTextureMesh(frameMesh, Tex, drawX, currentFrame.posY, currentFrame.width, currentFrame.height, 1.0f);
        }
    }
}

/**************************************** UNLOAD *****************************************/
void Frames_Unload() {
    if (frameMesh) {
        FreeMeshSafe(frameMesh);
        frameMesh = nullptr;
    }

    for (int design = 0; design < TOTAL_DESIGNS; design++) {
        for (int state = 0; state < MAX_FRAME_STATES; state++) {
            if (AllFrameDesigns[design][state] != nullptr) {
                UnloadTextureSafe(AllFrameDesigns[design][state]);
                AllFrameDesigns[design][state] = nullptr;
            }
        }
    }
}
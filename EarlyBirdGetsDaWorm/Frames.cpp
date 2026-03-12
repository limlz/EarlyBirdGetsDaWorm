#include "pch.hpp"

const f32 FRAME_WIDTH = 100.0f;
const f32 FRAME_HEIGHT = 130.0f;
const f32 FRAME_SPACING = DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET = 50.0f;

const int FRAMES_PERLVL = 6;
const int TOTAL_DESIGNS = 5;
const int MAX_FRAME_STATES = 5;

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

// Determines which illness the frames should react to.
// If the patient is a GHOST, it asks the Player module for the illness it is pretending to have.
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

// Tells the game exactly which state a poster should switch to.
// Returns 0 if the poster should NOT glitch for this illness.
static int GetSpecificGlitchState(ILLNESSES illness, int designID) {
    switch (illness) {

    case ILLNESSES::PARANOIA:
        if (designID == POSTER_EVERYDAY) return 2; // EverydayEye
        if (designID == POSTER_HOPE)     return 4; // HopeEye
        if (designID == POSTER_SIGNS)    return 1; // SignsBlood
        break;

    case ILLNESSES::MANIA:
        if (designID == POSTER_EVERYDAY) return 3; // EverydayHaunt
        if (designID == POSTER_COLOR)    return 2; // ColorBloody
        if (designID == POSTER_RECOVERY) return 2; // RecoveryScary
        break;

    case ILLNESSES::DEPRESSION:
        if (designID == POSTER_EVERYDAY) return 4; // EverydayNegative
        if (designID == POSTER_HOPE)     return 2; // HopeTear
        if (designID == POSTER_RECOVERY) return 3; // RecoverySinking
        break;

    case ILLNESSES::DEMENTIA:
        if (designID == POSTER_HOPE)     return 1; // HopePlain
        if (designID == POSTER_EVERYDAY) return 1; // EverydayPlain
        if (designID == POSTER_RECOVERY) return 4; // RecoveryMemLoss
        if (designID == POSTER_COLOR)    return 1; // ColorFaces
        break;

    case ILLNESSES::SCHIZOPHRENIA:
        if (designID == POSTER_SIGNS)    return 2; // SignsBeware
        if (designID == POSTER_EVERYDAY) return 4; // EverydayNegative
        if (designID == POSTER_HOPE)     return 4; // HopeEye
        break;

    case ILLNESSES::AIW_SYNDROME:
        if (designID == POSTER_RECOVERY) return 3; // RecoverySinking
        if (designID == POSTER_HOPE)     return 2; // HopeTear
        break;

    case ILLNESSES::INSOMNIA:
        if (designID == POSTER_EVERYDAY) return 1; // EverydayPlain
        if (designID == POSTER_HOPE)     return 3; // HopeFear
        if (designID == POSTER_COLOR)    return 1; // ColorFaces
        if (designID == POSTER_SIGNS)    return 1; // SignsBlood
        break;

    case ILLNESSES::OCD:
        if (designID == POSTER_RECOVERY) return 1; // RecoveryPlain
        if (designID == POSTER_HOPE)     return 1; // HopePlain
        if (designID == POSTER_EVERYDAY) return 3; // EverydayHaunt

        break;

    case ILLNESSES::SCOTOPHOBIA:
        if (designID == POSTER_SIGNS)    return 2; // SignsBeware
        if (designID == POSTER_HOPE)     return 3; // HopeFear
        if (designID == POSTER_COLOR)    return 2; // ColorBloody
        break;
    }
    return 0;
}



// Called by the Lighting system. If a nearby light turns off, the frame jumpscares.
void Frames_SyncToLight(s8 floor, int lightIndex, bool isLightOn) {

    if (!CentralPool_IsSystemActive(AnomalyFiles::Frames_isActive)) return;

    // Calculate the world X position of the light bulb
    float lightX = lightIndex * 600.0f + 300.0f;

    for (int f = 0; f < FRAMES_PERLVL; ++f) {
        FrameAnomaly& frame = levelMap[floor][f];

        // Only affect frames that are physically close to this specific light
        if (fabs(frame.posX - lightX) < 350.0f) {
            if (!isLightOn) {
                // LIGHTS OUT: Pick a random scary texture, pop it upwards, and jitter its X position
                frame.currentState = (rand() % (MAX_FRAME_STATES - 1)) + 1;
                frame.posY = 30.0f;
                frame.posX += (rand() % 20) - 10.0f;
            }
            else {
                // LIGHTS ON: Reset the frame to perfectly normal immediately
                frame.currentState = 0;
                frame.posY = 0.0f;

                // Recalculate its original base position
                frame.posX = f * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            }
        }
    }
}

// Calculates how many seconds a frame spends glitching, or how many seconds it waits before glitching again.
static float GetFrameTimerByIllness(ILLNESSES illness, bool isGlitchingPhase, int extraAnomalyCount) {
    float t = 1.0f;

    switch (illness) {
    case ILLNESSES::MANIA:
    case ILLNESSES::SCHIZOPHRENIA:
        // High energy: Glitches are extremely brief (0.05s-0.25s) and happen very often
        if (isGlitchingPhase) t = (float)((rand() % 20) + 5) / 100.0f;
        else                  t = (float)((rand() % 50) + 10) / 100.0f;
        break;

    case ILLNESSES::DEPRESSION:
    case ILLNESSES::INSOMNIA:
        // Low energy: Glitches last a very long time (1.0s-3.0s), but happen frequently
        if (isGlitchingPhase) t = (float)((rand() % 200) + 100) / 100.0f;
        else                  t = (float)((rand() % 50) + 10) / 100.0f;
        break;

    case ILLNESSES::DEMENTIA:
    case ILLNESSES::OCD:
        // Mid-paced: Moderate glitch times, long wait times between glitches
        if (isGlitchingPhase) t = (float)((rand() % 150) + 50) / 100.0f;
        else                  t = (float)((rand() % 300) + 100) / 100.0f;
        break;

    default:
        t = 1.0f;
        break;
    }

    // GHOST MODIFIER: Speeds up the pacing by 10-15% to make the environment feel slightly more frantic
    if (extraAnomalyCount >= 1) {
        t *= isGlitchingPhase ? 0.85f : 0.90f;

        // Prevent the timer from hitting zero, which would cause an infinite loop freeze
        if (t < 0.05f) t = 0.05f;
    }

    return t;
}

static void Frames_ResetAll() {
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];
            currentFrame.currentState = 0; // Everything is normal

            // Cycle through all 5 normal posters sequentially (0, 1, 2, 3, 4)
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
    Frames_Unload(); // Clean up old textures before loading new ones

    // --- 1. THE PATH MAP ---
    const char* framePaths[TOTAL_DESIGNS][MAX_FRAME_STATES] = {

        // POSTER_EVERYDAY (5 states)
        {
            Assets::Frame_Anomaly::EverydayNormal,
            Assets::Frame_Anomaly::EverydayPlain,
            Assets::Frame_Anomaly::EverydayEye,
            Assets::Frame_Anomaly::EverydayHaunt,
            Assets::Frame_Anomaly::EverydayNegative
        },

        // POSTER_HOPE (5 states)
        {
            Assets::Frame_Anomaly::HopeNormal,
            Assets::Frame_Anomaly::HopePlain,
            Assets::Frame_Anomaly::HopeTear,
            Assets::Frame_Anomaly::HopeFear,
            Assets::Frame_Anomaly::HopeEye
        },

        // POSTER_RECOVERY (5 states)
        {
            Assets::Frame_Anomaly::RecoveryNormal,
            Assets::Frame_Anomaly::RecoveryPlain,
            Assets::Frame_Anomaly::RecoveryScary,
            Assets::Frame_Anomaly::RecoverySinking,
            Assets::Frame_Anomaly::RecoveryMemLoss
        },

        // POSTER_SIGNS (3 states)
        {
            Assets::Frame_Anomaly::SignsNormal,
            Assets::Frame_Anomaly::SignsBlood,
            Assets::Frame_Anomaly::SignsBeware,
            nullptr,
            nullptr
        },

        // POSTER_COLOR (3 states)
        {
            Assets::Frame_Anomaly::ColorNormal,
            Assets::Frame_Anomaly::ColorFaces,
            Assets::Frame_Anomaly::ColorBloody,
            nullptr,
            nullptr
        }
    };

    // --- 2. LOAD THE TEXTURES ---
    for (int design = 0; design < TOTAL_DESIGNS; design++) {
        // Only loop up to the specific number of states this design actually has
        for (int state = 0; state < DesignStateCounts[design]; state++) {

            // Safety check: Only load if the path isn't a nullptr
            if (framePaths[design][state] != nullptr) {
                AllFrameDesigns[design][state] = LoadTextureChecked(framePaths[design][state]);
            }
        }
    }
}

// Sets up the initial positions and assigns a design (1, 2, or 3) to each frame
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

            // Mathematical trick: stagger the design IDs based on the floor level (even vs odd)
            // This ensures vertical variety so frames don't look identical across floors.
            if (level % 2 == 0) currentFrame.designID = ((frame + level) % 3) + 1;
            else                currentFrame.designID = 3 - ((frame + level) % 3);
        }
    }

    // Create the square geometry we will paint the frame textures onto
    if (frameMesh == nullptr) frameMesh = CreateSquareMesh(0xFFFFFFFF);
}

/***************************************** UPDATE ****************************************/
void Frames_Update(float dt) {
    
    if (!Player_HasPatient() || !CentralPool_IsSystemActive(AnomalyFiles::Frames_isActive)) {
        Frames_ResetAll();
        return;
    }

    const ILLNESSES effIllness = Frames_GetEffectiveIllness();
    const int extraAnoms = Frames_GetExtraAnomalyCount();

    // Static variables act as a simple internal state machine for the glitch timing
    static int timerState = STATE_NORMAL;
    static float timer = 2.0f;
    static float animTime = 0.0f;

    timer -= dt; // Tick down the clock

    // Only progress the animation timer if we are actively glitching
    if (timerState == STATE_GLITCHING) animTime += dt;
    else                               animTime = 0.0f;

    switch (timerState) {
    case STATE_NORMAL:
        // We have waited long enough. Start a glitch!
        if (timer <= 0.0f) {
            timerState = STATE_GLITCHING;
            timer = GetFrameTimerByIllness(effIllness, true, extraAnoms);
        }
        break;

    case STATE_GLITCHING:
        for (int level = 0; level < NUM_OF_FLOOR; ++level) {
            for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
                FrameAnomaly& f = levelMap[level][frame];

                // If this frame hasn't picked a scary texture yet, assign it one
                if (f.currentState == 0) {
                    // Ask the dictionary exactly which state this poster should become
                    f.currentState = GetSpecificGlitchState(effIllness, f.designID);

                    // Ghost override: 25% chance to force it to its absolute scariest state
                    if (f.currentState > 0 && extraAnoms >= 1 && (rand() % 100) < 25) {
                        f.currentState = DesignStateCounts[f.designID] - 1;
                    }
                }

                // MANIA OVERRIDE: Ignore the assigned texture and rapidly cycle through all valid textures
                if (effIllness == ILLNESSES::MANIA && f.currentState > 0) {
                    float speed = 20.0f; // Cycle 20 times per second

                    // SAFE CYCLING: Only cycle up to the max states THIS specific poster has!
                    int maxScaryStates = DesignStateCounts[f.designID] - 1;
                    f.currentState = (int)(fmodf(animTime * speed, (float)maxScaryStates)) + 1;
                }
                // Calculate where the frame *should* be
                float baseX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
                float baseY = 0.0f;

                // Apply physical shifts and scaling based on the illness
                switch (effIllness) {
                case ILLNESSES::DEMENTIA:
                case ILLNESSES::MANIA:
                    // Violent, erratic vibrating in all directions
                    f.posX = baseX + ((rand() % 60) - 30);
                    f.posY = baseY + ((rand() % 60) - 30);
                    break;

                case ILLNESSES::AIW_SYNDROME: {
                    // Alice in Wonderland: Frames throb and breathe smoothly
                    // sine wave oscillates between 0.5 and 1.5 multiplier
                    float scale = 1.0f + (sinf(animTime * 3.0f) * 0.5f);
                    float newWidth = FRAME_WIDTH * scale;
                    float newHeight = FRAME_HEIGHT * scale;

                    // Adjust the X and Y so the frame scales from the center, not from the corner
                    f.posX = baseX - ((newWidth - FRAME_WIDTH) / 2.0f);
                    f.posY = baseY - ((newHeight - FRAME_HEIGHT) / 2.0f);
                    f.width = newWidth;
                    f.height = newHeight;
                } break;

                case ILLNESSES::SCHIZOPHRENIA:
                    // Paranoid shifting: Frames snap hard to the left or right, and sometimes jump up
                    if (rand() % 10 > 7) f.posX = baseX + 20.0f;
                    else                 f.posX = baseX - 20.0f;

                    if (rand() % 100 > 95) f.posY = baseY + 50.0f;
                    break;

                case ILLNESSES::DEPRESSION:
                    // Frames feel heavy and slowly slide down the wall
                    f.posY = baseY - (animTime * 40.0f);
                    // Add a tiny, weak shudder
                    f.posX = baseX + ((rand() % 4) - 2);
                    break;

                default:
                    // Default light jittering for unhandled illnesses
                    f.posX = baseX + ((rand() % 10) - 5);
                    f.posY = baseY + ((rand() % 10) - 5);
                    break;
                }

                // GHOST MODIFIER: Adds violent micro-stutters to the frame on top of its normal behavior
                if (extraAnoms >= 1 && (rand() % 100) < 20) {
                    f.posX += (rand() % 8) - 4;
                    f.posY += (rand() % 8) - 4;
                }
            }
        }

        // The glitch duration is over. Move to the cooldown phase.
        if (timer <= 0.0f) {
            timerState = STATE_COOLDOWN;
            // Force a fixed 0.1 second snap-back to normal before picking the next timer
            timer = 0.1f;
        }
        break;

    case STATE_COOLDOWN:
        // Snap everything back to normal
        Frames_ResetAll();

        // Cooldown finished. Determine how long to wait until the next glitch.
        if (timer <= 0.0f) {
            timerState = STATE_NORMAL;
            timer = GetFrameTimerByIllness(effIllness, false, extraAnoms);
        }
        break;
    }
}

/***************************************** DRAW ******************************************/
void Frames_Draw(int currentLevel, f32 camX) {
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;
    if (frameMesh == nullptr) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];

        // Pull directly from the 2D array!
        AEGfxTexture* Tex = AllFrameDesigns[currentFrame.designID][currentFrame.currentState];

        if (Tex) {
            const f32 drawX = currentFrame.posX + camX;
            DrawTextureMesh(frameMesh, Tex, drawX, currentFrame.posY, currentFrame.width, currentFrame.height, 1.0f);
        }
    }
} 

/**************************************** UNLOAD *****************************************/
// Called when the game shuts down or restarts to clean up video memory
void Frames_Unload() {
    if (frameMesh) {
        FreeMeshSafe(frameMesh);
        frameMesh = nullptr;
    }

    // Safely unload everything in the grid
    for (int design = 0; design < TOTAL_DESIGNS; design++) {
        for (int state = 0; state < MAX_FRAME_STATES; state++) {
            if (AllFrameDesigns[design][state] != nullptr) {
                UnloadTextureSafe(AllFrameDesigns[design][state]);
                AllFrameDesigns[design][state] = nullptr;
            }
        }
    }
}
#include "pch.hpp"

const f32 FRAME_WIDTH = 100.0f;
const f32 FRAME_HEIGHT = 130.0f;
const f32 FRAME_SPACING = DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET = 50.0f;

const int FRAMES_PERLVL = 6;
const int FRAME_STATES = 5;

// --- Texture Arrays ---
static AEGfxTexture* framedesign_1[FRAME_STATES]{};
static AEGfxTexture* framedesign_2[FRAME_STATES]{};
static AEGfxTexture* framedesign_3[FRAME_STATES]{};


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

// Selects a random corrupted texture state (1-4) based on the patient's illness.
static int GetRandomStateByIllness(ILLNESSES illness, int extraAnomalyCount) {
    // These vectors define which scary textures fit the "theme" of the illness.
    static const std::vector<int> paranoiaPool{ 3, 4 };
    static const std::vector<int> maniaPool{ 2, 3 };
    static const std::vector<int> depressionPool{ 2, 3, 4 };
    static const std::vector<int> dementiaPool{ 1, 2 };
    static const std::vector<int> schizophreniaPool{ 1, 3 };
    static const std::vector<int> aiwPool{ 2, 4 };
    static const std::vector<int> insomniaPool{ 3, 1 };
    static const std::vector<int> ocdPool{ 4, 2 };
    static const std::vector<int> scotophobiaPool{ 1, 4 };

    // Lambda helper function to pick a random state from the provided pool
    auto pick = [&](std::vector<int> const& pool) -> int {
        if (pool.empty()) return 0;

        // Pick a random index from the pool
        int chosen = pool[rand() % (int)pool.size()];

        // GHOST MODIFIER: If the patient is a ghost, there is a 25% chance 
        // to override the choice with the absolute scariest frame state (4).
        if (extraAnomalyCount >= 1 && (rand() % 100) < 25) {
            chosen = 4;
        }
        return chosen;
        };

    // Route the current illness to its specific pool
    switch (illness) {
    case ILLNESSES::PARANOIA:       return pick(paranoiaPool);
    case ILLNESSES::MANIA:          return pick(maniaPool);
    case ILLNESSES::DEPRESSION:     return pick(depressionPool);
    case ILLNESSES::DEMENTIA:       return pick(dementiaPool);
    case ILLNESSES::SCHIZOPHRENIA:  return pick(schizophreniaPool);
    case ILLNESSES::AIW_SYNDROME:   return pick(aiwPool);
    case ILLNESSES::INSOMNIA:       return pick(insomniaPool);
    case ILLNESSES::OCD:            return pick(ocdPool);
    case ILLNESSES::SCOTOPHOBIA:    return pick(scotophobiaPool);
    default:                        return 0; // Fallback to normal
    }
}

// Called by the Lighting system. If a nearby light turns off, the frame jumpscares.
void Frames_SyncToLight(s8 floor, int lightIndex, bool isLightOn) {
    // Calculate the world X position of the light bulb
    float lightX = lightIndex * 600.0f + 300.0f;

    for (int f = 0; f < FRAMES_PERLVL; ++f) {
        FrameAnomaly& frame = levelMap[floor][f];

        // Only affect frames that are physically close to this specific light
        if (fabs(frame.posX - lightX) < 350.0f) {
            if (!isLightOn) {
                // LIGHTS OUT: Pick a random scary texture, pop it upwards, and jitter its X position
                frame.currentState = (rand() % (FRAME_STATES - 1)) + 1;
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

// Helper function to force all frames back to their perfectly normal state and original positions
static void Frames_ResetAll() {
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];
            currentFrame.currentState = 0;
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
        }
    }
}

/***************************************** LOAD ******************************************/
void Frames_Load() {
    Frames_Unload(); // Clean up old textures before loading new ones to prevent memory leaks

    // Loop through the 5 states and load the corresponding PNG file for each design
    for (int i = 0; i < FRAME_STATES; i++) {
        std::string suffix = "_ (" + std::to_string(i) + ").png";

        framedesign_1[i] = LoadTextureChecked(("Assets/Frame_Anomaly/Frame_1" + suffix).c_str());
        framedesign_2[i] = LoadTextureChecked(("Assets/Frame_Anomaly/Frame_2" + suffix).c_str());
        framedesign_3[i] = LoadTextureChecked(("Assets/Frame_Anomaly/Frame_3" + suffix).c_str());
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
    // Optimization & Safety: If there is no patient, the environment is safe. Reset and exit.
    if (!Player_HasPatient()) {
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
                if (f.currentState == 0) f.currentState = GetRandomStateByIllness(effIllness, extraAnoms);

                // MANIA OVERRIDE: Ignore the assigned texture and rapidly cycle through all textures
                if (effIllness == ILLNESSES::MANIA) {
                    float speed = 20.0f; // Cycle 20 times per second
                    // Use fmodf to loop the animation time, ensuring it picks states 1 through 4
                    f.currentState = (int)(fmodf(animTime * speed, (float)(FRAME_STATES - 1))) + 1;
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
    // Safety checks to prevent array out-of-bounds crashes
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;
    if (frameMesh == nullptr) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];
        AEGfxTexture* Tex = nullptr;

        // Select the correct texture array based on the frame's assigned design ID
        switch (currentFrame.designID) {
        case 1:  Tex = framedesign_1[currentFrame.currentState]; break;
        case 2:  Tex = framedesign_2[currentFrame.currentState]; break;
        case 3:  Tex = framedesign_3[currentFrame.currentState]; break;
        default: Tex = framedesign_1[0]; break;
        }

        if (Tex) {
            // Apply camera parallax/offset to ensure frames move with the hallway
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

    for (int i = 0; i < FRAME_STATES; i++) {
        UnloadTextureSafe(framedesign_1[i]);
        UnloadTextureSafe(framedesign_2[i]);
        UnloadTextureSafe(framedesign_3[i]);

        framedesign_1[i] = nullptr;
        framedesign_2[i] = nullptr;
        framedesign_3[i] = nullptr;
    }
}
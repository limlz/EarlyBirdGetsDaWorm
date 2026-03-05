#include "pch.hpp"

/*************************************** VARIABLES ***************************************/
const f32 FRAME_WIDTH		= 100.0f;
const f32 FRAME_HEIGHT		= 130.0f;

const f32 FRAME_SPACING		= DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET			= 50.0f;

const int FRAMES_PERLVL		= 6;
const int FRAME_STATES		= 5;
//const int MAX_FRAMES        = 5;

static AEGfxTexture* framedesign_1[FRAME_STATES];
static AEGfxTexture* framedesign_2[FRAME_STATES];
static AEGfxTexture* framedesign_3[FRAME_STATES];
static AEGfxVertexList* frameMesh;
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL];

/************************************* HELPERS *******************************************/
int GetRandomStateByIllness(ILLNESSES illness) {

    // Temporary repetition for testing
    static const std::vector<int> paranoiaPool { 3 , 4 };
    static const std::vector<int> maniaPool = { 2, 3 };
    static const std::vector<int> depressionPool = { 2 , 3, 4 };
    static const std::vector<int> dementiaPool = { 1 , 2 };
	static const std::vector<int> schizophreniaPool = { 1 , 3 };
	static const std::vector<int> aiwPool = { 2 , 4 };
	static const std::vector<int> insomniaPool = { 3 , 1 };
	static const std::vector<int> ocdPool = { 4 , 2 };
	static const std::vector<int> scotophobiaPool = { 1, 4 };
	static const std::vector<int> allPool = { 1, 2, 3, 4};

    switch (illness) {
    case ILLNESSES::PARANOIA:      return paranoiaPool[rand() % paranoiaPool.size()];
    case ILLNESSES::MANIA:         return maniaPool[rand() % maniaPool.size()];
    case ILLNESSES::DEPRESSION:    return depressionPool[rand() % depressionPool.size()];
    case ILLNESSES::DEMENTIA:      return dementiaPool[rand() % dementiaPool.size()];

    case ILLNESSES::SCHIZOPHRENIA:  return schizophreniaPool[rand() % schizophreniaPool.size()];
    case ILLNESSES::AIW_SYNDROME:   return aiwPool[rand() % aiwPool.size()];
    case ILLNESSES::INSOMNIA:       return insomniaPool[rand() % insomniaPool.size()];
    case ILLNESSES::OCD:            return ocdPool[rand() % ocdPool.size()];
    case ILLNESSES::SCOTOPHOBIA:    return scotophobiaPool[rand() % scotophobiaPool.size()];

    default:            return 0; // Default to normal state
    }
}

void Frames_SyncToLight(s8 floor, int lightIndex, bool isLightOn) {

    float lightX = lightIndex * 600.0f + 300.0f;

    for (int f = 0; f < FRAMES_PERLVL; ++f) {
        FrameAnomaly& frame = levelMap[floor][f];

        //  Distance Check: Is this frame close enough to be affected by this light?
        //  (350.0f covers the immediate area under the cone)
        if (fabs(frame.posX - lightX) < 350.0f) {

            if (!isLightOn) {
                // --- LIGHT OFF (DARKNESS) -> SCARY STATE ---
                // Pick a scary texture (1, 2, or 3)
                frame.currentState = (rand() % (FRAME_STATES - 1)) + 1;

                // Optional: Apply an immediate "Jumpscare" offset
                frame.posY = 30.0f;
                frame.posX += (rand() % 20) - 10.0f; // Jitter
            }
            else {
                // --- LIGHT ON (SAFE) -> NORMAL STATE ---
                frame.currentState = 0; // Back to normal painting

                // Reset positions
                frame.posY = 0.0f;
                frame.posX = f * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            }
        }
    }
}

float GetFrameTimerByIllness(ILLNESSES illness, bool isGlitchingPhase) {
    switch (illness) {
    case ILLNESSES::MANIA:
    case ILLNESSES::SCHIZOPHRENIA:
        // Hyperactive: Fast glitches (0.05s), Short waits (0.2s)
        if (isGlitchingPhase) return (float)((rand() % 20) + 5) / 100.0f;
        else                  return (float)((rand() % 50) + 10) / 100.0f; 
    case ILLNESSES::DEPRESSION:
    case ILLNESSES::INSOMNIA:
        // Sluggish: Long glitches (2.0s), Short normal periods
        if (isGlitchingPhase) return (float)((rand() % 200) + 100) / 100.0f;
        else                  return (float)((rand() % 50) + 10) / 100.0f;
    case ILLNESSES::DEMENTIA:
    case ILLNESSES::OCD:
        // Confused: Medium pacing
        if (isGlitchingPhase) return (float)((rand() % 150) + 50) / 100.0f;
        else                  return (float)((rand() % 300) + 100) / 100.0f; 
    
    default: return 1.0f;
    }
}

void Frames_ResetAll() {
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];

            // 1. Reset State
            currentFrame.currentState = 0;

            // 2. Reset Position
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;

            // 3. Reset Dimensions (Optional, but good safety if you add scaling later)
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
        }
    }
}

/*************************************** ACCESSORS ***************************************/


/***************************************** LOAD ******************************************/
void Frames_Load() {
	Frames_Unload(); // Ensure previous assets are cleared

    for (int i = 0; i < FRAME_STATES; i++) {

        int fileIndex = i;
        std::string suffix = "_ (" + std::to_string(fileIndex) + ").png";

        // Load Design 1 (Recovery is a journey...)
        std::string path1 = "Assets/Frame_Anomaly/Frame_1" + suffix;
        framedesign_1[i] = LoadTextureChecked(path1.c_str());

        // Load Design 2 (Every day is a step forward...)
        std::string path2 = "Assets/Frame_Anomaly/Frame_2" + suffix;
        framedesign_2[i] = LoadTextureChecked(path2.c_str());

        // Load Design 3 (Hope is stronger than fear...)
        std::string path3 = "Assets/Frame_Anomaly/Frame_3" + suffix;
        framedesign_3[i] = LoadTextureChecked(path3.c_str());
    }
}

/************************************** INITIALIZE ***************************************/
void Frames_Initialize() {

	for (int level = 0; level < NUM_OF_FLOOR; ++level) {

		for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
			FrameAnomaly& currentFrame = levelMap[level][frame];

            currentFrame.entity = HUMAN;    // ghost removed
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
			currentFrame.width = FRAME_WIDTH;
			currentFrame.height = FRAME_HEIGHT;
            currentFrame.currentState = 0;

            if (level % 2 == 0) {
                // Normal sequence for Even floors
                currentFrame.designID = ((frame + level) % 3) + 1;
            }
            else {
                // Reversed sequence for Odd floors
                currentFrame.designID = 3 - ((frame + level) % 3);
            }
		}
	}

    if (frameMesh == nullptr) {
        frameMesh = CreateSquareMesh(0xFFFFFFFF);
    }
}

/***************************************** UPDATE ****************************************/
void Frames_Update(float dt) {

    if (!Player_HasPatient()) {
        Frames_ResetAll();
        return;
    }

    ILLNESSES currentIllness = Player_GetCurrentIllness(); // ghost removed

    if (currentIllness == ILLNESSES::PARANOIA || currentIllness == ILLNESSES::SCOTOPHOBIA) {
        return;
    }

    static int timerState = STATE_NORMAL;
    static float timer = 2.0f;
    static float animTime = 0.0f;

    timer -= dt;
    if (timerState == STATE_GLITCHING) animTime += dt;
    else animTime = 0.0f;

    switch (timerState) {

    case STATE_NORMAL:
        if (timer <= 0.0f) {
            timerState = STATE_GLITCHING;
            timer = GetFrameTimerByIllness(currentIllness, true);
        }
        break;

    case STATE_GLITCHING:
        for (int level = 0; level < NUM_OF_FLOOR; ++level) {
            for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
                FrameAnomaly& f = levelMap[level][frame];

                if (f.currentState == 0) {
                    f.currentState = GetRandomStateByIllness(currentIllness);
                }

                if (currentIllness == ILLNESSES::MANIA) {
                    float speed = (currentIllness == ILLNESSES::ALL) ? 30.0f : 20.0f;
                    f.currentState = (int)(fmodf(animTime * speed, (float)(FRAME_STATES - 1))) + 1;
                }

                float baseX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
                float baseY = 0.0f;

                switch (currentIllness) {
                case ILLNESSES::DEMENTIA:
                case ILLNESSES::MANIA:
                    f.posX = baseX + ((rand() % 60) - 30);
                    f.posY = baseY + ((rand() % 60) - 30);
                    break;

                case ILLNESSES::AIW_SYNDROME:
                { 
                    float scale = 1.0f + (sinf(animTime * 3.0f) * 0.5f);

                    float newWidth = FRAME_WIDTH * scale;
                    float newHeight = FRAME_HEIGHT * scale;

                    f.posX = baseX - ((newWidth - FRAME_WIDTH) / 2.0f);
                    f.posY = baseY - ((newHeight - FRAME_HEIGHT) / 2.0f);

                    f.width = newWidth;
                    f.height = newHeight;
                }
                break;

                case ILLNESSES::SCHIZOPHRENIA:
                    if (rand() % 10 > 7) f.posX = baseX + 20.0f;
                    else f.posX = baseX - 20.0f;

                    if (rand() % 100 > 95) f.posY = baseY + 50.0f;
                    break;

                case ILLNESSES::DEPRESSION:
                    f.posY = baseY - (animTime * 40.0f);
                    f.posX = baseX + ((rand() % 4) - 2);
                    break;

                default:
                    f.posX = baseX + ((rand() % 10) - 5);
                    f.posY = baseY + ((rand() % 10) - 5);
                    break;
                }
            }
        }

        if (timer <= 0.0f) {
            timerState = STATE_COOLDOWN;
            timer = 0.1f;
        }
        break;

    case STATE_COOLDOWN:
        Frames_ResetAll();

        if (timer <= 0.0f) {
            timerState = STATE_NORMAL;
            timer = GetFrameTimerByIllness(currentIllness, false);
        }
        break;
    }
}

/***************************************** DRAW ******************************************/
void Frames_Draw(int currentLevel, f32 camX) {
    // 1. Safety check for level bounds
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;
    if (frameMesh == nullptr) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];

        AEGfxTexture* Tex = nullptr;

        switch (currentFrame.designID) {
        case 1:  Tex = framedesign_1[currentFrame.currentState]; break;
        case 2:  Tex = framedesign_2[currentFrame.currentState]; break;
        case 3:  Tex = framedesign_3[currentFrame.currentState]; break;
        default: Tex = framedesign_1[0]; break; // Fallback
        }

        if (Tex) {
            const f32 drawX = currentFrame.posX + camX;
            DrawTextureMesh(
                frameMesh,
                Tex,
                drawX, currentFrame.posY,
                currentFrame.width, currentFrame.height,
                1.0f                
            );
        }
    }
}

/**************************************** UNLOAD *****************************************/
void Frames_Unload() {

    if (frameMesh) {
        FreeMeshSafe(frameMesh); 
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

/*****************************************************************************************/

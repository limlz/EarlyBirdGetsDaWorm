#include "pch.hpp"
#include <ctime>
#include <cmath>
#include <vector>
#include <string>   // needed for Frames_Load (std::string)

/**************************************************************************************************
    FRAMES.CPP
    - Draws painting frames per floor.
    - When carrying a patient, frames can glitch based on the patient's illness.

    GHOST SUPPORT:
    - If Player_GetCurrentIllness() == ILLNESSES::GHOST:
        * Use Player_GetMimicIllness() as the "effective" illness for frame behavior.
        * Apply "+1 extra anomaly" as slightly more frequent / intense glitches.

    NOTES:
    - Keeps your existing logic and structure.
    - Only refactors for clarity + correctness + ghost behavior.
**************************************************************************************************/

/*************************************** CONSTANTS ***********************************************/
const f32 FRAME_WIDTH = 100.0f;
const f32 FRAME_HEIGHT = 130.0f;

const f32 FRAME_SPACING = DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET = 50.0f;

const int FRAMES_PERLVL = 6;
const int FRAME_STATES = 5; // 0..4 where 0 = normal, 1..4 = glitch/scary states

/*************************************** RENDER DATA *********************************************/
static AEGfxTexture* framedesign_1[FRAME_STATES]{};
static AEGfxTexture* framedesign_2[FRAME_STATES]{};
static AEGfxTexture* framedesign_3[FRAME_STATES]{};

static AEGfxVertexList* frameMesh = nullptr;

/*************************************** MAP DATA ************************************************/
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL]{};

/************************************* INTERNAL HELPERS ******************************************/

// If patient is a ghost, frames should still be based on the mimicked real illness.
static ILLNESSES Frames_GetEffectiveIllness()
{
    ILLNESSES current = Player_GetCurrentIllness();
    if (current == ILLNESSES::GHOST)
        return Player_GetMimicIllness();
    return current;
}

// +1 anomaly idea for ghost: a small intensity boost.
// Returns 1 when ghost (extra anomaly), otherwise 0.
static int Frames_GetExtraAnomalyCount()
{
    if (Player_GetCurrentIllness() == ILLNESSES::GHOST)
        return Player_GetGhostExtraAnomalies(); // expected 1
    return 0;
}

// Pick a random scary state based on illness.
// Returns 0..4 where 0 is normal and 1..4 are scary/glitch.
static int GetRandomStateByIllness(ILLNESSES illness, int extraAnomalyCount)
{
    // Pools of scary states (1..4). 0 is normal.
    static const std::vector<int> paranoiaPool{ 3, 4 };
    static const std::vector<int> maniaPool{ 2, 3 };
    static const std::vector<int> depressionPool{ 2, 3, 4 };
    static const std::vector<int> dementiaPool{ 1, 2 };

    static const std::vector<int> schizophreniaPool{ 1, 3 };
    static const std::vector<int> aiwPool{ 2, 4 };
    static const std::vector<int> insomniaPool{ 3, 1 };
    static const std::vector<int> ocdPool{ 4, 2 };
    static const std::vector<int> scotophobiaPool{ 1, 4 };

    // "+1 anomaly": bias slightly toward higher scary states
    auto pick = [&](std::vector<int> const& pool) -> int
        {
            if (pool.empty()) return 0;

            int chosen = pool[rand() % (int)pool.size()];

            // If ghost (+1), occasionally bump it to a "stronger" state
            if (extraAnomalyCount >= 1 && (rand() % 100) < 25)
                chosen = 4;

            return chosen;
        };

    switch (illness)
    {
    case ILLNESSES::PARANOIA:       return pick(paranoiaPool);
    case ILLNESSES::MANIA:          return pick(maniaPool);
    case ILLNESSES::DEPRESSION:     return pick(depressionPool);
    case ILLNESSES::DEMENTIA:       return pick(dementiaPool);

    case ILLNESSES::SCHIZOPHRENIA:  return pick(schizophreniaPool);
    case ILLNESSES::AIW_SYNDROME:   return pick(aiwPool);
    case ILLNESSES::INSOMNIA:       return pick(insomniaPool);
    case ILLNESSES::OCD:            return pick(ocdPool);
    case ILLNESSES::SCOTOPHOBIA:    return pick(scotophobiaPool);

    default:                        return 0; // normal
    }
}

// Sync nearby frames to a light cone (your existing logic kept).
void Frames_SyncToLight(s8 floor, int lightIndex, bool isLightOn)
{
    float lightX = lightIndex * 600.0f + 300.0f;

    for (int f = 0; f < FRAMES_PERLVL; ++f)
    {
        FrameAnomaly& frame = levelMap[floor][f];

        // Distance check: is this frame near this light?
        if (fabs(frame.posX - lightX) < 350.0f)
        {
            if (!isLightOn)
            {
                // LIGHT OFF -> scary state
                frame.currentState = (rand() % (FRAME_STATES - 1)) + 1;

                // Optional "jumpscare" offset
                frame.posY = 30.0f;
                frame.posX += (rand() % 20) - 10.0f;
            }
            else
            {
                // LIGHT ON -> normal state
                frame.currentState = 0;

                // Reset positions
                frame.posY = 0.0f;
                frame.posX = f * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            }
        }
    }
}

// Glitch timing depends on illness.
// isGlitchingPhase:
// - true  -> duration of glitch burst
// - false -> duration of normal/calm period before next glitch
static float GetFrameTimerByIllness(ILLNESSES illness, bool isGlitchingPhase, int extraAnomalyCount)
{
    float t = 1.0f;

    switch (illness)
    {
    case ILLNESSES::MANIA:
    case ILLNESSES::SCHIZOPHRENIA:
        // Hyperactive: fast glitches
        if (isGlitchingPhase) t = (float)((rand() % 20) + 5) / 100.0f;  // 0.05..0.24
        else                  t = (float)((rand() % 50) + 10) / 100.0f; // 0.10..0.59
        break;

    case ILLNESSES::DEPRESSION:
    case ILLNESSES::INSOMNIA:
        // Sluggish: long glitches
        if (isGlitchingPhase) t = (float)((rand() % 200) + 100) / 100.0f; // 1.00..2.99
        else                  t = (float)((rand() % 50) + 10) / 100.0f;   // 0.10..0.59
        break;

    case ILLNESSES::DEMENTIA:
    case ILLNESSES::OCD:
        // Confused: medium pacing
        if (isGlitchingPhase) t = (float)((rand() % 150) + 50) / 100.0f;  // 0.50..1.99
        else                  t = (float)((rand() % 300) + 100) / 100.0f; // 1.00..3.99
        break;

    default:
        t = 1.0f;
        break;
    }

    // "+1 anomaly" for ghost: slightly faster/more frequent glitches (subtle)
    if (extraAnomalyCount >= 1)
    {
        // reduce timer a bit but don't go crazy
        t *= isGlitchingPhase ? 0.85f : 0.90f;
        if (t < 0.05f) t = 0.05f;
    }

    return t;
}

static void Frames_ResetAll()
{
    for (int level = 0; level < NUM_OF_FLOOR; ++level)
    {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame)
        {
            FrameAnomaly& currentFrame = levelMap[level][frame];

            // Reset state
            currentFrame.currentState = 0;

            // Reset position
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;

            // Reset size
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
        }
    }
}

/***************************************** LOAD ******************************************/
void Frames_Load()
{
    Frames_Unload(); // Ensure previous assets are cleared

    for (int i = 0; i < FRAME_STATES; i++)
    {
        int fileIndex = i;
        std::string suffix = "_ (" + std::to_string(fileIndex) + ").png";

        std::string path1 = "Assets/Frame_Anomaly/Frame_1" + suffix;
        framedesign_1[i] = LoadTextureChecked(path1.c_str());

        std::string path2 = "Assets/Frame_Anomaly/Frame_2" + suffix;
        framedesign_2[i] = LoadTextureChecked(path2.c_str());

        std::string path3 = "Assets/Frame_Anomaly/Frame_3" + suffix;
        framedesign_3[i] = LoadTextureChecked(path3.c_str());
    }
}

/************************************** INITIALIZE ***************************************/
void Frames_Initialize()
{
    for (int level = 0; level < NUM_OF_FLOOR; ++level)
    {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame)
        {
            FrameAnomaly& currentFrame = levelMap[level][frame];

            currentFrame.entity = HUMAN; // keep your comment / setup
            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
            currentFrame.width = FRAME_WIDTH;
            currentFrame.height = FRAME_HEIGHT;
            currentFrame.currentState = 0;

            // Design sequencing (your original logic kept)
            if (level % 2 == 0)
                currentFrame.designID = ((frame + level) % 3) + 1;       // even floors
            else
                currentFrame.designID = 3 - ((frame + level) % 3);       // odd floors
        }
    }

    if (frameMesh == nullptr)
        frameMesh = CreateSquareMesh(0xFFFFFFFF);
}

/***************************************** UPDATE ****************************************/
void Frames_Update(float dt)
{
    // If not carrying a patient, frames go back to normal.
    if (!Player_HasPatient())
    {
        Frames_ResetAll();
        return;
    }

    // Use effective illness (ghost uses mimic)
    const ILLNESSES effIllness = Frames_GetEffectiveIllness();
    const int extraAnoms = Frames_GetExtraAnomalyCount();

    // Your original behavior: paranoia + scotophobia do nothing (kept)
    if (effIllness == ILLNESSES::PARANOIA || effIllness == ILLNESSES::SCOTOPHOBIA)
        return;

    // Shared state across updates (kept)
    static int   timerState = STATE_NORMAL;
    static float timer = 2.0f;
    static float animTime = 0.0f;

    timer -= dt;
    if (timerState == STATE_GLITCHING) animTime += dt;
    else                              animTime = 0.0f;

    switch (timerState)
    {
    case STATE_NORMAL:
        if (timer <= 0.0f)
        {
            timerState = STATE_GLITCHING;
            timer = GetFrameTimerByIllness(effIllness, true, extraAnoms);
        }
        break;

    case STATE_GLITCHING:
        for (int level = 0; level < NUM_OF_FLOOR; ++level)
        {
            for (int frame = 0; frame < FRAMES_PERLVL; ++frame)
            {
                FrameAnomaly& f = levelMap[level][frame];

                // If still normal, choose a scary state based on illness
                if (f.currentState == 0)
                    f.currentState = GetRandomStateByIllness(effIllness, extraAnoms);

                // Mania special: rapid cycling
                if (effIllness == ILLNESSES::MANIA)
                {
                    float speed = 20.0f;
                    f.currentState = (int)(fmodf(animTime * speed, (float)(FRAME_STATES - 1))) + 1;
                }

                // Base position
                float baseX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
                float baseY = 0.0f;

                switch (effIllness)
                {
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
                    else                 f.posX = baseX - 20.0f;

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

                // "+1 anomaly" for ghost: tiny extra jitter (subtle)
                if (extraAnoms >= 1 && (rand() % 100) < 20)
                {
                    f.posX += (rand() % 8) - 4;
                    f.posY += (rand() % 8) - 4;
                }
            }
        }

        if (timer <= 0.0f)
        {
            timerState = STATE_COOLDOWN;
            timer = 0.1f;
        }
        break;

    case STATE_COOLDOWN:
        Frames_ResetAll();

        if (timer <= 0.0f)
        {
            timerState = STATE_NORMAL;
            timer = GetFrameTimerByIllness(effIllness, false, extraAnoms);
        }
        break;
    }
}

/***************************************** DRAW ******************************************/
void Frames_Draw(int currentLevel, f32 camX)
{
    // Safety check
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;
    if (frameMesh == nullptr) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i)
    {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];

        AEGfxTexture* Tex = nullptr;

        switch (currentFrame.designID)
        {
        case 1:  Tex = framedesign_1[currentFrame.currentState]; break;
        case 2:  Tex = framedesign_2[currentFrame.currentState]; break;
        case 3:  Tex = framedesign_3[currentFrame.currentState]; break;
        default: Tex = framedesign_1[0]; break;
        }

        if (Tex)
        {
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
void Frames_Unload()
{
    if (frameMesh)
    {
        FreeMeshSafe(frameMesh);
        frameMesh = nullptr;
    }

    for (int i = 0; i < FRAME_STATES; i++)
    {
        UnloadTextureSafe(framedesign_1[i]);
        UnloadTextureSafe(framedesign_2[i]);
        UnloadTextureSafe(framedesign_3[i]);

        framedesign_1[i] = nullptr;
        framedesign_2[i] = nullptr;
        framedesign_3[i] = nullptr;
    }
}

/*****************************************************************************************/
#include "pch.hpp"

const f32 FRAME_WIDTH		= 100.0f;
const f32 FRAME_HEIGHT		= 130.0f;

const f32 FRAME_SPACING		= DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET			= 50.0f;

const int FRAMES_PERLVL		= 6;
const int FRAME_STATES		= 11;
const int MAX_FRAMES        = 5;

static AEGfxTexture* framedesign_1[FRAME_STATES];
static AEGfxTexture* framedesign_2[FRAME_STATES];
static AEGfxTexture* framedesign_3[FRAME_STATES];
static AEGfxVertexList* frameMesh;
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL];


int GetRandomStateByIllness(ILLNESSES illness) {
    static const int paranoiaPool[] = { 5, 6 };
    static const int maniaPool[] = { 1, 2, 3, 4 };
    static const int depressionPool[] = { 7, 8 };
    static const int dementiaPool[] = { 9, 10 };
	static const int allPool[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    switch (illness) {
    case PARANOIA:      return paranoiaPool[rand() % 2];
    case MANIA:         return maniaPool[rand() % 4];
    case DEPRESSION:    return depressionPool[rand() % 2];
    case DEMENTIA:      return dementiaPool[rand() % 2];
	case ALL:           return allPool[rand() % 10];
    default:            return 0; // Default to normal state
    }
}

void Frames_Load() {
	Frames_Unload(); // Ensure previous assets are cleared

    for (int i = 0; i < FRAME_STATES; i++) {

        int fileIndex = i;
        std::string suffix = "_(" + std::to_string(fileIndex) + ").png";

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

void Frames_Initialize() {
	for (int level = 0; level < NUM_OF_FLOOR; ++level) {

		for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
			ILLNESSES illness;
			FrameAnomaly& currentFrame = levelMap[level][frame];
            ENTITIES entity = Player_IsScaryPatient() ? GHOST : HUMAN;

            if (entity == GHOST) {
				illness = ALL;
            } else {
                illness = Player_GetCurrentIllness();
            }
			
			currentFrame.illness = illness;
            currentFrame.entity = entity;

            currentFrame.posX = frame * FRAME_SPACING + (DIST_BETWEEN_DOORS / 2);
            currentFrame.posY = 0.0f;
			currentFrame.width = FRAME_WIDTH;
			currentFrame.height = FRAME_HEIGHT;
			currentFrame.designID = (frame % 3) + 1; // Cycle through design IDs 1, 2, 3

            currentFrame.currentState = 0;
		}
	}
    if (frameMesh == nullptr) {
        frameMesh = CreateSquareMesh(0xFFFFFFFF);
    }
}

void Frames_Update(float dt) {
    static bool  isFlickering = false;
    static float flickerInterval = (float)(rand() % 4001 + 4000) / 1000.0f;
    static float flickerDuration = 4.0f;
    
    if (!isFlickering) {
        flickerInterval -= dt;
    }
    else {
        flickerDuration -= dt;
    }

    for (int level = 0; level < NUM_OF_FLOOR; ++level) {
        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            FrameAnomaly& currentFrame = levelMap[level][frame];

            if (isFlickering) {
                if (currentFrame.illness == MANIA || currentFrame.illness == ALL) {
                    // Logic: 4.0 minus the countdown gives us a value from 0.0 to 4.0
                    // Example: At 4.0s left, frame is 1. At 0.1s left, frame is 4.
                    float timePassed = 4.0f - flickerDuration;

                    // Cast to int to get 0, 1, 2, 3, then add 1 to get frames 1-4
                    float speed = 8.0f;
                    currentFrame.currentState = (int)(fmodf(timePassed * speed, 4.0f)) + 1;

                    // Safety cap to ensure we never hit index 5
                    if (currentFrame.currentState > 4) currentFrame.currentState = 4;
                }
                else if (currentFrame.currentState == 0) {
                    currentFrame.currentState = GetRandomStateByIllness(currentFrame.illness);
                }
            }
            else {
                currentFrame.currentState = 0;
            }
        }
    }

    // 3. State Transitions
    if (!isFlickering && flickerInterval <= 0.0f) {
        isFlickering = true;
        flickerDuration = 4.0f;
    }
    else if (isFlickering && flickerDuration <= 0.0f) {
        isFlickering = false;
        flickerInterval = (float)(rand() % 4001 + 4000) / 1000.0f;
    }
}

void Frames_Draw(int currentLevel, f32 camX) {
    // 1. Safety check for level bounds
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;

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

void Frames_Unload() {

    if (frameMesh) {
        FreeMeshSafe(frameMesh); // Crucial!
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


//to later attach this to after pick up of !human && not delivery floor
//if (currentFrame.entity == GHOST ) {
//    int randomCase = rand() % 4; 
//    switch (randomCase) { 
//    case 0: 
//        currentFrame.posX -= OFFSET; 
//        break;
//    case 1: 
//        currentFrame.posX += OFFSET; 
//        break;
//    case 2: 
//        currentFrame.posY -= OFFSET; 
//        break;
//    case 3: 
//        currentFrame.posY += OFFSET; 
//        break;
//    }
//}

//to later attach this to after pick up of !human && on delivery floor
//if (currentFrame.entity == GHOST && level == 3) {
//    int randomCase = rand() % 4;
//    switch (randomCase) {
//    case 0:
//        currentFrame.width = FRAME_WIDTH * 0.75f;
//        break;
//    case 1:
//        currentFrame.height = FRAME_HEIGHT * 0.75f;
//        break;
//    case 2:
//        currentFrame.height = FRAME_HEIGHT * 1.25f;
//        break;
//    case 3:
//        currentFrame.width = FRAME_WIDTH * 1.25f;
//        break;
//    }
//}
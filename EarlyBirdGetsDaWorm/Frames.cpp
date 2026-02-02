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

namespace {

    int GetRandomStateByIllness(ILLNESSES illness) {
        static const int paranoiaPool[] = { 0, 5, 6 };
        static const int maniaPool[] = { 0, 1, 2, 3, 4 };
        static const int depressionPool[] = { 0, 7, 8 };
        static const int dementiaPool[] = { 0, 9, 10 };

        switch (illness) {
        case PARANOIA:   return paranoiaPool[rand() % 3];
        case MANIA:      return maniaPool[rand() % 5];
        case DEPRESSION: return depressionPool[rand() % 3];
        case DEMENTIA:   return dementiaPool[rand() % 3];
        default:         return 0; // Default to normal state
        }
    }

}

void Frames_Load() {
    for (int i = 0; i < FRAME_STATES; i++) {

        int fileIndex = i;
        std::string suffix = "_(" + std::to_string(fileIndex) + ").png";

        // Load Design 1 (Recovery is a journey...)
        std::string path1 = "Assets/Frame_Anomaly/Frame_1" + suffix;
        framedesign_1[i] = AEGfxTextureLoad(path1.c_str());

        // Load Design 2 (Every day is a step forward...)
        std::string path2 = "Assets/Frame_Anomaly/Frame_2" + suffix;
        framedesign_2[i] = AEGfxTextureLoad(path2.c_str());

        // Load Design 3 (Hope is stronger than fear...)
        std::string path3 = "Assets/Frame_Anomaly/Frame_3" + suffix;
        framedesign_3[i] = AEGfxTextureLoad(path3.c_str());
    }
}

void Frames_Initialize() {
	for (int level = 0; level < NUM_OF_FLOOR; ++level) {

		for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {

			FrameAnomaly& currentFrame = levelMap[level][frame];
            ENTITIES entity = Player_IsScaryPatient() ? GHOST : HUMAN;
			ILLNESSES illness = Player_GetCurrentIllness();
            
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
    frameMesh = CreateSquareMesh(0xFFFFFFFF);
}

void Frames_Update() {
    for (int level = 0; level < NUM_OF_FLOOR; ++level) {

        for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {

            FrameAnomaly& currentFrame = levelMap[level][frame];

            if (rand() % 100 < 1) {
                currentFrame.currentState = GetRandomStateByIllness(currentFrame.illness);
            }
            else {
                currentFrame.currentState = 0;
            }
        }
    }
}

void Frames_Draw(int currentLevel, f32 camX) {
    // 1. Safety check for level bounds
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        FrameAnomaly& currentFrame = levelMap[currentLevel][i];

        // 2. Determine which design array to use
        AEGfxTexture* pTex = nullptr;

        switch (currentFrame.designID) {
        case 1:  pTex = framedesign_1[currentFrame.currentState]; break;
        case 2:  pTex = framedesign_2[currentFrame.currentState]; break;
        case 3:  pTex = framedesign_3[currentFrame.currentState]; break;
        default: pTex = framedesign_1[0]; break; // Fallback
        }

        // 3. Draw the frame if the texture loaded successfully
        if (pTex) {
            const f32 drawX = currentFrame.posX + camX;

            DrawTextureMesh(
                frameMesh,
                pTex,
                drawX, currentFrame.posY,
                currentFrame.width, currentFrame.height,
                1.0f                // Opacity (1.0 = fully visible)
            );
        }
    }
}

void Frames_Unload() {

    if (frameMesh) AEGfxMeshFree(frameMesh);

    for (int i = 0; i < FRAME_STATES; i++) {
        if (framedesign_1[i]) AEGfxTextureUnload(framedesign_1[i]);
        if (framedesign_2[i])  AEGfxTextureUnload(framedesign_2[i]);
        if (framedesign_3[i])  AEGfxTextureUnload(framedesign_3[i]);

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
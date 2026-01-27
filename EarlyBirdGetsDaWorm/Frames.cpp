#include "pch.hpp"

const f32 FRAME_WIDTH		= 100.0f;
const f32 FRAME_HEIGHT		= 130.0f;

const f32 FRAME_SPACING		= DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET			= 50.0f;

const int FRAMES_PERLVL		= 6;
const int FRAMES_ID			= 3;

static AEGfxTexture* frame_textures[FRAMES_ID];
static AEGfxVertexList* frameMesh;
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL];


void Frames_Load() {
	for (int i{}; i < FRAMES_ID; i++) {
		std::string filename = "Assets/frames_" + std::to_string(i) + ".png";
		frame_textures[i] = AEGfxTextureLoad(filename.c_str());
	}
}

void Frames_Initialize() {
	for (int level = 0; level < NUM_OF_FLOOR; ++level) {
		for (int frame = 0; frame < FRAMES_PERLVL; ++frame) {
            levelMap[level][frame].type = GHOST; //to change back to human later
            levelMap[level][frame].posX = frame * FRAME_SPACING +
                (DIST_BETWEEN_DOORS / 2);
            levelMap[level][frame].posY = 0.0f;
			levelMap[level][frame].width = FRAME_WIDTH;
			levelMap[level][frame].height = FRAME_HEIGHT;
			levelMap[level][frame].textureID = (frame + level) % FRAMES_ID;

			//to later attach this to after pick up of !human && not delivery floor
            if (levelMap[level][frame].type == GHOST && level == 4) {
                int randomCase = rand() % 4; 
                switch (randomCase) { 
                case 0: 
                    levelMap[level][frame].posX -= OFFSET; 
                    break;
                case 1: 
                    levelMap[level][frame].posX += OFFSET; 
                    break;
                case 2: 
                    levelMap[level][frame].posY -= OFFSET; 
                    break;
                case 3: 
                    levelMap[level][frame].posY += OFFSET; 
                    break;
                }
            }

            //to later attach this to after pick up of !human && on delivery floor
            if (levelMap[level][frame].type == GHOST && level == 3) {
                int randomCase = rand() % 4;
                switch (randomCase) {
                case 0:
                    levelMap[level][frame].width = FRAME_WIDTH * 0.75f;
                    break;
                case 1:
                    levelMap[level][frame].height = FRAME_HEIGHT * 0.75f;
                    break;
                case 2:
                    levelMap[level][frame].height = FRAME_HEIGHT * 1.25f;
                    break;
                case 3:
                    levelMap[level][frame].width = FRAME_WIDTH * 1.25f;
                    break;
                }
            }
		}
	}

    frameMesh = CreateSquareMesh(0xFFFFFFFF);

}

void Frames_Update() {
	return;
}

void Frames_Draw(int currentLevel, f32 camX) {
    
    if (currentLevel < 0 || currentLevel >= NUM_OF_FLOOR) return;

    for (int i = 0; i < FRAMES_PERLVL; ++i) {
        
        FrameAnomaly* frame = &levelMap[currentLevel][i];

        AEGfxTexture* texture = frame_textures[frame->textureID];

        const f32 drawX = frame->posX + camX;

        DrawTextureMesh(
            frameMesh,
            frame_textures[frame->textureID],
            drawX,	frame->posY,
            frame->width,	frame->height,
            1.0f 
        );
    }
}

void Frames_Unload() {

    if (frameMesh) AEGfxMeshFree(frameMesh);

    for (int i = 0; i < FRAMES_ID; i++) {
        if (frame_textures[i]) {
            AEGfxTextureUnload(frame_textures[i]);
            frame_textures[i] = nullptr;    
        }
    }
}
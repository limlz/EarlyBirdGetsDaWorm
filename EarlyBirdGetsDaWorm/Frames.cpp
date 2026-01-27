#include "pch.hpp"

const f32 FRAME_WIDTH		= 100.0f;
const f32 FRAME_HEIGHT		= 130.0f;

const f32 FRAME_SPACING		= DIST_BETWEEN_DOORS * 2.0f;
const f32 OFFSET			= 50.0f;

const int FRAMES_PERLVL		= 6;
const int FRAMES_ID			= 3;

static AEGfxTexture* frametextures_normal[FRAMES_ID];
static AEGfxTexture* frametextures_tier1[FRAMES_ID];
static AEGfxTexture* frametextures_tier2[FRAMES_ID];
static AEGfxVertexList* frameMesh;
static FrameAnomaly levelMap[NUM_OF_FLOOR][FRAMES_PERLVL];



void Frames_Load() {
	for (int i{}; i < FRAMES_ID; i++) {
        std::string baseName = "Assets/frames_" + std::to_string(i);

        frametextures_normal[i] = AEGfxTextureLoad((baseName + ".png").c_str());

        frametextures_tier1[i] = AEGfxTextureLoad((baseName + "_t1.png").c_str());

        frametextures_tier2[i] = AEGfxTextureLoad((baseName + "_t2.png").c_str());
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

        AEGfxTexture* texture = nullptr;

        if (frame->type == GHOST) {
            if (currentLevel == 4) {
                texture = frametextures_tier1[frame->textureID]; // Tier 1 Visuals
            }
            else if (currentLevel == 3) {
                texture = frametextures_tier2[frame->textureID]; // Tier 2 Visuals
            }
            else {
                texture = frametextures_normal[frame->textureID]; // Normal Visuals
            }
        }
        else {
            texture = frametextures_normal[frame->textureID]; // Human/Normal fallback
        }

        const f32 drawX = frame->posX + camX;

        DrawTextureMesh(
            frameMesh,
            texture,
            drawX,	frame->posY,
            frame->width,	frame->height,
            1.0f 
        );
    }
}

void Frames_Unload() {

    if (frameMesh) AEGfxMeshFree(frameMesh);

    for (int i = 0; i < FRAMES_ID; i++) {
        if (frametextures_normal[i]) AEGfxTextureUnload(frametextures_normal[i]);
        if (frametextures_tier1[i])  AEGfxTextureUnload(frametextures_tier1[i]);
        if (frametextures_tier2[i])  AEGfxTextureUnload(frametextures_tier2[i]);

        frametextures_normal[i] = nullptr;
        frametextures_tier1[i] = nullptr;
        frametextures_tier2[i] = nullptr;
    }
}
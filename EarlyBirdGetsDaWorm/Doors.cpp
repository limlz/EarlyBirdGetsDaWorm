#include "pch.hpp"

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
s8 doorFontId = 0;

void Doors_Load() {
	doorFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
	return;
}

void Doors_Initialize() {
	squareMesh = CreateSquareMesh(0xFFFFFFFF);
}

int Doors_Update(float camX) {
	// How close you need to be to the center of a door to be considered "in front" of it
    float detectionRange = DOOR_WIDTH / 2.0f;

    for (int i = 0; i < NUM_DOORS; i++) {
        // Calculate world position 
        float wallX = DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * i);

        // Check if the door is centered on screen (where the player is)
        if (wallX > -detectionRange && wallX < detectionRange) {
            return i; // Player is in front of door 'i'
        }
    }

    return -1; // Not in front of any door
}

void Doors_Draw(f32 camX, s8 floorNum, f32 textXoffset, f32 textY) {
    for (int i = 0; i < NUM_DOORS; i++) {
        // Calculate world position of the door based on player offset
        float wallX = DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * i);

        // Simple Culling: Only draw if within reasonable screen bounds
        // (Adjusted logic to be relative to screen width rather than hardcoded 10)
        if (wallX > -SCREEN_WIDTH_HALF - DOOR_WIDTH && wallX < SCREEN_WIDTH_HALF + DOOR_WIDTH) {

            DrawSquareMesh(squareMesh, wallX, -100.0f, DOOR_WIDTH, DOOR_HEIGHT, COLOR_DOOR_BROWN);
			DrawSquareMesh(squareMesh, wallX, -5.0f, 160.0f, 70.0f, COLOR_BLACK);
            // Door Text
            char textBuffer[32];

            if (floorNum == 0) {
                sprintf_s(textBuffer, "B1-%02d", i + 1);
            }
            else {
                sprintf_s(textBuffer, "%02d-%02d", floorNum, i + 1);
            }

            float textNDC_X = (wallX / SCREEN_WIDTH_HALF) - textXoffset;
            float textNDC_Y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(doorFontId, textBuffer, textNDC_X, textNDC_Y, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}
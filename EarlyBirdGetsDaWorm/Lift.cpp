#include "pch.hpp"

#define LIFT_TIMER      2.0f

static bool liftActive  = false;
static bool NearLift    = false;
static float liftAnimation{};

void Lift_Initialize() {
    liftActive = false;
    NearLift = false;
}

void Lift_Update(float dt, float camX, float maxDist) {

    // Lift Interaction Check
    // Checks if player is at the far left (start) or far right (end)
    NearLift = ((camX > -5.0f || camX < -maxDist + 5.0f) && camX > -maxDist - 10.0f);

    // Toggle lift with 'L'
    if (NearLift && AEInputCheckTriggered(AEVK_L)) {
        liftActive = !liftActive;
        if (liftActive) {
            liftAnimation = LIFT_TIMER;
        }
    }

    // If we walk away, close the lift
    if (!NearLift) {
        liftActive = false;
    }

    if (liftActive) {
        Timer_SetPaused(true);
    } else {
        Timer_SetPaused(false);
	}
}

void Lift_HandleInput(s8& floorNum) {
    if (!liftActive) return;

    // Buttons would go here
    for (int i{}; i < NUM_OF_FLOOR; i++) {
        if (AEInputCheckTriggered(AEVK_0 + i)) {
            floorNum = i;
            liftActive = false;
        }
    }
}

void Lift_Draw(AEGfxVertexList* squareMesh) {
    bool animation_over = false;
    if (!liftActive) return;

    float dt = (f32)AEFrameRateControllerGetFrameTime();

    if (liftAnimation > 0.0f) {
        liftAnimation -= dt;
    } else {
        animation_over = true;
	}
    if (liftAnimation < 0.0f) liftAnimation = 0.0f;

    float gapOffset = (liftAnimation / 4.0f) * SCREEN_W;

    // Left Door: Meets at center when gapOffset is 0
    /*float leftDoorX = -(SCREEN_WIDTH_HALF * 0.5f) - gapOffset;*/
    DrawSquareMesh(squareMesh, -450.0f - gapOffset, 0.0f, SCREEN_WIDTH_HALF + (0.5 * LIFT_WIDTH), static_cast<f32>(SCREEN_H), COLOR_LIFT_GREY);

    // Right Door: Meets at center when gapOffset is 0
    /*float rightDoorX = (SCREEN_WIDTH_HALF * 0.5f) + gapOffset;*/
    DrawSquareMesh(squareMesh, 450.0f + gapOffset, 0.0f, SCREEN_WIDTH_HALF + (0.5 * LIFT_WIDTH), static_cast<f32>(SCREEN_H), COLOR_LIFT_GREY);

    if (animation_over) {
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 2.0f, static_cast<f32>(SCREEN_H), COLOR_BLACK);
        // Background rectangle
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 500.0f, 800.0f, COLOR_LIFT_BG);
        // Lift console panel
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 400.0f, 700.0f, COLOR_LIFT_CONSOLE);
        // Note: Button text rendering should be called here using your Prompts/Font system
    }
}


bool Lift_IsActive() { return liftActive; }
bool Lift_IsNear() { return NearLift; }

void Lift_Unload() {
    // Add texture unloads here if you eventually use sprites for the buttons
}
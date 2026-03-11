#include "pch.hpp"
#include <cstdio>
#include <algorithm>
#include <random>
#include <ctime>

// --- Config ---
static constexpr float LIFT_TIMER = 1.2f;    // Duration of doors sliding open
static constexpr float CLOSE_TIMER = 0.8f;   // Duration of doors sliding shut
static constexpr float TRANSIT_TIMER = 2.0f; // How long lift stays closed while moving

// --- Render Data ---
static s8 liftFontId = -1;
static AEGfxVertexList* QuadMesh = nullptr;

static AEGfxTexture* LiftTex = nullptr;
static AEGfxTexture* LiftDoorTex = nullptr;
static AEGfxTexture* LiftPanelTex = nullptr;
static AEGfxTexture* LiftWallTex[NUM_OF_FLOOR]{};
static AEGfxTexture* LiftButtonTex[NUM_OF_FLOOR]{};

// --- Anomaly State ---
static int  ButtonMap[NUM_OF_FLOOR]{};
static int  OcdTarget = -1;
static bool AnomaliesGenerated = false;

// --- General State ---
static bool LiftActive = false;
static bool NearLift = false;

// --- State Machine Variables ---
static int   LiftPhase = 0;       // 1 = Doors Closing, 2 = Panel Active, 3 = Transit & Opening
static float DoorProgress = 1.0f; // 0.0 = Fully Closed, 1.0 = Fully Open
static float PhaseTimer = 0.0f;   // Tracks time for transit delays
static int   SelectedSlot = -1;   // Tracks which button was clicked for the red highlight
static s8    PendingFloor = -1;   // The floor the lift is travelling to

/******************************* LIFT LOGIC & ANOMALIES **********************************/
static void GetButtonRect(int slotIndex, float& x, float& y, float& w, float& h) {
    w = 120.0f;
    h = 120.0f;

    const float xSpacing = 100.0f;
    const float startY = -280.0f;
    const float ySpacing = 140.0f;

	// slotIndex is 0-9 for the 10 buttons
    int col = slotIndex % 2;
    int row = slotIndex / 2;

    x = (col == 0) ? -xSpacing : xSpacing;
    y = startY + (row * ySpacing);

    // Check: lift anomalies are active?
    if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
        ILLNESSES illness = Player_GetAnomalyIllness();

        if (illness == ILLNESSES::AIW_SYNDROME) {
            float scale = 0.6f + (row * 0.2f);
            w *= scale;
            h *= scale;
        }

        if (illness == ILLNESSES::OCD && slotIndex == OcdTarget) {
            x += 8.0f;
            y -= 8.0f;
        }
    }
}

static void Lift_GenerateAnomalies() {
    // Normal Mapping; 0-9 in order followring B1 to F9
    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        ButtonMap[i] = i;
    }
    OcdTarget = -1;

	// Check: lift anomalies are active?
    if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
        ILLNESSES illness = Player_GetAnomalyIllness();

        if (illness == ILLNESSES::DEMENTIA) {
			// Shuffle the button mapping so the floors are all mixed up
            unsigned seed = (unsigned)time(NULL);
            std::shuffle(std::begin(ButtonMap), std::end(ButtonMap), std::default_random_engine(seed));
        }
        else if (illness == ILLNESSES::OCD) {
			// Pick one random button to be the OCD "target" that is slightly misaligned
            OcdTarget = rand() % NUM_OF_FLOOR;
        }
    }
    
    AnomaliesGenerated = true;
}

/***************************************** LOAD ******************************************/
void Lift_Load() {
    liftFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    LiftTex = LoadTextureChecked(Assets::Lift::LiftBg);
    LiftDoorTex = LoadTextureChecked(Assets::Lift::LiftDoor);
    LiftPanelTex = LoadTextureChecked(Assets::Lift::LiftPanel);

    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        LiftButtonTex[i] = AEGfxTextureLoad(Assets::Lift::LiftButton[i]);
    }

    for (int i = 0; i < 3; i++) {
        LiftWallTex[i] = AEGfxTextureLoad(Assets::Lift::OutsideWorld[i]);
    }
}

/*************************************** INITIALIZE **************************************/
void Lift_Initialize() {
    QuadMesh = CreateSquareMesh(0xFFFFFFFF);

    LiftActive = false;
    NearLift = false;
    AnomaliesGenerated = false;

    LiftPhase = 0;
    DoorProgress = 1.0f;
    PhaseTimer = 0.0f;  
    SelectedSlot = -1;
    PendingFloor = -1;
}

/***************************************** UPDATE ****************************************/
void Lift_Update(float /*dt*/, float camX, float maxDist) {

    const bool nearLeftLift = (camX > -5.0f);
    const bool nearRightLift = (camX < -(maxDist - 5.0f));
    NearLift = (nearLeftLift || nearRightLift);

    if (NearLift && AEInputCheckTriggered(AEVK_L)) {
        if (!LiftActive) {
            LiftActive = true;
            LiftPhase = 1;       // Start Phase 1: Closing
            DoorProgress = 1.0f; // Start fully open
            SelectedSlot = -1;
            PendingFloor = -1;

            // TODO: Add door sliding shut sound here

            if (!AnomaliesGenerated) {
                Lift_GenerateAnomalies();
            }
        }
        else if (LiftPhase == 2) {
            // If L is pressed while the panel is active, cancel and open doors
            LiftPhase = 3;
            PhaseTimer = TRANSIT_TIMER; // Skip delay, go straight to opening
            PendingFloor = -1;          // Don't change floors
        }
    }

    // Auto-close if the player walks away while panel is open
    if (!NearLift && LiftActive && LiftPhase == 2) {
        LiftPhase = 3;
        PhaseTimer = TRANSIT_TIMER;
        PendingFloor = -1;
    }

    Timer_SetPaused(LiftActive);
}

void Lift_HandleInput(s8& floorNum) {
    if (!LiftActive) return;

    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // =======================================================
    // PHASE 1: DOORS CLOSING
    // =======================================================
    if (LiftPhase == 1) {
		// Decrement door progress based on timer; 
        // When it hits 0, doors are fully closed, switching to the panel phase
        DoorProgress -= dt / CLOSE_TIMER;
        if (DoorProgress <= 0.0f) {
            DoorProgress = 0.0f;
            LiftPhase = 2; 
        }
    }
    // =======================================================
    // PHASE 2: PANEL APPEARS (AWAITING PLAYER INPUT)
    // =======================================================
    else if (LiftPhase == 2) {
        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            for (int i = 0; i < NUM_OF_FLOOR; ++i) {

                float bx, by, bw, bh;
                GetButtonRect(i, bx, by, bw, bh);

				// Check: did the player click within this button's rect?
                if (IsAreaClickedByMouse(bx, by, bw, bh)) {
                    SelectedSlot = i;                  // Track for red highlight
                    PendingFloor = (s8)ButtonMap[i];   // Track destination

                    LiftPhase = 3;                     // Move to Phase 3: Delay & Open
                    PhaseTimer = 0.0f;                 // Start transit timer

                    // TODO: Add Button Click & Elevator Moving sound here
                    break;
                }
            }
        }
    }
    // =======================================================
    // PHASE 3: DELAY & DOORS OPEN
    // =======================================================
    else if (LiftPhase == 3) {
        // Wait for the short delay sound to finish
        if (PhaseTimer < TRANSIT_TIMER) {
            PhaseTimer += dt;

            // Exactly when the timer hits, change the floor in the background!
            if (PhaseTimer >= TRANSIT_TIMER) {
                if (PendingFloor != -1) {
                    floorNum = PendingFloor;
                }
                // TODO: Add Elevator 'Ding' and Door Opening sound here
            }
        }
        // Delay is finished, open the doors!
        else {
            DoorProgress += dt / LIFT_TIMER;
            if (DoorProgress >= 1.0f) {
                DoorProgress = 1.0f;

                // Lift fully open, reset and hide UI
                LiftActive = false;
                AnomaliesGenerated = false;
                SelectedSlot = -1;
                PendingFloor = -1;
                LiftPhase = 0;
            }
        }
    }
}

/***************************************** DRAW ******************************************/
void Lift_DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor) {

	// Clamp floor to valid range
    if (floor < 0) floor = 0;
    if (floor >= NUM_OF_FLOOR) floor = NUM_OF_FLOOR - 1;

	// For Outside World Tex, Floor 0 and 1 have unique textures, floors 2 and above all share the same one
    int texIndex = floor;
    if (floor == 0) texIndex = 0;
    else if (floor == 1) texIndex = 1;
    else if (floor >= 2) texIndex = 2;

    AEGfxTexture* tex = LiftWallTex[texIndex];

    if (mesh && tex) {
        DrawTextureMesh(mesh, tex, x, y, w, h, 1.0f);
    }
}

void Lift_DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float /*textXoffset*/, float textY) {
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    if (QuadMesh && LiftTex) {
        DrawTextureMesh(QuadMesh, LiftTex, x, y, w, h, 1.0f);
    }

    const float panelWidth = w * 0.40f;
    const float panelHeight = h * 0.10f;
    const float panelX = x;
    const float panelY = textY + 20.0f;
    const float borderPad = 4.0f;

    DrawSquareMesh(squareMesh, panelX, panelY, panelWidth + borderPad, panelHeight + borderPad, COLOR_WHITE);
    DrawSquareMesh(squareMesh, panelX, panelY, panelWidth, panelHeight, COLOR_DARK_GREY);

    if (liftFontId >= 0) {
        char textBuffer[32]{};

        if (floorNum == 0) sprintf_s(textBuffer, "B1");
        else               sprintf_s(textBuffer, "%02d", (int)floorNum);

		// CHAR_NDC is a tweakable value that determines how much horizontal space each character takes up in NDC
        const float CHAR_NDC = 0.018f;
        const int   len = (int)strlen(textBuffer);
		// Calculate the total width of the text in NDC, then divide by 2 to get the half-width for centering
        const float halfTextWidthNDC = (len * CHAR_NDC) * 0.5f;

        const float panelNDC_X = panelX / SCREEN_WIDTH_HALF;
        const float panelNDC_Y = panelY / SCREEN_HEIGHT_HALF;

		// TEXT_CENTER_Y_TWEAK is a small vertical offset to visually center the text within the panel
        const float TEXT_CENTER_Y_TWEAK = -0.025f;
        const float textNDC_X = panelNDC_X - halfTextWidthNDC;
        const float textNDC_Y = panelNDC_Y + TEXT_CENTER_Y_TWEAK;

        const float SCALE = 1.0f;
		// GLOW_O is the distance the glow extends from the text; higher values make a bigger glow
        const float GLOW_O = 0.006f;

        AEGfxPrintWithGlow(liftFontId, textBuffer, textNDC_X, textNDC_Y, SCALE, 1.0f, 0.1f, 0.1f, 1.0f, 0.45f, 0.0f, 0.0f, 0.75f, GLOW_O);
    }
}

void Lift_Draw(AEGfxVertexList* squareMesh) {
    if (!LiftActive) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // --- CALCULATE JITTER FOR TRANSIT PHASE ---
    float jitterX = 0.0f;
    float jitterY = 0.0f;

    // How long to wait after clicking before the elevator starts moving
    const float JITTER_DELAY = 0.6f;

    // Shake ONLY during Phase 3, AFTER the delay, and BEFORE the doors open
    if (LiftPhase == 3 && PhaseTimer >= JITTER_DELAY && PhaseTimer < TRANSIT_TIMER) {
        // Generates a random offset between -3 and +3 pixels
        jitterX = (float)((rand() % 5) - 3);
        jitterY = (float)((rand() % 5) - 3);
    }

    // --- 1) DRAW DOORS ---
    const float doorW = SCREEN_WIDTH_HALF + (0.5f * LIFT_WIDTH);
    const float doorH = (f32)SCREEN_H;

    float gapOffset = DoorProgress * SCREEN_W / 2.0f;

    // Apply jitterX to the base door positions
    float leftDoorX = -450.0f - gapOffset + jitterX;
    float rightDoorX = 450.0f + gapOffset + jitterX;

    if (QuadMesh && LiftDoorTex) {
        // Draw with jitterY
        DrawTextureMesh(QuadMesh, LiftDoorTex, leftDoorX, jitterY, doorW, doorH, 1.0f);
        DrawTextureMesh(QuadMesh, LiftDoorTex, rightDoorX, jitterY, doorW, doorH, 1.0f);
    }

    // --- 2) DRAW INTERACTIVE PANEL (Drawn on top of doors) ---
    // Only drawn when doors are fully closed (Phase 2, and the beginning of Phase 3)
    if (LiftPhase == 2 || (LiftPhase == 3 && PhaseTimer < TRANSIT_TIMER)) {
        if (QuadMesh && LiftPanelTex) {
            // Draw panel background with jitter
            DrawTextureMesh(QuadMesh, LiftPanelTex, jitterX, jitterY, 450.0f, 750.0f, 1.0f);

            for (int i = 0; i < NUM_OF_FLOOR; i++) {
                int floorToDraw = ButtonMap[i];

                if (LiftButtonTex[floorToDraw]) {
                    float drawX, drawY, btnWidth, btnHeight;
                    GetButtonRect(i, drawX, drawY, btnWidth, btnHeight);
                     
                    // Apply jitter to the individual button coordinates
                    drawX += jitterX;
                    drawY += jitterY;

                    // Highlight Red if this button was clicked!
                    u32 bgColor = (i == SelectedSlot) ? 0xFF0000FF : COLOR_LIGHT_GREY;
                    DrawSquareMesh(squareMesh, drawX, drawY, btnWidth + 10.0f, btnHeight + 10.0f, bgColor);

                    DrawTextureMesh(QuadMesh, LiftButtonTex[floorToDraw], drawX, drawY, btnWidth, btnHeight, 1.0f);
                }
            }
        }
    }
}
/**************************************** UNLOAD *****************************************/
void Lift_Unload() {
    UnloadTextureSafe(LiftTex);
    FreeMeshSafe(QuadMesh);

    if (LiftDoorTex) { AEGfxTextureUnload(LiftDoorTex);  LiftDoorTex = nullptr; }
    if (LiftPanelTex) { AEGfxTextureUnload(LiftPanelTex); LiftPanelTex = nullptr; }

    AEGfxDestroyFont(liftFontId);
    liftFontId = -1;

    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        if (LiftWallTex[i]) {
            AEGfxTextureUnload(LiftWallTex[i]);
            LiftWallTex[i] = nullptr;
        }

        if (LiftButtonTex[i]) {
            AEGfxTextureUnload(LiftButtonTex[i]);
            LiftButtonTex[i] = nullptr;
        }
    }
}

/*************************************** ACCESSORS ***************************************/
bool Lift_IsActive() { return LiftActive; }
bool Lift_IsNear() { return NearLift; }
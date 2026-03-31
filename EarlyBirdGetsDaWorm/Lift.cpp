#include "pch.hpp"

// --- Config ---
static constexpr float LIFT_TIMER       = 1.2f;         // Duration of doors sliding open
static constexpr float CLOSE_TIMER      = 0.8f;         // Duration of doors sliding shut
static constexpr float TRANSIT_TIMER    = 2.2f;         // How long lift stays closed while moving
static const char* GLITCH_CHARS         = "0123456789XXYYZZWWM";

// --- Render Data ---
static s8 liftFontId = -1;
static AEGfxVertexList* QuadMesh = nullptr;

static AEGfxTexture* LiftTex = nullptr;
static AEGfxTexture* LiftDoorTex = nullptr;
static AEGfxTexture* LiftPanelTex = nullptr;
static AEGfxTexture* LiftWallTex[NUM_OF_FLOOR]{};
static AEGfxTexture* LiftButtonTex[NUM_OF_FLOOR]{};

// --- Anomaly State ---
static int   ButtonMap[NUM_OF_FLOOR]{};
static int   OcdTarget          = -1;
static float AiwScaleMap[NUM_OF_FLOOR]{};       // Stores random scales for AIW
static bool  AnomaliesGenerated = false;
static bool  SchizoLied         = false;        // Tracks if the elevator has already done the fake-out
static bool  InsomniaFakeOpen   = false;        // Tracks if the door fake-out has happened
static float ParanoiaTimer      = 0.0f;         // Tracks how long the buttons dodge
static int   OcdClickCount      = 0;            // Tracks how many times a button was pressed

// --- General State ---
static bool LiftActive = false;
static bool LiftWasActive = false; // tracks previous frame state so Timer_SetPaused is only called on change
static bool NearLift = false;

// --- State Machine Variables ---
static int   LiftPhase          = 0;            // 1 = Doors Closing, 2 = Panel Active, 3 = Transit & Opening
static float DoorProgress       = 1.0f;         // 0.0 = Fully Closed, 1.0 = Fully Open
static float PhaseTimer         = 0.0f;         // Tracks time for transit delays
static int   SelectedSlot       = -1;           // Tracks which button was clicked for the red highlight
static s8    PendingFloor       = -1;           // The floor the lift is travelling to

/******************************* ANOMALY HOOKS **********************************/

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 1. PARANOIA:     buttons repel away from the mouse cursor for a few seconds after the panel becomes active
// 2. MANIA:        buttons constantly vibrate around their center point while the panel is active
// 3. DEPRESSION:   buttons are all physically lower on the panel, as if weighed down by sadness
// 6. AIW_SYNDROME: size scaling through AiwScaleMap that is randomised by LiftGenerateAnomalies();
// 8. OCD:          slight misalignment of a single button, chosen in LiftGenerateAnomalies() and stored in OcdTarget; 
//                  button also shudders if player tries to click it but hasn't clicked 3 times yet
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

static void LiftAnomaly_ModifyButtonRect(int slotIndex, float& x, float& y, float& w, float& h) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return;

    ILLNESSES illness = Player_GetAnomalyIllness();

    if (illness == ILLNESSES::AIW_SYNDROME) {
        Journal_ReportAnomaly(ANOMALYID::Lift_AIW); // Report the anomaly to the journal

		// --- AIW SYNDROME ANOMALY (RANDOM SCALE) ---
        w *= AiwScaleMap[slotIndex];
        h *= AiwScaleMap[slotIndex];
    }
    else if (illness == ILLNESSES::OCD) { 
		// --- OCD ANOMALY (MISALIGNED BUTTON) ---
        if (slotIndex == OcdTarget) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD); // Report the anomaly to the journal

            x += 8.0f;
            y -= 8.0f;
        }
        // If this is the button the player is trying to click, but haven't clicked 3 times yet
        if (slotIndex == SelectedSlot && OcdClickCount < 3 && OcdClickCount > 0) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD); // Report the anomaly to the journal

            x += (rand() % 5) - 2; // Small OCD nervous shudder
        }
    }
    else if (illness == ILLNESSES::PARANOIA) {        
        // --- PARANOIA ANOMALY (DODGING BUTTONS) ---
        const float MAX_DODGE_TIME = 5.0f; // How many seconds the buttons dodge for

        // Only repel if the timer hasn't finished yet
        if (ParanoiaTimer < MAX_DODGE_TIME) {

            // 1. Get raw mouse pixel coordinates
            s32 mousePixX, mousePixY;
            AEInputGetCursorPosition(&mousePixX, &mousePixY);

            // 2. Convert to UI coordinates
            float mx = (float)mousePixX - SCREEN_WIDTH_HALF;
            float my = SCREEN_HEIGHT_HALF - (float)mousePixY;

            // 3. Calculate distance
            float dx = x - mx;
            float dy = y - my;
            float distance = sqrtf(dx * dx + dy * dy);

            // 4. Repel Math
            const float REPEL_RADIUS = 150.0f;
            if (distance < REPEL_RADIUS && distance > 0.1f) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia); // Report the anomaly to the journal

                // Calculate how much time is left (1.0 at start, fading to 0.0 at end)
                float timeMultiplier = 1.0f - (ParanoiaTimer / MAX_DODGE_TIME);

                // The push strength gets weaker as the timer runs out
                float pushStrength = (REPEL_RADIUS - distance) * 1.5f * timeMultiplier;

                x += (dx / distance) * pushStrength;
                y += (dy / distance) * pushStrength;
            }
        }
    }
    else if (illness == ILLNESSES::MANIA) {
        Journal_ReportAnomaly(ANOMALYID::Lift_Mania); // Report the anomaly to the journal
        
        // --- MANIA ANOMALY (HYPERACTIVE VIBRATION) ---
        // Every button constantly shakes around its center point
        x += (rand() % 7) - 3;
        y += (rand() % 7) - 3;

    }
    else if (illness == ILLNESSES::DEPRESSION) {
        Journal_ReportAnomaly(ANOMALYID::Lift_Depression); // Report the anomaly to the journal
        
        // --- DEPRESSION ANOMALY (SAGGING/HEAVY BUTTONS) ---
        // Push all buttons further down the panel so they look "heavy" and lethargic
        y -= 50.0f;
    }
}

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 5. SCHIZOPHRENIA:once per patient, the elevator lies about where it's taking you by changing the destination 
//                  to a random floor other than the one selected.
// 8. OCD:          for the first few clicks, the elevator refuses to move and makes a higher-pitched "denied" sound effect
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

static bool LiftAnomaly_OnDestinationSelected(s8& pendingFloor) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return true;

    ILLNESSES illness = Player_GetAnomalyIllness();

    // --- SCHIZOPHRENIA ANOMALY (ONE-TIME SCARE) ---
    if (illness == ILLNESSES::SCHIZOPHRENIA && !SchizoLied) {  
        Journal_ReportAnomaly(ANOMALYID::Lift_Schizophrenia); // Report the anomaly to the journal

        // Secretly change the destination to a completely random floor
        s8 fakeFloor;
        do {
            fakeFloor = rand() % NUM_OF_FLOOR;
        } while (fakeFloor == pendingFloor); // Make sure it's actually a wrong floor

        pendingFloor = fakeFloor;
        SchizoLied = true; // Set to true so it NEVER lies again for this patient
    }

    // --- OCD ANOMALY (REPETITIVE CLICKING) ---
    if (illness == ILLNESSES::OCD) {        
        OcdClickCount++;
        if (OcdClickCount < 3) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD); // Report the anomaly to the journal

            AudioManager_PlaySFX(SFX_ELEVATOR_PRESS, 0.5f, 1.2f); // Higher pitched "denied" click
            return false; // DENY the click! Force the panel to stay open!
        }
    }

    return true; // ACCEPT the click! Let the lift move.
}

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 7. INSOMNIA:     once the doors have been open for a few seconds, they suddenly slam shut again, 
//                  forcing the player to wait through the opening animation again.
// 9. SCOTOPHOBIA:  during the transit phase when the doors are closed, the screen goes pitch black 
//                  so you can't see the floor numbers changing.
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

static void LiftAnomaly_OnDraw(AEGfxVertexList* squareMesh) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return;

    ILLNESSES illness = Player_GetAnomalyIllness();

    // --- INSOMNIA ANOMALY (FAKE-OUT DOORS) ---
    if (illness == ILLNESSES::INSOMNIA && !InsomniaFakeOpen) {
        // If doors are starting to open (Phase 3, after delay)
        if (LiftPhase == 3 && PhaseTimer >= TRANSIT_TIMER && DoorProgress > 0.4f) {
            Journal_ReportAnomaly(ANOMALYID::Lift_Insomnia); // Report the anomaly to the journal

            DoorProgress = 0.0f;     // SLAM SHUT
            
            // Wind back the transit timer, this forces the elevator to wait 1.5 seconds in silence 
            // before it is allowed to start opening again!
            PhaseTimer = TRANSIT_TIMER - 1.5f; 
            
            InsomniaFakeOpen = true; // Only do it once
            
            // (Highly recommended: Play a loud metal bang here so the slam has impact!)
            // AudioManager_PlaySFX(SFX_METAL_BANG, 0.8f);
        }
    }

    // --- SCOTOPHOBIA ANOMALY (PITCH BLACK TRANSIT) ---
    if (illness == ILLNESSES::SCOTOPHOBIA) {
        if (LiftPhase == 3 && PhaseTimer < TRANSIT_TIMER) {
            Journal_ReportAnomaly(ANOMALYID::Lift_Scotophobia); // Report the anomaly to the journal

            DrawSquareMesh(squareMesh, 0.0f, 0.0f, SCREEN_W * 2.0f, SCREEN_H * 2.0f, COLOR_BLACK);
        }
    }
}

/******************************* LIFT LOGIC **********************************/

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
    LiftAnomaly_ModifyButtonRect(slotIndex, x, y, w, h);
}

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 4. Dementia:     the button mapping is completely shuffled, but you will still go to 
//                  the floor that the button indicates (just a visual trick).
// 6. AIW Syndrome: randomly pick several buttons to be warped by different amounts, either tiny or 
//                  huge, stored in AiwScaleMap.
// 8. OCD:          choose the button that will be the "target" (stored in OcdTarget) and make it 
//                  slightly misaligned, so it stands out as "off" compared to the others.
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

static void Lift_GenerateAnomalies() {
    // Normal Mapping; 0-9 in order followring B1 to F9
    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        ButtonMap[i] = i;
        AiwScaleMap[i] = 1.0f; // Reset scales to normal
    }
    OcdTarget = -1;

    // Reset the Schizophrenia flag if the current patient DOESN'T have it
    if (Player_GetAnomalyIllness() != ILLNESSES::SCHIZOPHRENIA) {
        SchizoLied = false;
    }

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
        else if (illness == ILLNESSES::AIW_SYNDROME) {
            // Pick between 3 and 5 random buttons to warp
            int numToWarp = (rand() % 3) + 3;
            for (int i = 0; i < numToWarp; i++) {
                int randomSlot = rand() % NUM_OF_FLOOR;
                // 50/50 chance to be tiny (0.5x to 0.7x) or massive (1.3x to 1.6x)
                if (rand() % 2 == 0) {
                    AiwScaleMap[randomSlot] = 0.5f + ((float)(rand() % 21) / 100.0f);
                }
                else {
                    AiwScaleMap[randomSlot] = 1.3f + ((float)(rand() % 31) / 100.0f);
                }
            }
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
    ParanoiaTimer = 0.0f;

    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        AiwScaleMap[i] = 1.0f;
    }
}

/***************************************** UPDATE ****************************************/
void Lift_Update(float /*dt*/, float camX, float maxDist, bool dementiaActive) {

    const bool nearLeftLift  = (camX > -5.0f);
    const bool nearRightLift = !dementiaActive && (camX < -(maxDist - 100.0f));
    NearLift = (nearLeftLift || nearRightLift);

    if (NearLift && AEInputCheckTriggered(AEVK_E)) {
        if (!LiftActive) {

            AudioManager_StopSFX();
            LiftActive = true;
            LiftPhase = 1;          // Start Phase 1: Closing
            DoorProgress = 1.0f;    // Start fully open
            SelectedSlot = -1;
            PendingFloor = -1;
            AudioManager_PlaySFX(SFX_ELEVATOR_CLOSE, 0.5f);

            if (!AnomaliesGenerated) {
                Lift_GenerateAnomalies();
            }
        }
        else if (LiftPhase == 2) {
            // If L is pressed while the panel is active, cancel and open doors
            LiftPhase = 3;
            PhaseTimer = TRANSIT_TIMER;     // Skip delay, go straight to opening
            AudioManager_PlaySFX(SFX_ELEVATOR_CLOSE, 0.5f);
            PendingFloor = -1;              // Don't change floors
        }
    }

    // Auto-close if the player walks away while panel is open
    if (!NearLift && LiftActive && LiftPhase == 2) {
        LiftPhase = 3;
        PhaseTimer = TRANSIT_TIMER;
        PendingFloor = -1;
    }

    // Only call Timer_SetPaused when LiftActive actually changes state,
    // so it doesn't constantly override the F4 debug pause every frame.
    if (LiftActive != LiftWasActive) {
        Timer_SetPaused(LiftActive);
        LiftWasActive = LiftActive;
    }
}

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 1. PARANOIA:     buttons repel away from the mouse cursor for a few seconds after the panel becomes active
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

void Lift_HandleInput(s8& floorNum) {
    if (!LiftActive) return;

    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // --- PHASE 1: DOORS CLOSING ---
    if (LiftPhase == 1) {
        // Decrement door progress based on timer; 
        // When it hits 0, doors are fully closed, switching to the panel phase
        DoorProgress -= dt / CLOSE_TIMER;
        if (DoorProgress <= 0.0f) {
            DoorProgress = 0.0f;
            LiftPhase = 2;
        }
    }

    // --- PHASE 2: PANEL APPEARS (AWAITING PLAYER INPUT) ---
    else if (LiftPhase == 2) {

        //if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)
        //    && Player_GetAnomalyIllness() == ILLNESSES::PARANOIA) {
        //    Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia); // Report the anomaly to the journal

        //    ParanoiaTimer += dt;
        //}

        if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
            ILLNESSES illness = Player_GetAnomalyIllness();

            if (illness == ILLNESSES::PARANOIA) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia); // Report the anomaly to the journal
                ParanoiaTimer += dt; 
            }

            if (illness == ILLNESSES::DEMENTIA) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Dementia); // Report the anomaly to the journal
            }
        }

        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            for (int i = 0; i < NUM_OF_FLOOR; ++i) {

                float bx, by, bw, bh;
                GetButtonRect(i, bx, by, bw, bh);

                // Check: did the player click within this button's rect?
                if (IsAreaClickedByMouse(bx, by, bw, bh)) {
                    SelectedSlot = i;                  // Track for red highlight
                    PendingFloor = (s8)ButtonMap[i];   // Track destination

                    // Ask the anomaly system if it wants to hijack the floor
                    // If the hook returns false, abort the movement!
                    if (!LiftAnomaly_OnDestinationSelected(PendingFloor)) {
                        break;
                    }

                    LiftPhase = 3;                     // Move to Phase 3: Delay & Open

                    // --- SAME FLOOR CHECK ---
                    if (PendingFloor == floorNum) {
                        // Fast-forward the timer to skip the transit delay and instantly open the doors.
                        AudioManager_PlaySFX(SFX_ELEVATOR_CLOSE, 0.5f);
                        PhaseTimer = TRANSIT_TIMER;
                    }
                    else {
                        // Normal transit to a new floor
                        PhaseTimer = 0.0f;
                        ParanoiaTimer = 0.0f;
                        AudioManager_PlaySFX(SFX_ELEVATOR_PRESS, 0.5f);
                    }

                    break;
                }
            }
        }
    }

    // --- PHASE 3: DELAY & DOORS OPEN ---
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

                ParanoiaTimer = 0.0f;
                OcdClickCount = 0;
                InsomniaFakeOpen = false;

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
    int TexIndex = floor;
    if (floor == 0) TexIndex = 0;
    else if (floor == 1) TexIndex = 1;
    else if (floor >= 2) TexIndex = 2;

    AEGfxTexture* tex = LiftWallTex[TexIndex];

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

    LiftAnomaly_OnDraw(squareMesh);
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

/*************************************** ACCESSORS *****s**********************************/
bool Lift_IsActive() { return LiftActive; }
bool Lift_IsNear() { return NearLift; }
bool Lift_IsFloorPressed() {
    return (SelectedSlot != -1);
}
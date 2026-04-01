// ============================================================
// LIFT MODULE (class-based singleton)
// ============================================================

#include "pch.hpp"

// ============================================================
// FILE-SCOPE CONSTANTS
// ============================================================

static constexpr float LIFT_TIMER       = 1.2f;         // Duration of doors sliding open
static constexpr float CLOSE_TIMER      = 0.8f;         // Duration of doors sliding shut
static constexpr float TRANSIT_TIMER    = 2.2f;         // How long lift stays closed while moving
static const char* GLITCH_CHARS         = "0123456789XXYYZZWWM";

// ============================================================
// LIFT CLASS
// ============================================================

class Lift {
public:
    // Nested structs
    struct Textures {
        AEGfxTexture* bg      = nullptr;
        AEGfxTexture* door    = nullptr;
        AEGfxTexture* panel   = nullptr;
        AEGfxTexture* screen = nullptr;
        AEGfxTexture* wall[NUM_OF_FLOOR]{};
        AEGfxTexture* button[NUM_OF_FLOOR]{};
    };

    struct PhaseState {
        int   phase          = 0;            // 1 = Doors Closing, 2 = Panel Active, 3 = Transit & Opening
        float timer          = 0.0f;         // Tracks time for transit delays
        float doorProgress   = 1.0f;         // 0.0 = Fully Closed, 1.0 = Fully Open
        s8    pendingFloor   = -1;           // The floor the lift is travelling to
        int   selectedSlot   = -1;           // Tracks which button was clicked for the red highlight
    };

    struct AnomalyState {
        int   buttonMap[NUM_OF_FLOOR]{};
        float aiwScale[NUM_OF_FLOOR]{};      // Stores random scales for AIW
        bool  generated      = false;
        bool  schizoLied     = false;        // Tracks if the elevator has already done the fake-out
        bool  insomniaFake   = false;        // Tracks if the door fake-out has happened
        float paranoiaTimer  = 0.0f;         // Tracks how long the buttons dodge
        int   ocdTarget      = -1;
        int   ocdClicks      = 0;            // Tracks how many times a button was pressed
    };

    // Render data
    s8                 fontId   = -1;
    AEGfxVertexList*   quadMesh = nullptr;
    Textures           tex;

    // State
    PhaseState         phase;
    AnomalyState       anomaly;

    bool               liftActive    = false;
    bool               liftWasActive = false;  // tracks previous frame state so Timer_SetPaused is only called on change
    bool               nearLift      = false;

public:
    void Load();
    void Initialize();
    void Update(float dt, float camX, float maxDist, bool dementiaActive);
    void HandleInput(s8& floorNum);
    void DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor);
    void DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float textXoffset, float textY);
    void Draw(AEGfxVertexList* squareMesh, s8 currentFloor); 
    void Unload();
    bool IsActive();
    bool IsNear();
    bool IsFloorPressed();

private:
    void GenerateAnomalies();
    void GetButtonRect(int slotIndex, float& x, float& y, float& w, float& h);
    void Anomaly_ModifyButtonRect(int slotIndex, float& x, float& y, float& w, float& h);
    bool Anomaly_OnDestinationSelected(s8& pendingFloor);
    void Anomaly_OnDraw(AEGfxVertexList* squareMesh);
};

// ============================================================
// SINGLETON + WRAPPERS
// ============================================================

static Lift TheLift;

void Lift_Load()                                                                          { TheLift.Load(); }
void Lift_Initialize()                                                                    { TheLift.Initialize(); }
void Lift_Update(float dt, float camX, float maxDist, bool dementiaActive)                { TheLift.Update(dt, camX, maxDist, dementiaActive); }
void Lift_HandleInput(s8& floorNum)                                                       { TheLift.HandleInput(floorNum); }
void Lift_DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor) { TheLift.DrawBackground(mesh, x, y, w, h, floor); }
void Lift_DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float textXoffset, float textY) { TheLift.DrawWorld(squareMesh, x, y, w, h, floorNum, textXoffset, textY); }
void Lift_Draw(AEGfxVertexList* squareMesh, s8 currentFloor) { TheLift.Draw(squareMesh, currentFloor); } // <-- Add currentFloor here
void Lift_Unload()                                                                        { TheLift.Unload(); }
bool Lift_IsActive()                                                                      { return TheLift.IsActive(); }
bool Lift_IsNear()                                                                        { return TheLift.IsNear(); }
bool Lift_IsFloorPressed()                                                                { return TheLift.IsFloorPressed(); }

/******************************* ANOMALY HOOKS **********************************/

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 1. PARANOIA:     buttons repel away from the mouse cursor for a few seconds after the panel becomes active
// 2. MANIA:        buttons constantly vibrate around their center point while the panel is active
// 3. DEPRESSION:   buttons are all physically lower on the panel, as if weighed down by sadness
// 6. AIW_SYNDROME: size scaling through AiwScaleMap that is randomised by GenerateAnomalies();
// 8. OCD:          slight misalignment of a single button, chosen in GenerateAnomalies() and stored in OcdTarget;
//                  button also shudders if player tries to click it but hasn't clicked 3 times yet
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

void Lift::Anomaly_ModifyButtonRect(int slotIndex, float& x, float& y, float& w, float& h) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return;

    ILLNESSES illness = Player_GetAnomalyIllness();

    if (illness == ILLNESSES::AIW_SYNDROME) {
        Journal_ReportAnomaly(ANOMALYID::Lift_AIW); // Report the anomaly to the journal

		// --- AIW SYNDROME ANOMALY (RANDOM SCALE) ---
        w *= anomaly.aiwScale[slotIndex];
        h *= anomaly.aiwScale[slotIndex];
    }
    else if (illness == ILLNESSES::OCD) {
		// --- OCD ANOMALY (MISALIGNED BUTTON) ---
        if (slotIndex == anomaly.ocdTarget) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD); // Report the anomaly to the journal

            x += 8.0f;
            y -= 8.0f;
        }
        // If this is the button the player is trying to click, but haven't clicked 3 times yet
        if (slotIndex == phase.selectedSlot && anomaly.ocdClicks < 3 && anomaly.ocdClicks > 0) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD); // Report the anomaly to the journal

            x += RandInt(-2, 2); // Small OCD nervous shudder
        }
    }
    else if (illness == ILLNESSES::PARANOIA) {
        // --- PARANOIA ANOMALY (DODGING BUTTONS) ---
        const float MAX_DODGE_TIME = 5.0f; // How many seconds the buttons dodge for

        // Only repel if the timer hasn't finished yet
        if (anomaly.paranoiaTimer < MAX_DODGE_TIME) {

            // 1. Get raw mouse pixel coordinates
            s32 mousePixX, mousePixY;
            AEInputGetCursorPosition(&mousePixX, &mousePixY);

            // 2. Convert to UI coordinates
            float mx = static_cast<float>(mousePixX) - SCREEN_WIDTH_HALF;
            float my = SCREEN_HEIGHT_HALF - static_cast<float>(mousePixY);

            // 3. Calculate distance
            float dx = x - mx;
            float dy = y - my;
            float distance = sqrtf(dx * dx + dy * dy);

            // 4. Repel Math
            const float REPEL_RADIUS = 150.0f;
            if (distance < REPEL_RADIUS && distance > 0.1f) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia); // Report the anomaly to the journal

                // Calculate how much time is left (1.0 at start, fading to 0.0 at end)
                float timeMultiplier = 1.0f - (anomaly.paranoiaTimer / MAX_DODGE_TIME);

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
        x += RandInt(-3, 3);
        y += RandInt(-3, 3);

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

bool Lift::Anomaly_OnDestinationSelected(s8& pendingFloor) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return true;

    ILLNESSES illness = Player_GetAnomalyIllness();

    // --- SCHIZOPHRENIA ANOMALY (ONE-TIME SCARE) ---
    if (illness == ILLNESSES::SCHIZOPHRENIA && !anomaly.schizoLied) {
        Journal_ReportAnomaly(ANOMALYID::Lift_Schizophrenia); // Report the anomaly to the journal

        // Secretly change the destination to a completely random floor
        s8 fakeFloor;
        do {
            fakeFloor = RandInt(0, NUM_OF_FLOOR - 1);
        } while (fakeFloor == pendingFloor); // Make sure it's actually a wrong floor

        pendingFloor = fakeFloor;
        anomaly.schizoLied = true; // Set to true so it NEVER lies again for this patient
    }

    // --- OCD ANOMALY (REPETITIVE CLICKING) ---
    if (illness == ILLNESSES::OCD) {
        anomaly.ocdClicks++;
        if (anomaly.ocdClicks < 3) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD); // Report the anomaly to the journal

            AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_PRESS, 0.5f, 1.2f); // Higher pitched "denied" click
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

void Lift::Anomaly_OnDraw(AEGfxVertexList* squareMesh) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return;

    ILLNESSES illness = Player_GetAnomalyIllness();

    // --- INSOMNIA ANOMALY (FAKE-OUT DOORS) ---
    if (illness == ILLNESSES::INSOMNIA && !anomaly.insomniaFake) {
        // If doors are starting to open (Phase 3, after delay)
        if (phase.phase == 3 && phase.timer >= TRANSIT_TIMER && phase.doorProgress > 0.4f) {
            Journal_ReportAnomaly(ANOMALYID::Lift_Insomnia); // Report the anomaly to the journal

            phase.doorProgress = 0.0f;     // SLAM SHUT

            // Wind back the transit timer, this forces the elevator to wait 1.5 seconds in silence
            // before it is allowed to start opening again!
            phase.timer = TRANSIT_TIMER - 1.5f;

            anomaly.insomniaFake = true; // Only do it once

            // (Highly recommended: Play a loud metal bang here so the slam has impact!)
            // AudioManager_PlaySFX(SFX_METAL_BANG, 0.8f);
        }
    }

    // --- SCOTOPHOBIA ANOMALY (PITCH BLACK TRANSIT) ---
    if (illness == ILLNESSES::SCOTOPHOBIA) {
        if (phase.phase == 3 && phase.timer < TRANSIT_TIMER) {
            Journal_ReportAnomaly(ANOMALYID::Lift_Scotophobia); // Report the anomaly to the journal

            DrawSquareMesh(squareMesh, 0.0f, 0.0f, SCREEN_W * 2.0f, SCREEN_H * 2.0f, COLOR_BLACK);
        }
    }
}

/******************************* LIFT LOGIC **********************************/

void Lift::GetButtonRect(int slotIndex, float& x, float& y, float& w, float& h) {
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
    Anomaly_ModifyButtonRect(slotIndex, x, y, w, h);
}

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 4. Dementia:     the button mapping is completely shuffled, but you will still go to
//                  the floor that the button indicates (just a visual trick).
// 6. AIW Syndrome: randomly pick several buttons to be warped by different amounts, either tiny or
//                  huge, stored in AiwScaleMap.
// 8. OCD:          choose the button that will be the "target" (stored in OcdTarget) and make it
//                  slightly misaligned, so it stands out as "off" compared to the others.
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

void Lift::GenerateAnomalies() {
    // Normal Mapping; 0-9 in order followring B1 to F9
    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        anomaly.buttonMap[i] = i;
        anomaly.aiwScale[i] = 1.0f; // Reset scales to normal
    }
    anomaly.ocdTarget = -1;

    // Reset the Schizophrenia flag if the current patient DOESN'T have it
    if (Player_GetAnomalyIllness() != ILLNESSES::SCHIZOPHRENIA) {
        anomaly.schizoLied = false;
    }

    // Check: lift anomalies are active?
    if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
        ILLNESSES illness = Player_GetAnomalyIllness();

        if (illness == ILLNESSES::DEMENTIA) {
            // Shuffle the button mapping so the floors are all mixed up
            std::shuffle(std::begin(anomaly.buttonMap), std::end(anomaly.buttonMap), GetGlobalRng());
        }
        else if (illness == ILLNESSES::OCD) {
            // Pick one random button to be the OCD "target" that is slightly misaligned
            anomaly.ocdTarget = RandInt(0, NUM_OF_FLOOR - 1);
        }
        else if (illness == ILLNESSES::AIW_SYNDROME) {
            // Pick between 3 and 5 random buttons to warp
            int numToWarp = RandInt(3, 5);
            for (int i = 0; i < numToWarp; i++) {
                int randomSlot = RandInt(0, NUM_OF_FLOOR - 1);
                // 50/50 chance to be tiny (0.5x to 0.7x) or massive (1.3x to 1.6x)
                if (RandInt(0, 1) == 0) {
                    anomaly.aiwScale[randomSlot] = 0.5f + (static_cast<float>(RandInt(0, 20)) / 100.0f);
                }
                else {
                    anomaly.aiwScale[randomSlot] = 1.3f + (static_cast<float>(RandInt(0, 30)) / 100.0f);
                }
            }
        }
    }

    anomaly.generated = true;
}

// ============================================================
// LOAD
// ============================================================

void Lift::Load() {
    fontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    tex.bg = LoadTextureChecked(Assets::Lift::LiftBg);
    tex.door = LoadTextureChecked(Assets::Lift::LiftDoor);
    tex.panel = LoadTextureChecked(Assets::Lift::LiftPanel);
	tex.screen = LoadTextureChecked(Assets::Lift::LiftScreen);

    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        tex.button[i] = AEGfxTextureLoad(Assets::Lift::LiftButton[i]);
    }

    for (int i = 0; i < 3; i++) {
        tex.wall[i] = AEGfxTextureLoad(Assets::Lift::OutsideWorld[i]);
    }
}

// ============================================================
// INITIALIZE
// ============================================================

void Lift::Initialize() {
    quadMesh = CreateSquareMesh(0xFFFFFFFF);

    liftActive = false;
    nearLift = false;
    anomaly.generated = false;

    phase.phase = 0;
    phase.doorProgress = 1.0f;
    phase.timer = 0.0f;
    phase.selectedSlot = -1;
    phase.pendingFloor = -1;
    anomaly.paranoiaTimer = 0.0f;

    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        anomaly.aiwScale[i] = 1.0f;
    }
}

// ============================================================
// UPDATE
// ============================================================

void Lift::Update(float /*dt*/, float camX, float maxDist, bool dementiaActive) {

    const bool nearLeftLift  = (camX > -5.0f);
    const bool nearRightLift = !dementiaActive && (camX < -(maxDist - 100.0f));
    nearLift = (nearLeftLift || nearRightLift);

    if (nearLift && AEInputCheckTriggered(AEVK_E)) {
        if (!liftActive) {

            AudioManager_StopSFX();
            liftActive = true;
            phase.phase = 1;          // Start Phase 1: Closing
            phase.doorProgress = 1.0f;    // Start fully open
            phase.selectedSlot = -1;
            phase.pendingFloor = -1;
            AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_CLOSE, 0.5f);

            if (!anomaly.generated) {
                GenerateAnomalies();
            }
        }
        else if (phase.phase == 2) {
            // If E is pressed while the panel is active, cancel and open doors
            phase.phase = 3;
            phase.timer = TRANSIT_TIMER;     // Skip delay, go straight to opening
            AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_CLOSE, 0.5f);
            phase.pendingFloor = -1;              // Don't change floors
        }
    }

    // Auto-close if the player walks away while panel is open
    if (!nearLift && liftActive && phase.phase == 2) {
        phase.phase = 3;
        phase.timer = TRANSIT_TIMER;
        phase.pendingFloor = -1;
    }

    // Only call Timer_SetPaused when liftActive actually changes state,
    // so it doesn't constantly override the F4 debug pause every frame.
    if (liftActive != liftWasActive) {
        Timer_SetPaused(liftActive);
        liftWasActive = liftActive;
    }
}

// ============================================================
// HANDLE INPUT
// ============================================================

/********************************************   ANOMALY DESCRIPTIONS   *******************************************/
// 1. PARANOIA:     buttons repel away from the mouse cursor for a few seconds after the panel becomes active
/********************************************   ANOMALY DESCRIPTIONS   *******************************************/

void Lift::HandleInput(s8& floorNum) {
    if (!liftActive) return;

    float dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());

    // --- PHASE 1: DOORS CLOSING ---
    if (phase.phase == 1) {
        // Decrement door progress based on timer;
        // When it hits 0, doors are fully closed, switching to the panel phase
        phase.doorProgress -= dt / CLOSE_TIMER;
        if (phase.doorProgress <= 0.0f) {
            phase.doorProgress = 0.0f;
            phase.phase = 2;
        }
    }

    // --- PHASE 2: PANEL APPEARS (AWAITING PLAYER INPUT) ---
    else if (phase.phase == 2) {

        if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
            ILLNESSES illness = Player_GetAnomalyIllness();

            if (illness == ILLNESSES::PARANOIA) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia); // Report the anomaly to the journal
                anomaly.paranoiaTimer += dt;
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
                    phase.selectedSlot = i;                  // Track for red highlight
                    phase.pendingFloor = static_cast<s8>(anomaly.buttonMap[i]);   // Track destination

                    // Ask the anomaly system if it wants to hijack the floor
                    // If the hook returns false, abort the movement!
                    if (!Anomaly_OnDestinationSelected(phase.pendingFloor)) {
                        break;
                    }

                    phase.phase = 3;                     // Move to Phase 3: Delay & Open

                    // --- SAME FLOOR CHECK ---
                    if (phase.pendingFloor == floorNum) {
                        // Fast-forward the timer to skip the transit delay and instantly open the doors.
                        AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_CLOSE, 0.5f);
                        phase.timer = TRANSIT_TIMER;
                    }
                    else {
                        // Normal transit to a new floor
                        phase.timer = 0.0f;
                        anomaly.paranoiaTimer = 0.0f;
                        AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_PRESS, 0.5f);
                    }

                    break;
                }
            }
        }
    }

    // --- PHASE 3: DELAY & DOORS OPEN ---
    else if (phase.phase == 3) {
        // Wait for the short delay sound to finish
        if (phase.timer < TRANSIT_TIMER) {
            phase.timer += dt;

            // Exactly when the timer hits, change the floor in the background!
            if (phase.timer >= TRANSIT_TIMER) {
                if (phase.pendingFloor != -1) {
                    floorNum = phase.pendingFloor;
                }
                // TODO: Add Elevator 'Ding' and Door Opening sound here
            }
        }
        // Delay is finished, open the doors!
        else {
            phase.doorProgress += dt / LIFT_TIMER;
            if (phase.doorProgress >= 1.0f) {
                phase.doorProgress = 1.0f;

                anomaly.paranoiaTimer = 0.0f;
                anomaly.ocdClicks = 0;
                anomaly.insomniaFake = false;

                // Lift fully open, reset and hide UI
                liftActive = false;
                anomaly.generated = false;
                phase.selectedSlot = -1;
                phase.pendingFloor = -1;
                phase.phase = 0;
            }
        }
    }
}

// ============================================================
// DRAW
// ============================================================

void Lift::DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor) {

    // Clamp floor to valid range
    if (floor < 0) floor = 0;
    if (floor >= NUM_OF_FLOOR) floor = NUM_OF_FLOOR - 1;

    // For Outside World Tex, Floor 0 and 1 have unique textures, floors 2 and above all share the same one
    int TexIndex = floor;
    if (floor == 0) TexIndex = 0;
    else if (floor == 1) TexIndex = 1;
    else if (floor >= 2) TexIndex = 2;

    AEGfxTexture* t = tex.wall[TexIndex];

    if (mesh && t) {
        DrawTextureMesh(mesh, t, x, y, w, h, 1.0f);
    }
}

void Lift::DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float /*textXoffset*/, float textY) {
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    if (quadMesh && tex.bg) {
        DrawTextureMesh(quadMesh, tex.bg, x, y, w, h, 1.0f);
    }

    const float panelWidth = w * 0.40f;
    const float panelHeight = h * 0.10f;
    const float panelX = x;
    const float panelY = textY + 20.0f;
    const float borderPad = 4.0f;

    DrawSquareMesh(squareMesh, panelX, panelY, panelWidth + borderPad, panelHeight + borderPad, COLOR_WHITE);
    DrawSquareMesh(squareMesh, panelX, panelY, panelWidth, panelHeight, COLOR_DARK_GREY);

    if (fontId >= 0) {
        std::ostringstream oss;

        if (floorNum == 0) oss << "B1";
        else               oss << std::setw(2) << std::setfill('0') << static_cast<int>(floorNum);

        std::string textBuffer = oss.str();

        // CHAR_NDC is a tweakable value that determines how much horizontal space each character takes up in NDC
        const float CHAR_NDC = 0.018f;
        const int   len = static_cast<int>(textBuffer.size());
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

        AEGfxPrintWithGlow(fontId, textBuffer.c_str(), textNDC_X, textNDC_Y, SCALE, 1.0f, 0.1f, 0.1f, 1.0f, 0.45f, 0.0f, 0.0f, 0.75f, GLOW_O);
    }
}

void Lift::Draw(AEGfxVertexList* squareMesh, s8 currentFloor) { // <-- Added currentFloor
    if (!liftActive) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // --- CALCULATE JITTER FOR TRANSIT PHASE ---
    float jitterX = 0.0f;
    float jitterY = 0.0f;

    const float JITTER_DELAY = 0.6f;

    if (phase.phase == 3 && phase.timer >= JITTER_DELAY && phase.timer < TRANSIT_TIMER) {
        jitterX = static_cast<float>(RandInt(-3, 1));
        jitterY = static_cast<float>(RandInt(-3, 1));
    }

    // --- 1) DRAW DOORS ---
    const float doorW = SCREEN_WIDTH_HALF + (0.5f * LIFT_WIDTH);
    const float doorH = static_cast<f32>(SCREEN_H);

    float gapOffset = phase.doorProgress * SCREEN_W / 2.0f;

    float leftDoorX = -450.0f - gapOffset + jitterX;
    float rightDoorX = 450.0f + gapOffset + jitterX;

    if (quadMesh && tex.door) {
        DrawTextureMesh(quadMesh, tex.door, leftDoorX, jitterY, doorW, doorH, 1.0f);
        DrawTextureMesh(quadMesh, tex.door, rightDoorX, jitterY, doorW, doorH, 1.0f);
    }

    // --- 2) DRAW INTERACTIVE PANEL & SIDE SCREEN ---
    if (phase.phase == 2 || (phase.phase == 3 && phase.timer < TRANSIT_TIMER)) {
        if (quadMesh && tex.panel) {

            // Draw main panel background with jitter
            DrawTextureMesh(quadMesh, tex.panel, jitterX, jitterY, 450.0f, 750.0f, 1.0f);

            // Draw side screen background
            float screenX = jitterX + 300.0f;
            DrawTextureMesh(quadMesh, tex.screen, screenX, jitterY, 100.0f, 750.0f, 1.0f);

            // ========================================================
            // VERTICAL FLOOR INDICATOR STRIP
            // ========================================================
            float startY = 300.0f;          // Highest point on the screen (Floor 9)
            float ySpacing = 600.0f / 9.0f; // Distributes 10 floors evenly down the screen

            // --- CALCULATE DYNAMIC GLOWING FLOOR ---
            int glowingFloor = currentFloor;

            // If in the transit phase and actually moving to a new floor
            if (phase.phase == 3 && phase.pendingFloor != -1 && phase.timer < TRANSIT_TIMER) {
                // Get progress from 0.0 to 1.0
                float progress = phase.timer / TRANSIT_TIMER;
                float distance = static_cast<float>(phase.pendingFloor - currentFloor);

                float offset = distance * progress;
                if (offset > 0) offset += 0.5f;
                else            offset -= 0.5f;

                glowingFloor = currentFloor + static_cast<int>(offset);
            }

            for (int i = 9; i >= 0; --i) {
                float drawY = jitterY + startY - ((9 - i) * ySpacing);

                if (fontId >= 0) {
                    std::ostringstream oss;
                    if (i == 0) oss << "B1";
                    else        oss << i;
                    std::string text = oss.str();

                    const float CHAR_NDC = 0.018f;
                    const float halfTextWidthNDC = (text.length() * CHAR_NDC) * 0.5f;

                    float ndcX = screenX / SCREEN_WIDTH_HALF - halfTextWidthNDC;
                    float ndcY = drawY / SCREEN_HEIGHT_HALF - 0.020f; // Visual center tweak

                    // Check against glowingFloor instead of currentFloor!
                    if (i == glowingFloor) {
                        // Glowing Red Text for the active moving floor
                        AEGfxPrintWithGlow(fontId, text.c_str(), ndcX, ndcY, 1.0f, 1.0f, 0.1f, 0.1f, 1.0f, 0.6f, 0.0f, 0.0f, 0.8f, 0.006f);
                    }
                    else {
                        // Dim Grey Text for inactive floors
                        AEGfxPrintWithGlow(fontId, text.c_str(), ndcX, ndcY, 1.0f, 0.4f, 0.4f, 0.4f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                    }
                }
            }
            // ========================================================

            // Draw clickable Buttons on the main panel
            for (int i = 0; i < NUM_OF_FLOOR; i++) {
                int floorToDraw = anomaly.buttonMap[i];

                if (tex.button[floorToDraw]) {
                    float drawX, drawY, btnWidth, btnHeight;
                    GetButtonRect(i, drawX, drawY, btnWidth, btnHeight);

                    drawX += jitterX;
                    drawY += jitterY;

                    u32 bgColor = (i == phase.selectedSlot) ? 0xFF0000FF : COLOR_LIGHT_GREY;
                    DrawSquareMesh(squareMesh, drawX, drawY, btnWidth + 10.0f, btnHeight + 10.0f, bgColor);
                    DrawTextureMesh(quadMesh, tex.button[floorToDraw], drawX, drawY, btnWidth, btnHeight, 1.0f);
                }
            }
        }
    }

    Anomaly_OnDraw(squareMesh);
}
// ============================================================
// UNLOAD
// ============================================================

void Lift::Unload() {
    UnloadTextureSafe(tex.bg);
    FreeMeshSafe(quadMesh);

    if (tex.door) { AEGfxTextureUnload(tex.door);  tex.door = nullptr; }
    if (tex.panel) { AEGfxTextureUnload(tex.panel); tex.panel = nullptr; }
    if (tex.screen) { AEGfxTextureUnload(tex.screen); tex.screen = nullptr; }

    AEGfxDestroyFont(fontId);
    fontId = -1;

    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        if (tex.wall[i]) {
            AEGfxTextureUnload(tex.wall[i]);
            tex.wall[i] = nullptr;
        }

        if (tex.button[i]) {
            AEGfxTextureUnload(tex.button[i]);
            tex.button[i] = nullptr;
        }
    }
}

// ============================================================
// ACCESSORS
// ============================================================

bool Lift::IsActive() { return liftActive; }
bool Lift::IsNear() { return nearLift; }
bool Lift::IsFloorPressed() {
    return (phase.selectedSlot != -1);
}

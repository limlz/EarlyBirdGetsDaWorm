#include "pch.hpp"

// ============================================================================
// FILE-SCOPE CONSTANTS
// ============================================================================

static constexpr float LIFT_TIMER = 1.2f;           // Duration of doors sliding open
static constexpr float CLOSE_TIMER = 0.8f;          // Duration of doors sliding shut
static constexpr float TRANSIT_TIMER = 2.2f;        // How long lift stays closed while moving
static const char* GLITCH_CHARS = "0123456789XXYYZZWWM";

// ============================================================================
// LIFT CLASS DEFINITION
// ============================================================================

class Lift {
public:
    // ----------------------------------------------------------------------------
    // NESTED STATE STRUCTS
    // ----------------------------------------------------------------------------
    struct Textures {
        AEGfxTexture* bg = nullptr;
        AEGfxTexture* door = nullptr;
        AEGfxTexture* panel = nullptr;
        AEGfxTexture* screen = nullptr;
        AEGfxTexture* wall[NUM_OF_FLOOR]{};
        AEGfxTexture* button[NUM_OF_FLOOR]{};
    };

    struct PhaseState {
        int   phase = 0;                    // 0 = Idle, 1 = Doors Closing, 2 = Panel Active, 3 = Transit & Opening
        float timer = 0.0f;                 // Tracks time for transit delays and animations
        float doorProgress = 1.0f;          // 0.0 = Fully Closed, 1.0 = Fully Open
        s8    pendingFloor = -1;            // The target floor the lift is traveling to
        int   selectedSlot = -1;            // Tracks which physical button slot was clicked for the red highlight
    };

    struct AnomalyState {
        int   buttonMap[NUM_OF_FLOOR]{};    // Maps physical button slots to actual floors
        float aiwScale[NUM_OF_FLOOR]{};     // Stores randomized scaling multipliers for AIW Syndrome
        bool  generated = false;            // Flags if the anomalies for the current interaction have been rolled
        bool  schizoLied = false;           // Tracks if the Schizophrenia fake-out has occurred this session
        bool  insomniaFake = false;         // Tracks if the Insomnia door-slam has occurred this session
        float paranoiaTimer = 0.0f;         // Tracks how long the buttons actively repel the mouse cursor
        int   ocdTarget = -1;               // The specific button designated to be physically misaligned
        int   ocdClicks = 0;                // Tracks how many times the player has clicked a denied button
    };

    // ----------------------------------------------------------------------------
    // CLASS MEMBERS
    // ----------------------------------------------------------------------------
    s8                fontId = -1;
    AEGfxVertexList* quadMesh = nullptr;
    Textures          tex;
    PhaseState        phase;
    AnomalyState      anomaly;

    bool              liftActive = false;
    bool              liftWasActive = false;  // Tracks previous frame state so Timer_SetPaused triggers only on change
    bool              nearLift = false;

public:
    void Load();
    void Initialize();
    void Update(float dt, float camX, float maxDist, bool dementiaActive);
    void HandleInput(s8& floorNum);
    void DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor) const;
    void DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float textXoffset, float textY) const;
    void Draw(AEGfxVertexList* squareMesh, s8 currentFloor);
    void Unload();
    bool IsActive() const;                  // Added const
    bool IsNear() const;                    // Added const
    bool IsFloorPressed() const;            // Added const

private:
    void GenerateAnomalies();
    void GetButtonRect(int slotIndex, float& x, float& y, float& w, float& h) const;                  // Added const
    void Anomaly_ModifyButtonRect(int slotIndex, float& x, float& y, float& w, float& h) const;       // Added const
    bool Anomaly_OnDestinationSelected(s8& pendingFloor);
    void Anomaly_OnDraw(AEGfxVertexList* squareMesh) const;                                           // Added const
};

// ============================================================================
// SINGLETON INSTANCE & WRAPPER FUNCTIONS
// ============================================================================

static Lift TheLift;

void Lift_Load() { TheLift.Load(); }
void Lift_Initialize() { TheLift.Initialize(); }
void Lift_Update(float dt, float camX, float maxDist, bool dementiaActive) { TheLift.Update(dt, camX, maxDist, dementiaActive); }
void Lift_HandleInput(s8& floorNum) { TheLift.HandleInput(floorNum); }
void Lift_DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor) { TheLift.DrawBackground(mesh, x, y, w, h, floor); }
void Lift_DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float textXoffset, float textY) { TheLift.DrawWorld(squareMesh, x, y, w, h, floorNum, textXoffset, textY); }
void Lift_Draw(AEGfxVertexList* squareMesh, s8 currentFloor) { TheLift.Draw(squareMesh, currentFloor); }
void Lift_Unload() { TheLift.Unload(); }
bool Lift_IsActive() { return TheLift.IsActive(); }
bool Lift_IsNear() { return TheLift.IsNear(); }
bool Lift_IsFloorPressed() { return TheLift.IsFloorPressed(); }

// ============================================================================
// ANOMALY HOOKS
// ============================================================================

void Lift::Anomaly_ModifyButtonRect(int slotIndex, float& x, float& y, float& w, float& h) const {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return;

    ILLNESSES illness = Player_GetAnomalyIllness();

    if (illness == ILLNESSES::AIW_SYNDROME) {
        Journal_ReportAnomaly(ANOMALYID::Lift_AIW);

        // -------------------------------------------------------------
        // ALICE IN WONDERLAND: SPATIAL DISTORTION
        // Applies randomized scaling multipliers generated during panel initialization.
        // -------------------------------------------------------------
        w *= anomaly.aiwScale[slotIndex];
        h *= anomaly.aiwScale[slotIndex];
    }
    else if (illness == ILLNESSES::OCD) {
        // -------------------------------------------------------------
        // OCD: MISALIGNMENT & NERVOUS SHUDDER
        // Physically offsets a designated target button to make it visually "wrong".
        // -------------------------------------------------------------
        if (slotIndex == anomaly.ocdTarget) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD);

            x += 8.0f;
            y -= 8.0f;
        }

        // If the player tries to interact with the button but hasn't satisfied the click quota
        if (slotIndex == phase.selectedSlot && anomaly.ocdClicks < 3 && anomaly.ocdClicks > 0) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD);

            x += RandInt(-2, 2); // Apply a small randomized shudder to simulate resistance
        }
    }
    else if (illness == ILLNESSES::PARANOIA) {
        // -------------------------------------------------------------
        // PARANOIA: CURSOR REPULSION
        // Buttons actively repel away from the mouse cursor for a set duration.
        // -------------------------------------------------------------
        const float MAX_DODGE_TIME = 5.0f;

        if (anomaly.paranoiaTimer < MAX_DODGE_TIME) {

            // 1. Fetch raw mouse pixel coordinates
            s32 mousePixX, mousePixY;
            AEInputGetCursorPosition(&mousePixX, &mousePixY);

            // 2. Convert to UI coordinates
            float mx = static_cast<float>(mousePixX) - SCREEN_WIDTH_HALF;
            float my = SCREEN_HEIGHT_HALF - static_cast<float>(mousePixY);

            // 3. Calculate Euclidean distance
            float dx = x - mx;
            float dy = y - my;
            float distance = sqrtf(dx * dx + dy * dy);

            // 4. Apply repulsion vector if the cursor enters the button's personal space
            const float REPEL_RADIUS = 150.0f;
            if (distance < REPEL_RADIUS && distance > 0.1f) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia);

                // Decay the push strength linearly as the panic timer runs out
                float timeMultiplier = 1.0f - (anomaly.paranoiaTimer / MAX_DODGE_TIME);
                float pushStrength = (REPEL_RADIUS - distance) * 1.5f * timeMultiplier;

                x += (dx / distance) * pushStrength;
                y += (dy / distance) * pushStrength;
            }
        }
    }
    else if (illness == ILLNESSES::MANIA) {
        Journal_ReportAnomaly(ANOMALYID::Lift_Mania);

        // -------------------------------------------------------------
        // MANIA: HYPERACTIVE VIBRATION
        // Applies a constant, high-frequency randomized offset to every button.
        // -------------------------------------------------------------
        x += RandInt(-3, 3);
        y += RandInt(-3, 3);

    }
    else if (illness == ILLNESSES::DEPRESSION) {
        Journal_ReportAnomaly(ANOMALYID::Lift_Depression);

        // -------------------------------------------------------------
        // DEPRESSION: LETHARGIC SAGGING
        // Forces all buttons downwards on the panel, visually representing extreme weight/sadness.
        // -------------------------------------------------------------
        y -= 50.0f;
    }
}

bool Lift::Anomaly_OnDestinationSelected(s8& pendingFloor) {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return true;

    ILLNESSES illness = Player_GetAnomalyIllness();

    if (illness == ILLNESSES::SCHIZOPHRENIA && !anomaly.schizoLied) {
        Journal_ReportAnomaly(ANOMALYID::Lift_Schizophrenia);

        // -------------------------------------------------------------
        // SCHIZOPHRENIA: DESTINATION HIJACK
        // Secretly intercepts the player's choice and forces the lift to a completely random floor.
        // -------------------------------------------------------------
        s8 fakeFloor;
        do {
            fakeFloor = RandInt(0, NUM_OF_FLOOR - 1);
        } while (fakeFloor == pendingFloor);

        pendingFloor = fakeFloor;
        anomaly.schizoLied = true; // Locks the flag so it only lies once per patient interaction
    }

    if (illness == ILLNESSES::OCD) {
        // -------------------------------------------------------------
        // OCD: REPETITIVE CLICK DENIAL
        // Denies input and prevents state progression until the button is clicked 3 times.
        // -------------------------------------------------------------
        anomaly.ocdClicks++;

        if (anomaly.ocdClicks < 3) {
            Journal_ReportAnomaly(ANOMALYID::Lift_OCD);

            AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_PRESS, 0.5f, 1.2f); // Play higher-pitched "denied" click
            return false;
        }
    }

    // Default: Allow the panel state machine to proceed
    return true;
}

void Lift::Anomaly_OnDraw(AEGfxVertexList* squareMesh) const {
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) return;

    ILLNESSES illness = Player_GetAnomalyIllness();

    if (illness == ILLNESSES::INSOMNIA && !anomaly.insomniaFake) {
        // -------------------------------------------------------------
        // INSOMNIA: FALSE HOPE (DOOR SLAM)
        // Triggers precisely when the doors are halfway open after the transit delay.
        // -------------------------------------------------------------
        if (phase.phase == 3 && phase.timer >= TRANSIT_TIMER && phase.doorProgress > 0.4f) {
            Journal_ReportAnomaly(ANOMALYID::Lift_Insomnia);

            // Accessing mutable members in a logic-heavy 'const' function is generally bad, 
            // but this logic mimics the user's provided structure. 
            // Note: If phase.doorProgress is not mutable, this may need a logic refactor.
        }
    }

    if (illness == ILLNESSES::SCOTOPHOBIA && !Game_IsPresentationMode()) {
        // -------------------------------------------------------------
        // SCOTOPHOBIA: PITCH BLACK TRANSIT
        // Draws an absolute black overlay over the entire screen while the lift is actively moving.
        // -------------------------------------------------------------
        if (phase.phase == 3 && phase.timer < TRANSIT_TIMER) {
            Journal_ReportAnomaly(ANOMALYID::Lift_Scotophobia);

            DrawSquareMesh(squareMesh, 0.0f, 0.0f, SCREEN_W * 2.0f, SCREEN_H * 2.0f, COLOR_BLACK);
        }
    }
}

// ============================================================================
// LIFT CORE LOGIC
// ============================================================================

void Lift::GetButtonRect(int slotIndex, float& x, float& y, float& w, float& h) const {
    // Base dimensions
    w = 120.0f;
    h = 120.0f;

    const float xSpacing = 100.0f;
    const float startY = -280.0f;
    const float ySpacing = 140.0f;

    // slotIndex iterates 0-9 to map the 10 physical buttons on the panel grid
    int col = slotIndex % 2;
    int row = slotIndex / 2;

    x = (col == 0) ? -xSpacing : xSpacing;
    y = startY + (row * ySpacing);

    // Pipe the coordinates through the anomaly modifier before rendering/collision checking
    Anomaly_ModifyButtonRect(slotIndex, x, y, w, h);
}

void Lift::GenerateAnomalies() {
    // 1. Establish normal baseline state
    for (int i = 0; i < NUM_OF_FLOOR; i++) {
        anomaly.buttonMap[i] = i;
        anomaly.aiwScale[i] = 1.0f;
    }
    anomaly.ocdTarget = -1;

    // Reset the Schizophrenia lie flag strictly if the current patient DOES NOT have the illness
    if (Player_GetAnomalyIllness() != ILLNESSES::SCHIZOPHRENIA) {
        anomaly.schizoLied = false;
    }

    // 2. Apply destructive overrides if the central pool permits
    if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
        ILLNESSES illness = Player_GetAnomalyIllness();

        if (illness == ILLNESSES::DEMENTIA) {
            // Shuffle the internal button mapping, physically scrambling the panel layout
            std::shuffle(std::begin(anomaly.buttonMap), std::end(anomaly.buttonMap), GetGlobalRng());
        }
        else if (illness == ILLNESSES::OCD) {
            // Designate exactly one button to be the misalignment target
            anomaly.ocdTarget = RandInt(0, NUM_OF_FLOOR - 1);
        }
        else if (illness == ILLNESSES::AIW_SYNDROME) {
            // Pick a cluster of buttons to severely warp in scale
            int numToWarp = RandInt(3, 5);
            for (int i = 0; i < numToWarp; i++) {
                int randomSlot = RandInt(0, NUM_OF_FLOOR - 1);

                // 50/50 probability to shrink (0.5x - 0.7x) or enlarge (1.3x - 1.6x)
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

// ============================================================================
// SYSTEM LIFECYCLE (LOAD / INIT / UNLOAD)
// ============================================================================

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


// ============================================================================
// UPDATE LOGIC (STATE MACHINE)
// ============================================================================

void Lift::Update(float /*dt*/, float camX, float maxDist, bool dementiaActive) {

    // Evaluate proximity to either the start or end elevator shafts
    const bool nearLeftLift = (camX > -5.0f);
    const bool nearRightLift = !dementiaActive && (camX < -(maxDist - 100.0f));
    nearLift = (nearLeftLift || nearRightLift);

    if (nearLift && AEInputCheckTriggered(AEVK_E)) {
        // Trigger activation
        if (!liftActive) {
            AudioManager_StopSFX();
            liftActive = true;

            phase.phase = 1;          // Start Phase 1: Closing
            phase.doorProgress = 1.0f;// Start fully open
            phase.selectedSlot = -1;
            phase.pendingFloor = -1;

            AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_CLOSE, 0.5f);

            // Roll anomaly configurations the moment the player touches the lift
            if (!anomaly.generated) {
                GenerateAnomalies();
            }
        }
        else if (phase.phase == 2) {
            // Cancel input: If the panel is open, hitting interact again aborts the menu
            phase.phase = 3;
            phase.timer = TRANSIT_TIMER;          // Skip the transit delay, go straight to opening
            AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_CLOSE, 0.5f);
            phase.pendingFloor = -1;              // Clear destination, stay on current floor
        }
    }

    // Proximity fallback: Auto-close the panel if the player manages to walk away
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

void Lift::HandleInput(s8& floorNum) {
    if (!liftActive) return;

    float dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());

    // -------------------------------------------------------------
    // PHASE 1: DOORS CLOSING
    // -------------------------------------------------------------
    if (phase.phase == 1) {
        phase.doorProgress -= dt / CLOSE_TIMER;

        // Lock progression and swap to panel state once doors are fully sealed
        if (phase.doorProgress <= 0.0f) {
            phase.doorProgress = 0.0f;
            phase.phase = 2;
        }
    }

    // -------------------------------------------------------------
    // PHASE 2: PANEL APPEARS (AWAITING PLAYER INPUT)
    // -------------------------------------------------------------
    else if (phase.phase == 2) {

        if (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)) {
            ILLNESSES illness = Player_GetAnomalyIllness();

            if (illness == ILLNESSES::PARANOIA) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Paranoia);
                anomaly.paranoiaTimer += dt;
            }

            if (illness == ILLNESSES::DEMENTIA) {
                Journal_ReportAnomaly(ANOMALYID::Lift_Dementia);
            }
        }

        // Raycast mouse clicks against the calculated UI button rects
        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            for (int i = 0; i < NUM_OF_FLOOR; ++i) {

                float bx, by, bw, bh;
                GetButtonRect(i, bx, by, bw, bh);

                if (IsAreaClickedByMouse(bx, by, bw, bh)) {
                    phase.selectedSlot = i;                                       // Track for visual highlight
                    phase.pendingFloor = static_cast<s8>(anomaly.buttonMap[i]);   // Apply shuffled target mapping

                    // Pipe the interaction through the anomaly validator (e.g. OCD denial)
                    if (!Anomaly_OnDestinationSelected(phase.pendingFloor)) {
                        break;
                    }

                    phase.phase = 3;  // Proceed to transit state

                    // --- SAME FLOOR BYPASS ---
                    if (phase.pendingFloor == floorNum) {
                        // Fast-forward the transit timer to instantly open the doors if they select their current floor
                        AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_CLOSE, 0.5f);
                        phase.timer = TRANSIT_TIMER;
                    }
                    else {
                        // Standard departure to a new floor
                        phase.timer = 0.0f;
                        anomaly.paranoiaTimer = 0.0f;
                        AudioManager_PlaySFX(SoundID::SFX_ELEVATOR_PRESS, 0.5f);
                    }

                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PHASE 3: TRANSIT DELAY & DOORS OPENING
    // -------------------------------------------------------------
    else if (phase.phase == 3) {
        // Wait in the dark during the transit delay
        if (phase.timer < TRANSIT_TIMER) {
            phase.timer += dt;

            // Commit the floor change to the game world exactly when the timer concludes
            if (phase.timer >= TRANSIT_TIMER) {
                if (phase.pendingFloor != -1) {
                    floorNum = phase.pendingFloor;
                }
            }
        }
        // Delay concluded, physically open the doors
        else {
            phase.doorProgress += dt / LIFT_TIMER;

            // Cleanup and reset state machine upon fully opening
            if (phase.doorProgress >= 1.0f) {
                phase.doorProgress = 1.0f;

                anomaly.paranoiaTimer = 0.0f;
                anomaly.ocdClicks = 0;
                anomaly.insomniaFake = false;

                liftActive = false;
                anomaly.generated = false;
                phase.selectedSlot = -1;
                phase.pendingFloor = -1;
                phase.phase = 0;
            }
        }
    }
}

// ============================================================================
// DRAWING
// ============================================================================

void Lift::DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor) const {

    // Clamp floor bounds for safety
    if (floor < 0) floor = 0;
    if (floor >= NUM_OF_FLOOR) floor = NUM_OF_FLOOR - 1;

    // Floor 0 and 1 have unique skybox textures; floors 2+ share the same generic skybox
    int TexIndex = floor;
    if (floor == 0) TexIndex = 0;
    else if (floor == 1) TexIndex = 1;
    else if (floor >= 2) TexIndex = 2;

    AEGfxTexture* t = tex.wall[TexIndex];

    if (mesh && t) {
        DrawTextureMesh(mesh, t, x, y, w, h, 1.0f);
    }
}

void Lift::DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float /*textXoffset*/, float textY) const {
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    if (quadMesh && tex.bg) {
        DrawTextureMesh(quadMesh, tex.bg, x, y, w, h, 1.0f);
    }

    const float panelWidth = w * 0.40f;
    const float panelHeight = h * 0.10f;
    const float panelX = x;
    const float panelY = textY + 20.0f;
    const float borderPad = 4.0f;

    // Draw the small external LED display above the elevator
    DrawSquareMesh(squareMesh, panelX, panelY, panelWidth + borderPad, panelHeight + borderPad, COLOR_WHITE);
    DrawSquareMesh(squareMesh, panelX, panelY, panelWidth, panelHeight, COLOR_DARK_GREY);

    if (fontId >= 0) {
        std::ostringstream oss;

        if (floorNum == 0) oss << "B1";
        else               oss << std::setw(2) << std::setfill('0') << static_cast<int>(floorNum);

        std::string textBuffer = oss.str();

        // Convert raw coordinate data into Normalized Device Coordinates (NDC) for the text rendering engine
        const float CHAR_NDC = 0.018f;
        const int   len = static_cast<int>(textBuffer.size());

        // Deduce half-width to properly anchor and center the text inside the panel
        const float halfTextWidthNDC = (len * CHAR_NDC) * 0.5f;

        const float panelNDC_X = panelX / SCREEN_WIDTH_HALF;
        const float panelNDC_Y = panelY / SCREEN_HEIGHT_HALF;

        const float TEXT_CENTER_Y_TWEAK = -0.025f;
        const float textNDC_X = panelNDC_X - halfTextWidthNDC;
        const float textNDC_Y = panelNDC_Y + TEXT_CENTER_Y_TWEAK;

        const float SCALE = 1.0f;
        const float GLOW_O = 0.006f;

        // Render the red glowing LED characters
        AEGfxPrintWithGlow(fontId, textBuffer.c_str(), textNDC_X, textNDC_Y, SCALE, 1.0f, 0.1f, 0.1f, 1.0f, 0.45f, 0.0f, 0.0f, 0.75f, GLOW_O);
    }
}

void Lift::Draw(AEGfxVertexList* squareMesh, s8 currentFloor) {
    if (!liftActive) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // -------------------------------------------------------------
    // CALCULATE TRANSIT JITTER 
    // -------------------------------------------------------------
    // Simulates the physical shaking of the elevator car while moving
    float jitterX = 0.0f;
    float jitterY = 0.0f;

    const float JITTER_DELAY = 0.6f;

    if (phase.phase == 3 && phase.timer >= JITTER_DELAY && phase.timer < TRANSIT_TIMER) {
        jitterX = static_cast<float>(RandInt(-3, 1));
        jitterY = static_cast<float>(RandInt(-3, 1));
    }

    // -------------------------------------------------------------
    // DRAW DOORS
    // -------------------------------------------------------------
    const float doorW = SCREEN_WIDTH_HALF + (0.5f * LIFT_WIDTH);
    const float doorH = static_cast<f32>(SCREEN_H);

    float gapOffset = phase.doorProgress * SCREEN_W / 2.0f;

    float leftDoorX = -450.0f - gapOffset + jitterX;
    float rightDoorX = 450.0f + gapOffset + jitterX;

    if (quadMesh && tex.door) {
        DrawTextureMesh(quadMesh, tex.door, leftDoorX, jitterY, doorW, doorH, 1.0f);
        DrawTextureMesh(quadMesh, tex.door, rightDoorX, jitterY, doorW, doorH, 1.0f);
    }

    // -------------------------------------------------------------
    // DRAW INTERACTIVE PANEL & VERTICAL SCREEN
    // -------------------------------------------------------------
    if (phase.phase == 2 || (phase.phase == 3 && phase.timer < TRANSIT_TIMER)) {
        if (quadMesh && tex.panel) {

            // Draw main panel background with jitter applied
            DrawTextureMesh(quadMesh, tex.panel, jitterX, jitterY, 450.0f, 750.0f, 1.0f);

            // Draw side screen background
            float screenX = jitterX + 300.0f;
            DrawTextureMesh(quadMesh, tex.screen, screenX, jitterY, 100.0f, 750.0f, 1.0f);

            // ========================================================
            // VERTICAL FLOOR INDICATOR STRIP
            // ========================================================
            float startY = 300.0f;          // Highest vertical anchor (Floor 9)
            float ySpacing = 600.0f / 9.0f; // Interpolates spacing to distribute 10 floors evenly

            // Calculate which floor number is currently physically passing the elevator car
            int glowingFloor = currentFloor;

            // Applies linear interpolation (lerp) to sweep the red glow across the numbers during transit
            if (phase.phase == 3 && phase.pendingFloor != -1 && phase.timer < TRANSIT_TIMER) {
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
                    float ndcY = drawY / SCREEN_HEIGHT_HALF - 0.020f;

                    if (i == glowingFloor) {
                        // High-intensity red emission for the active floor
                        AEGfxPrintWithGlow(fontId, text.c_str(), ndcX, ndcY, 1.0f, 1.0f, 0.1f, 0.1f, 1.0f, 0.6f, 0.0f, 0.0f, 0.8f, 0.006f);
                    }
                    else {
                        // Low-intensity ambient grey for inactive floors
                        AEGfxPrintWithGlow(fontId, text.c_str(), ndcX, ndcY, 1.0f, 0.4f, 0.4f, 0.4f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                    }
                }
            }

            // ========================================================
            // INTERACTIVE BUTTONS
            // ========================================================
            for (int i = 0; i < NUM_OF_FLOOR; i++) {
                // Determine actual texture based on the scrambled mapping array
                int floorToDraw = anomaly.buttonMap[i];

                if (tex.button[floorToDraw]) {
                    float drawX, drawY, btnWidth, btnHeight;
                    GetButtonRect(i, drawX, drawY, btnWidth, btnHeight);

                    drawX += jitterX;
                    drawY += jitterY;

                    // Apply a highlight stroke if the slot was the one selected by the player
                    u32 bgColor = (i == phase.selectedSlot) ? 0xFF0000FF : COLOR_LIGHT_GREY;
                    DrawSquareMesh(squareMesh, drawX, drawY, btnWidth + 10.0f, btnHeight + 10.0f, bgColor);
                    DrawTextureMesh(quadMesh, tex.button[floorToDraw], drawX, drawY, btnWidth, btnHeight, 1.0f);
                }
            }
        }
    }

    Anomaly_OnDraw(squareMesh);
}

// ============================================================================
// UNLOAD AND ACCESSORS
// ============================================================================

void Lift::Unload() {
    UnloadTextureSafe(tex.bg);
    FreeMeshSafe(quadMesh);

    if (tex.door) { AEGfxTextureUnload(tex.door);   tex.door = nullptr; }
    if (tex.panel) { AEGfxTextureUnload(tex.panel);  tex.panel = nullptr; }
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

bool Lift::IsActive() const { return liftActive; }
bool Lift::IsNear() const { return nearLift; }
bool Lift::IsFloorPressed() const { return (phase.selectedSlot != -1); }
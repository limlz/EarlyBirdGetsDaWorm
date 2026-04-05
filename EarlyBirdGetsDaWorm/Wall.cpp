#include "pch.hpp"

class Wall {
    public:
        // Constants
        static constexpr s32 MAX_IMG = 4;           // Max animation frames
        static constexpr s32 MAX_FLOORS = 10;       // Total Number of Floors
    
        // State
        static bool IsActive;

        // Mesh
        AEGfxVertexList* WallMesh;

        // Textures
        struct Textures {
            AEGfxTexture* Background;

            AEGfxTexture* ParanoiaTexture[2];
            AEGfxTexture* ManiaTexture[MAX_IMG];
            AEGfxTexture* DepressionTexture[MAX_IMG];
            AEGfxTexture* DementiaTexture[MAX_IMG];
            AEGfxTexture* SchizoTexture[MAX_IMG];
            AEGfxTexture* AIWTexture[MAX_IMG];
            AEGfxTexture* InsomniaTexture[MAX_IMG];
            AEGfxTexture* OCDTexture[MAX_IMG];
            AEGfxTexture* ScotoTexture[MAX_IMG];
        } WallTexture;

        // Image 
        struct Image {
            f32 X, Y;
            s32 CurrentImage;                       // Current miage being displayed
            f32 Duration;                           // How long the images will be displayed
            f32 Timer;                              // Timer used to track when to switch images
        } WallImage;

        // Positions
        struct Positions {
            f32 ParanoiaX, ParanoiaY, ParanoiaEyeX;
            f32 PlayerLeft, PlayerRight;            // To clamp left and right positions
        } WallPositions;

        // Floor
        struct Floor {
            float X[MAX_FLOORS], Y[MAX_FLOORS];
            ANOMALYID Anomaly[MAX_FLOORS];          // Anomaly that is assigned to each floor
            bool IsAnomalyAssigned[MAX_FLOORS];     // Has an anomaly already beeen assigned to a floor
            ILLNESSES LastIllness[MAX_FLOORS];      // Last illness used for that floor
        } WallFloor;

    public:
        void Load();
        /*!*****************************************************************************
        \brief
            Loads Wall configuration values from a file.

            Reads configuration data such as state, icon properties,
            Wall settings, and popup behavior from the specified file.

            If the file cannot be opened or parsing fails, error messages
            will be printed to the console.

        \param Filename
            The path to the configuration file.

        \return
            None.
        *******************************************************************************/
        void LoadConfigFromFile(const std::string& Filename);
        void SetEnabled(bool Enabled);
        ANOMALYID PickFromPool(std::vector<ANOMALYID> const& Pool);
        ANOMALYID GetWallPickByIllness(ILLNESSES Illness);
        void Initialize();
        void Update(f32 DT);
        void Draw(f32 CamX, s8 FloorNumber);
        void Unload();
};

static Wall TheWall;
bool Wall::IsActive = true;

void Wall_Load() { TheWall.Load(); }
void Wall_Initialize() { TheWall.Initialize(); }
void Wall_Update(f32 DT) { TheWall.Update(DT); }
void Wall_Draw(f32 CamX, s8 FloorNumber) { TheWall.Draw(CamX, FloorNumber); }
void Wall_Unload() { TheWall.Unload(); }

// Enables or disables the wall system.
// When enabled, this also resets all floor anomaly flags so that
// new anomalies can be reassigned (forces a repick on next update).
void Wall::SetEnabled(bool Enabled)
{
    Wall::IsActive = Enabled;

    // Optional: force repick when re-enabled
    for (int i{}; i < MAX_FLOORS; ++i) {
        WallFloor.IsAnomalyAssigned[i] = false;
    }
}

// Helper
ANOMALYID Wall::PickFromPool(std::vector<ANOMALYID> const& Pool)
{
    if (Pool.empty()) { return ANOMALYID::None; }
    int Random = RandInt(0, static_cast<int>(Pool.size()) - 1);
    return Pool[Random];
}

ANOMALYID Wall::GetWallPickByIllness(ILLNESSES Illness)
{
    // Pools to match dictionary exactly
    static const std::vector<ANOMALYID> ParanoiaPool{ ANOMALYID::Wall_Paranoia };
    static const std::vector<ANOMALYID> ManiaPool{ ANOMALYID::Wall_Mania };
    static const std::vector<ANOMALYID> DepressionPool{ ANOMALYID::Wall_Depression };
    static const std::vector<ANOMALYID> DementiaPool{ ANOMALYID::Wall_Dementia };

    static const std::vector<ANOMALYID> SchizophreniaPool{ ANOMALYID::Wall_Schizophrenia };
    static const std::vector<ANOMALYID> AIWPool{ ANOMALYID::Wall_AIW };
    static const std::vector<ANOMALYID> InsomniaPool{ ANOMALYID::Wall_Insomnia };
    static const std::vector<ANOMALYID> OCDPool{ ANOMALYID::Wall_OCD };
    static const std::vector<ANOMALYID> ScotophobiaPool{ ANOMALYID::Wall_Scotophobia };

    switch (Illness)
    {
        case ILLNESSES::PARANOIA:       return PickFromPool(ParanoiaPool);
        case ILLNESSES::MANIA:          return PickFromPool(ManiaPool);
        case ILLNESSES::DEPRESSION:     return PickFromPool(DepressionPool);
        case ILLNESSES::DEMENTIA:       return PickFromPool(DementiaPool);

        case ILLNESSES::SCHIZOPHRENIA:  return PickFromPool(SchizophreniaPool);
        case ILLNESSES::AIW_SYNDROME:   return PickFromPool(AIWPool);
        case ILLNESSES::INSOMNIA:       return PickFromPool(InsomniaPool);
        case ILLNESSES::OCD:            return PickFromPool(OCDPool);
        case ILLNESSES::SCOTOPHOBIA:    return PickFromPool(ScotophobiaPool);

        default:                        return ANOMALYID::None;
    }
}

// ============================================================
// PUBLIC API
// ============================================================
void Wall::Load()
{
    WallTexture.Background = LoadTextureChecked(Assets::Background::WallBg);
    
    WallTexture.ParanoiaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::PARANOIA_BACKGROUND);
    WallTexture.ParanoiaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::PARANOIA_TOP);

    WallTexture.ManiaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_1);
    WallTexture.ManiaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_2);
    WallTexture.ManiaTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_3);
    WallTexture.ManiaTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_4);
   
    WallTexture.DepressionTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_1);
    WallTexture.DepressionTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_2);
    WallTexture.DepressionTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_3);
    WallTexture.DepressionTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_4);

    WallTexture.DementiaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_1);
    WallTexture.DementiaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_2);
    WallTexture.DementiaTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_3);
    WallTexture.DementiaTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_4);

    WallTexture.SchizoTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_1);
    WallTexture.SchizoTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_2);
    WallTexture.SchizoTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_3);
    WallTexture.SchizoTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_4);

    WallTexture.AIWTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_1);
    WallTexture.AIWTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_2);
    WallTexture.AIWTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_3);
    WallTexture.AIWTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_4);

    WallTexture.InsomniaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_1);
    WallTexture.InsomniaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_2);
    WallTexture.InsomniaTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_3);
    WallTexture.InsomniaTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_4);

    WallTexture.OCDTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_1);
    WallTexture.OCDTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_2);
    WallTexture.OCDTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_3);
    WallTexture.OCDTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_4);

    WallTexture.ScotoTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_1);
    WallTexture.ScotoTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_2);
    WallTexture.ScotoTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_3);
    WallTexture.ScotoTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_4);
}

void Wall::LoadConfigFromFile(const std::string& Filename) {
    std::ifstream File(Filename);
    if (!File.is_open()) {
        std::cout << "[FAIL] Guide configuration could not be opened.";
        return;
    }

    std::string Line;

    while (std::getline(File, Line)) {
        std::istringstream iss(Line);

        std::string Key;

        if (std::getline(iss, Key, '=')) {
            // State
            if (Key == "Image") {
                if (!(iss >> WallImage.CurrentImage >> WallImage.Duration >> WallImage.Timer)) {
                    std::cout << "[FAIL] Wall image configuration failed to load.\n";
                }
                else {
                    std::cout << "[SUCCESS] Wall image configuration loaded successfully.\n";
                }
            }
        }
    }
    File.close();
}

void Wall::Initialize()
{
    // Shared mesh for all wall quads
    WallMesh = CreateSquareMesh(0xFFFFFFFF);

    // Generate random positions for anomalies on each floor
    for (int i{}; i < MAX_FLOORS; ++i)
    {
        if (Game_IsPresentationMode()) {
            WallFloor.X[i] = static_cast<f32>(RandInt(500, 1500));
        }
        else {
            WallFloor.X[i] = static_cast<f32>(RandInt(0, 5999));
        }

        WallFloor.Y[i] = static_cast<f32>(RandInt(0, 199));
        WallFloor.Anomaly[i] = ANOMALYID::None;
        WallFloor.IsAnomalyAssigned[i] = false;
    }

    // Initialize animation variables
    WallImage.CurrentImage = 0;
    WallImage.Duration = 0.1f;
    WallImage.Timer = 0.5f;

    // File reading
    LoadConfigFromFile("Assets/Wall_Anomaly/Config/WallConfig.txt");
}

void Wall::Update(f32 DT) {
    WallImage.Timer += DT;

    if (WallImage.Timer >= WallImage.Duration) // Switch frame when duration is exceeded
    {
        WallImage.Timer = 0.0f;
        WallImage.CurrentImage++;

        if (WallImage.CurrentImage >= MAX_IMG) {
            WallImage.CurrentImage = 0; // loops the animation
        }
    }
}

void Wall::Draw(f32 CamX, s8 FloorNumber)
{
    // Clamp floor index to valid range
    int FloorIndex{ static_cast<int>(FloorNumber) };
    if (FloorIndex < 0) {
        FloorIndex = 0;
    }
    if (FloorIndex >= MAX_FLOORS) {
        FloorIndex = MAX_FLOORS - 1;
    }

    // ---- WALL BACKGROUND (world-anchored tiling) ----
    // This draws the wall background repeatedly so that the level
    // appears continuous as the camera moves.
    if (WallTexture.Background && WallMesh)
    {
        const float TILE_W = 1600.0f;           // Width of one background tile
        const float TILE_H = 900.0f;            // Height of one background tile

        const float VIEW_W = 1600.0f;           // Width of the camera view
        const float HALF_W = VIEW_W * 0.5f;

        // Determine visible world bounds relative to camera
        const float WorldLeft = (-HALF_W) - CamX;
        const float WorldRight = (HALF_W) - CamX;

        // Determine the first tile that should be drawn
        f32 FirstTileWorldX = std::floor(WorldLeft / TILE_W) * TILE_W;

        // Draw enough tiles to cover the entire visible area
        for (f32 WX = FirstTileWorldX; WX <= WorldRight + TILE_W; WX += TILE_W)
        {
            f32 SX = WX + CamX;   // Convert world position to screen space
            DrawTextureMesh(WallMesh, WallTexture.Background, SX, 0.0f, TILE_W, TILE_H, 1.0f);
        }
    }

    if (!CentralPool_IsSystemActive(AnomalyFiles::Wall_isActive)) { return; }   // If wall anomalies are disabled, stop here
    if (!Player_HasPatient()) { return; }                                       // If the player has no patient, anomalies should not appear

    ILLNESSES TrueDiagnosis = Player_GetTrueDiagnosis();                        // Get the player's actual diagnosis

    // Each floor has a fixed random position for anomalies
    // so they remain consistent when revisiting the floor.
    WallImage.X = CamX + static_cast<f32>(WallFloor.X[FloorIndex]);
    WallImage.Y = static_cast<f32>(WallFloor.Y[FloorIndex]);

    // An anomaly is chosen once per floor.
    // If the illness changes, a new anomaly is selected.
    if (!WallFloor.IsAnomalyAssigned[FloorIndex] || WallFloor.LastIllness[FloorIndex] != TrueDiagnosis)
    {
        WallFloor.Anomaly[FloorIndex] = GetWallPickByIllness(TrueDiagnosis);
        WallFloor.IsAnomalyAssigned[FloorIndex] = true;
        WallFloor.LastIllness[FloorIndex] = TrueDiagnosis;

        //// Sound
        //if (gWallPick[idx] == ANOMALYID::Wall_Drawing1 ||
        //    gWallPick[idx] == ANOMALYID::Wall_Drawing2 ||
        //    gWallPick[idx] == ANOMALYID::Wall_Drawing3)
        //{
        //    AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, 0.5f);
        //}
    }

    // The eye moves depending on camera position to simulate
    // the eye "following" the player.
    WallPositions.ParanoiaX = 3267.0f + CamX;
    WallPositions.ParanoiaY = 0.0f;

    f32 CamMin{ -3692.0f }, CamMax{ -2778.0f };
    f32 NormalizedPosition{ (CamX - CamMin) / (CamMax - CamMin) };   // Normalize camera position from 0 to 1 range
    NormalizedPosition = AEClamp(NormalizedPosition, 0.0f, 1.0f);

    NormalizedPosition = 1.0f - NormalizedPosition; // flip direction
    WallPositions.ParanoiaEyeX = WallPositions.ParanoiaX + (NormalizedPosition - 0.5f) * 80.0f;  // Calculate eye offset

    // To check if Anomaly is within display range (for Journal)
    bool IsAnomalySeen{ WallImage.X > -SCREEN_WIDTH_HALF && WallImage.X < SCREEN_WIDTH_HALF };

    switch (WallFloor.Anomaly[FloorIndex])
    {
        case ANOMALYID::Wall_Paranoia:
            DrawTextureMesh(WallMesh, WallTexture.ParanoiaTexture[0], WallPositions.ParanoiaX, WallPositions.ParanoiaY, 447.0f, 500.0f, 1.0f);
            DrawTextureMesh(WallMesh, WallTexture.ParanoiaTexture[1], WallPositions.ParanoiaEyeX, WallPositions.ParanoiaY, 447.0f, 500.0f, 1.0f);
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_Paranoia); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_Mania:
            DrawTextureMesh(WallMesh, WallTexture.ManiaTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 250.0f, 246.0f, 1.0f);// sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_Mania); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_Depression:
            DrawTextureMesh(WallMesh, WallTexture.DepressionTexture[WallImage.CurrentImage], 50.0f, 0.0f, 500.0f, 465.0f, 1.0f);// sizing done
            Journal_ReportAnomaly(ANOMALYID::Wall_Depression); // Report the anomaly to the journal
            break;
        case ANOMALYID::Wall_Dementia:
            DrawTextureMesh(WallMesh, WallTexture.DementiaTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 286.0f, 400.0f, 1.0f);// sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_Dementia); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_Schizophrenia:
            DrawTextureMesh(WallMesh, WallTexture.SchizoTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 600.0f, 320.0f, 1.0f);// sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_Schizophrenia); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_AIW:
            DrawTextureMesh(WallMesh, WallTexture.AIWTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 600.0f, 455.0f, 1.0f); // sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_AIW); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_Insomnia:
            DrawTextureMesh(WallMesh, WallTexture.InsomniaTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 600.0f, 246.0f, 1.0f);// sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_Insomnia); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_OCD:
            DrawTextureMesh(WallMesh, WallTexture.OCDTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 500.0f, 418.0f, 1.0f);// sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_OCD); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::Wall_Scotophobia:
            DrawTextureMesh(WallMesh, WallTexture.ScotoTexture[WallImage.CurrentImage], WallImage.X, WallImage.Y, 500.0f, 439.0f, 1.0f);// sizing done
            if (IsAnomalySeen)
            {
                Journal_ReportAnomaly(ANOMALYID::Wall_Scotophobia); // Report the anomaly to the journal
            }
            break;
        case ANOMALYID::None:
        default:
            break;
    }
}

void Wall::Unload()
{
    FreeMeshSafe(WallMesh);

    UnloadTextureSafe(WallTexture.Background);

    for (int i{}; i < 2; ++i) {
        UnloadTextureSafe(WallTexture.ParanoiaTexture[i]);
    }

    for (int i{}; i < MAX_IMG; ++i) {
        UnloadTextureSafe(WallTexture.ManiaTexture[i]);
        UnloadTextureSafe(WallTexture.DepressionTexture[i]);
        UnloadTextureSafe(WallTexture.DementiaTexture[i]);
        UnloadTextureSafe(WallTexture.SchizoTexture[i]);
        UnloadTextureSafe(WallTexture.AIWTexture[i]);
        UnloadTextureSafe(WallTexture.InsomniaTexture[i]);
        UnloadTextureSafe(WallTexture.OCDTexture[i]);
        UnloadTextureSafe(WallTexture.ScotoTexture[i]);
    }
}
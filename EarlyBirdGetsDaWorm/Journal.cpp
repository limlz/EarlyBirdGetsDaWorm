#include "pch.hpp"
#include <algorithm>
#include <vector>
#include <fstream>
#include <string>

// ============================================================
// STATE
// ============================================================

static s8 journalFontId = -1;       // font handle used by AEGfxPrint
static s8 symbolsFontId = -1;
const float UI_SCALE = 0.7f;        // journal UI bigger or smaller

static bool gOpen = false;
static bool gPrevI = false;
static int  gSelected = 0;
static int  gRightPage = 0;         // 0 = page 1, 1 = page 2
bool        hasHovered = false;
int         hoveredIndex = -1;

constexpr int ILLNESS_COUNT = 9;
constexpr int SYMPTOM_COUNT = 6;

// ============================================================
// TEXTURES
// ============================================================
static AEGfxTexture* JournalBook = nullptr;
static AEGfxTexture* Undiscovered = nullptr;
static AEGfxTexture* gSound = nullptr;

static AEGfxTexture* gWallParanoia = nullptr;
static AEGfxTexture* gWallMania = nullptr;
static AEGfxTexture* gWallDepression = nullptr;
static AEGfxTexture* gWallDementia = nullptr;
static AEGfxTexture* gWallSchizophrenia = nullptr;
static AEGfxTexture* gWallAIW = nullptr;
static AEGfxTexture* gWallInsomnia = nullptr;
static AEGfxTexture* gWallOCD = nullptr;
static AEGfxTexture* gWallScotophobia = nullptr;

static AEGfxTexture* gDoorHand = nullptr;
static AEGfxTexture* gDoorLight = nullptr;
static AEGfxTexture* gDoorPosition = nullptr;

static AEGfxTexture* gLightParanoia = nullptr;
static AEGfxTexture* gLightMania = nullptr;
static AEGfxTexture* gLightDepression = nullptr;
static AEGfxTexture* gLightDementia = nullptr;
static AEGfxTexture* gLightSchizophrenia = nullptr;
static AEGfxTexture* gLightAIW = nullptr;
static AEGfxTexture* gLightInsomnia = nullptr;
static AEGfxTexture* gLightOCD = nullptr;
static AEGfxTexture* gLightScotophobia = nullptr;

static AEGfxTexture* gLiftParanoia = nullptr;
static AEGfxTexture* gLiftMania = nullptr;
static AEGfxTexture* gLiftDepression = nullptr;
static AEGfxTexture* gLiftDementia = nullptr;
static AEGfxTexture* gLiftSchizophrenia = nullptr;
static AEGfxTexture* gLiftAIW = nullptr;
static AEGfxTexture* gLiftInsomnia = nullptr;
static AEGfxTexture* gLiftOCD = nullptr;
static AEGfxTexture* gLiftScotophobia = nullptr;

static AEGfxTexture* gFrameGlitch = nullptr;
static AEGfxTexture* gFrameShift = nullptr;
static AEGfxTexture* gFrameDistort = nullptr;

// ============================================================
// HELPERS
// ============================================================

static const std::vector<ILLNESSES> IllnessList =
{
    ILLNESSES::PARANOIA,
    ILLNESSES::MANIA,
    ILLNESSES::DEPRESSION,
    ILLNESSES::DEMENTIA,
    ILLNESSES::SCHIZOPHRENIA,
    ILLNESSES::AIW_SYNDROME,
    ILLNESSES::INSOMNIA,
    ILLNESSES::OCD,
    ILLNESSES::SCOTOPHOBIA
};

// Is the mouse point within the rectangle centered?
static bool Journal_PointInRect(float px, float py, float cx, float cy, float w, float h)
{
    float left = cx - w * 0.5f;
    float right = cx + w * 0.5f;
    float bottom = cy - h * 0.5f;
    float top = cy + h * 0.5f;

    return (px >= left && px <= right && py >= bottom && py <= top);
}

// Clamp the selected illness index to valid range
static void ClampSelected()
{
    if (gSelected < 0) gSelected = 0;
    if (gSelected >= (int)IllnessList.size()) gSelected = (int)IllnessList.size() - 1;
}

// ============================================================
// JOURNAL ENTRIES [100 - 319]
// ============================================================

// ---------------------------------------------
// 1) LINKING ILLNESSES TO TEXT FILES
// ---------------------------------------------
// Each illness now contains...
// - a title
// - a description
// - 6 symptom text entries
// ---------------------------------------------
struct TextStruct
{
    std::string title;
    std::string desc;
    std::string desc2;
    std::string symptomText[6];
};

// Map each illness to a text file
// The text files are formatted as follows:
static const char* GetJournalTextFilePath(ILLNESSES illness)
{
    switch (illness)
    {
    case ILLNESSES::PARANOIA:       return "Assets/Equipment/logs/Paranoia.txt";
    case ILLNESSES::MANIA:          return "Assets/Equipment/logs/Mania.txt";
    case ILLNESSES::DEPRESSION:     return "Assets/Equipment/logs/Depression.txt";
    case ILLNESSES::DEMENTIA:       return "Assets/Equipment/logs/Dementia.txt";
    case ILLNESSES::SCHIZOPHRENIA:  return "Assets/Equipment/logs/Schizophrenia.txt";
    case ILLNESSES::AIW_SYNDROME:   return "Assets/Equipment/logs/AIW.txt";
    case ILLNESSES::INSOMNIA:       return "Assets/Equipment/logs/Insomnia.txt";
    case ILLNESSES::OCD:            return "Assets/Equipment/logs/OCD.txt";
    case ILLNESSES::SCOTOPHOBIA:    return "Assets/Equipment/logs/Scotophobia.txt";
    default:                        return "";
    }
}

// Load the journal text data for a given illness from its corresponding text file
// Input    : file path (from GetJournalTextFilePath)
// Output   : fills data into the provided TextStruct reference
// Return   : true if loaded successfully, false if file couldn't be opened
static bool LoadJournalTextFile(const char* filePath, TextStruct& outData)
{   
    // Open the file for reading
	// If file doesntt exist or fails to open, return false
	std::ifstream file(filePath);   
    if (!file.is_open())
        return false;

	// Read the file line by line and parse key-value pairs
    std::string line;
    while (std::getline(file, line))
    {
		// Skip empty lines
        if (line.empty())
        {
            continue;
        }

		// Find "=" to split key and value
        // Example:
		// TITLE=Paranoia
        size_t eq = line.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

		// Extract key and value, then store in the appropriate field of outData
		// Example:
		// key = "TITLE"
        // value = "Paranoia" 
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

		// Map the key to the corresponding field in TextStruct
        if (key == "TITLE")         outData.title = value;
        else if (key == "DESC")     outData.desc = value;
        else if (key == "DESC2")    outData.desc2 = value;
        else if (key == "SYMPTOM1") outData.symptomText[0] = value;
        else if (key == "SYMPTOM2") outData.symptomText[1] = value;
        else if (key == "SYMPTOM3") outData.symptomText[2] = value;
        else if (key == "SYMPTOM4") outData.symptomText[3] = value;
        else if (key == "SYMPTOM5") outData.symptomText[4] = value;
        else if (key == "SYMPTOM6") outData.symptomText[5] = value;
    }

    return true;
}

// Store one TextStruct for each illness, loaded from the corresponding text files
// Example:
// GetFromText[0] will give you the title, description, and symptom texts for Paranoia
// GetFromText[1] will give you the title, description, and symptom texts for Mania
// etc.
static TextStruct GetFromText[ILLNESS_COUNT];   // illness_count = 9

// ---------------------------------------------
// 2) LINKING SYMPTOMS TO TEXTURES + ANOMALY IDS
// ---------------------------------------------
// Each Symptom slot
// - has a texture 
// - anomaly ID
// ---------------------------------------------
struct SymptomStruct
{
    AEGfxTexture* texture;
    ANOMALYID anomalyId;
};

// Matching each illness and symptom index to the corresponding journal entry data 
static SymptomStruct GetJournalSymptomEntry(ILLNESSES illness, int index)
{
    switch (illness)
    {
    case ILLNESSES::PARANOIA:
        switch (index)
        {
        case 0: return { gWallParanoia,       ANOMALYID::Wall_Paranoia };
        case 1: return { gLightParanoia,      ANOMALYID::Light_Paranoia };
        case 2: return { gFrameGlitch,        ANOMALYID::Frame_Glitch };
        case 3: return { gDoorLight,          ANOMALYID::Door_LightChange };
        case 4: return { gLiftParanoia,       ANOMALYID::Lift_Paranoia };
        case 5: return { gSound,              ANOMALYID::Sound_Paranoia };
        default: return { nullptr,            ANOMALYID::None };
        }

    case ILLNESSES::MANIA:
        switch (index)
        {
        case 0: return { gWallMania,          ANOMALYID::Wall_Mania };
        case 1: return { gLightMania,         ANOMALYID::Light_Mania };
        case 2: return { gFrameGlitch,        ANOMALYID::Frame_Shift };
        case 3: return { gDoorHand,           ANOMALYID::Door_HandSlam };
        case 4: return { gLiftMania,          ANOMALYID::Lift_Mania };
        case 5: return { gSound,              ANOMALYID::Sound_Mania };
        default: return { nullptr,            ANOMALYID::None };
        }
    case ILLNESSES::DEPRESSION:
        switch (index)
        {
        case 0: return { gWallDepression,      ANOMALYID::Wall_Depression };
        case 1: return { gLightDepression,     ANOMALYID::Light_Depression };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Distort };
        case 3: return { gDoorLight,           ANOMALYID::Door_LightChange };
        case 4: return { gLiftDepression,      ANOMALYID::Lift_Depression };
        case 5: return { gSound,               ANOMALYID::Sound_Depression };
        default: return { nullptr,             ANOMALYID::None };
        }

    case ILLNESSES::DEMENTIA:
        switch (index)
        {
        case 0: return { gWallDementia,        ANOMALYID::Wall_Dementia };
        case 1: return { gLightDementia,       ANOMALYID::Light_Dementia };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Glitch };
        case 3: return { gDoorLight,           ANOMALYID::Door_LightChange };
        case 4: return { gLiftDementia,        ANOMALYID::Lift_Dementia };
        case 5: return { gSound,               ANOMALYID::Sound_Dementia };
        default: return { nullptr,             ANOMALYID::None };
        }

    case ILLNESSES::SCHIZOPHRENIA:
        switch (index)
        {
        case 0: return { gWallSchizophrenia,   ANOMALYID::Wall_Schizophrenia };
        case 1: return { gLightSchizophrenia,  ANOMALYID::Light_Schizophrenia };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Glitch };
        case 3: return { gDoorHand,            ANOMALYID::Door_HandSlam };
        case 4: return { gLiftSchizophrenia,   ANOMALYID::Lift_Schizophrenia };
        case 5: return { gSound,               ANOMALYID::Sound_Schizophrenia };
        default: return { nullptr,             ANOMALYID::None };
        }

    case ILLNESSES::AIW_SYNDROME:
        switch (index)
        {
        case 0: return { gWallAIW,             ANOMALYID::Wall_AIW };
        case 1: return { gLightAIW,            ANOMALYID::Light_AIW };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Distort };
        case 3: return { gDoorPosition,        ANOMALYID::Door_Position };
        case 4: return { gLiftAIW,             ANOMALYID::Lift_AIW };
        case 5: return { gSound,               ANOMALYID::Sound_AIW };
        default: return { nullptr,             ANOMALYID::None };
        }

    case ILLNESSES::INSOMNIA:
        switch (index)
        {
        case 0: return { gWallInsomnia,        ANOMALYID::Wall_Insomnia };
        case 1: return { gLightInsomnia,       ANOMALYID::Light_Insomnia };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Distort };
        case 3: return { gDoorLight,           ANOMALYID::Door_LightChange };
        case 4: return { gLiftInsomnia,        ANOMALYID::Lift_Insomnia };
        case 5: return { gSound,               ANOMALYID::Sound_Insomnia };
        default: return { nullptr,             ANOMALYID::None };
        }

    case ILLNESSES::OCD:
        switch (index)
        {
        case 0: return { gWallOCD,             ANOMALYID::Wall_OCD };
        case 1: return { gLightOCD,            ANOMALYID::Light_OCD };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Shift };
        case 3: return { gDoorPosition,        ANOMALYID::Door_Position };
        case 4: return { gLiftOCD,             ANOMALYID::Lift_OCD };
        case 5: return { gSound,               ANOMALYID::Sound_OCD };
        default: return { nullptr,             ANOMALYID::None };
        }

    case ILLNESSES::SCOTOPHOBIA:
        switch (index)
        {
        case 0: return { gWallScotophobia,     ANOMALYID::Wall_Scotophobia };
        case 1: return { gLightScotophobia,    ANOMALYID::Light_Scotophobia };
        case 2: return { gFrameGlitch,         ANOMALYID::Frame_Glitch };
        case 3: return { gDoorLight,           ANOMALYID::Door_LightChange };
        case 4: return { gLiftScotophobia,     ANOMALYID::Lift_Scotophobia };
        case 5: return { gSound,               ANOMALYID::Sound_Scotophobia };
        default: return { nullptr,             ANOMALYID::None };
        }

    default:
        return { nullptr, ANOMALYID::None };

    }
}

// Stores the anomaly entries
// Example:
// Illness Count      Symptom
// ------------------------------------------------
// [0] (Paranoia)     6 symptoms [1][2][3][4][5][6]
// [1] (Mania)        6 symptoms [1][2][3][4][5][6]
// [2] (Depression)   ...
// Usage:
// JournalEntries[0][0] -> texture and anomaly ID for Paranoia Light
// JournalEntries[0][1] -> texture and anomaly ID for Paranoia Frames
static SymptomStruct JournalEntries[ILLNESS_COUNT][SYMPTOM_COUNT];

// ============================================================
// OBSERVED ANOMALIES
// ============================================================

// Create a list of true/false values
// (int)ANOMALYID::None + 1 :   The total number of anomaly IDs, including None
// false                    :   Initialize all entries to false (not observed)
// ---------------------------------------------------------------------------------
// Usage:
// Journal_HasObserved(ANOMALYID::Wall_Paranoia) -> gObserved[(int)id] = true;
static std::vector<bool> gObserved ((int)ANOMALYID::None + 1, false);

// Used in all anomly files to report that the player has observed a specific anomaly
void Journal_ReportAnomaly(ANOMALYID id)
{
    if (id == ANOMALYID::None)
    {
        return;
    }

    int index = (int)id; // index for the gObserved vector, based on the anomaly ID enum value

    // Ensure index is within bounds
    if (index < 0 || index >= (int)gObserved.size())
    {
        return;
    }

	// Mark the anomaly as observed in the gObserved vector
    gObserved[index] = true;
}

// Check if an anomaly has been observed 
bool Journal_HasObserved(ANOMALYID id)
{
	// If there isnt any anomaly, return false
    if (id == ANOMALYID::None)
    {
        return false;
    }

	int index = (int)id; // index for the gObserved vector, based on the anomaly ID enum value

    // Ensure index is within bounds
    if (index < 0 || index >= (int)gObserved.size())
    {
        return false;
    }

	// Return the observed status of the anomaly from the gObserved vector
    return gObserved[index];
}

void Journal_Load()
{
    symbolsFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    journalFontId = AEGfxCreateFont(Assets::Fonts::Handwriting1, 50);

    JournalBook = LoadTextureChecked(Assets::Equipment::JournalBook);
	Undiscovered = LoadTextureChecked(Assets::Equipment::Undiscovered);
    gSound = LoadTextureChecked(Assets::Equipment::Sound);

	gWallParanoia = LoadTextureChecked(Assets::Equipment::WallParanoia);
	gWallMania = LoadTextureChecked(Assets::Equipment::WallMania);
	gWallDepression = LoadTextureChecked(Assets::Equipment::WallDepression);
	gWallDementia = LoadTextureChecked(Assets::Equipment::WallDementia);
	gWallSchizophrenia = LoadTextureChecked(Assets::Equipment::WallSchizophrenia);
	gWallAIW = LoadTextureChecked(Assets::Equipment::WallAIW);
	gWallInsomnia = LoadTextureChecked(Assets::Equipment::WallInsomnia);
	gWallOCD = LoadTextureChecked(Assets::Equipment::WallOCD);
	gWallScotophobia = LoadTextureChecked(Assets::Equipment::WallScotophobia);

    gFrameGlitch = LoadTextureChecked(Assets::Equipment::FrameGlitch);
    gFrameShift = LoadTextureChecked(Assets::Equipment::FrameShift);
    gFrameDistort = LoadTextureChecked(Assets::Equipment::FrameDistort);

	gDoorHand = LoadTextureChecked(Assets::Equipment::DoorHand);
    gDoorLight = LoadTextureChecked(Assets::Equipment::DoorLight);
    gDoorPosition = LoadTextureChecked(Assets::Equipment::DoorPosition);

    gLiftParanoia = LoadTextureChecked(Assets::Equipment::LiftParanoia);
    gLiftMania = LoadTextureChecked(Assets::Equipment::LiftMania);
    gLiftDepression = LoadTextureChecked(Assets::Equipment::LiftDepression);
    gLiftDementia = LoadTextureChecked(Assets::Equipment::LiftDementia);
    gLiftSchizophrenia = LoadTextureChecked(Assets::Equipment::LiftSchizophrenia);
    gLiftAIW = LoadTextureChecked(Assets::Equipment::LiftAIW);
    gLiftInsomnia = LoadTextureChecked(Assets::Equipment::LiftInsomnia);
    gLiftOCD = LoadTextureChecked(Assets::Equipment::LiftOCD);
    gLiftScotophobia = LoadTextureChecked(Assets::Equipment::LiftScotophobia);

    gLightParanoia = LoadTextureChecked(Assets::Equipment::LightParanoia);
    gLightMania = LoadTextureChecked(Assets::Equipment::LightMania);
    gLightDepression = LoadTextureChecked(Assets::Equipment::LightDepression);
    gLightDementia = LoadTextureChecked(Assets::Equipment::LightDementia);
    gLightSchizophrenia = LoadTextureChecked(Assets::Equipment::LightSchizophrenia);
    gLightAIW = LoadTextureChecked(Assets::Equipment::LightAIW);
    gLightInsomnia = LoadTextureChecked(Assets::Equipment::LightInsomnia);
    gLightOCD = LoadTextureChecked(Assets::Equipment::LightOCD);
    gLightScotophobia = LoadTextureChecked(Assets::Equipment::LightScotophobia);

	// Load journal text data for each illness
    for (int i = 0; i < (int)IllnessList.size(); ++i)
    {
        ILLNESSES illness = IllnessList[i];
        const char* FilePath = GetJournalTextFilePath(illness);

        if (LoadJournalTextFile(FilePath, GetFromText[i]))
        {
            std::cout << "Loaded journal: "
                << GetFromText[i].title
                << std::endl;
        }
    }
}

// ============================================================
// KEYS & NAVIGATION 
// ============================================================

void Journal_Update()
{
    // Toggle book with I
    if (AEInputCheckTriggered(AEVK_I))
    {
        gOpen = !gOpen;
        AudioManager_PlaySFX(SFX_JOURNAL_OPEN, 1.0f);

		// Reset right page 0 when opening the journal
        if (gOpen)
        {
            gRightPage = 0;
        }
    }

    if (!gOpen) return;

    if (AEInputCheckTriggered(AEVK_UP))
    {
        gSelected--;
        gRightPage = 0;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    if (AEInputCheckTriggered(AEVK_DOWN))
    {
        gSelected++;
        gRightPage = 0;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    ClampSelected();

    if (AEInputCheckTriggered(AEVK_LEFT))
    {
        if (gRightPage > 0)
        {
            --gRightPage;
            AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
        }
    }

    if (AEInputCheckTriggered(AEVK_RIGHT))
    {
        if (gRightPage < 1)
        {
            ++gRightPage;
            AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
        }
    }
}

// ============================================================
// BOOK DRAW (two pages)
// ============================================================

void Journal_Draw(AEGfxVertexList* squareMesh)
{
    if (!gOpen) return;

    // background
    DrawTextureMesh(squareMesh,
        JournalBook,
        0.0f * UI_SCALE,
        0.0f * UI_SCALE,
        1800.0f * UI_SCALE,
        1050.0f * UI_SCALE,
        1.0f);

    if (journalFontId < 0) return;

    auto X = [&](float x) { return (x * UI_SCALE) / SCREEN_WIDTH_HALF; };
    auto Y = [&](float y) { return (y * UI_SCALE) / SCREEN_HEIGHT_HALF; };

    // Dimensions -----------------------------------------------------------------------------------------
    float HeaderY       = 380.0f;
    float SubHeaderY    = 170.0f;

    // Pages X positions
    float LeftStartX    = -700.0f;
    float RightStartX   = 80.0f;

    // Anomalies list
    float ListY         = 230.0f;
    float ListStep      = 70.0f;

    // Image grid layout
    float imgW          = 200.0f;
    float imgH          = 220.0f;

    float colGap        = 220.0f;
    float rowGap        = 220.0f;

    float gridStartX    = (RightStartX - 30.0f) + imgW * 0.5f;
    float gridStartY    = (ListY)-imgH * 0.5f;

    float tiltAngle     = 0.12f;

    // Sizing -----------------------------------------------------------------------------------------
    float Header        = 1.2f;
    float SubHeader     = 0.90f;
    float text          = 0.55f;
    float list          = 0.90f;
    float listSelected  = 0.80f;

    // ============================================================
    // LEFT SIDE
    // ============================================================
    
    // Header 
    AEGfxPrint(journalFontId, "Illnesses", 
        X(LeftStartX), Y(HeaderY),
        Header,
        0, 0, 0, // black
        1);

	// Description
    AEGfxPrint(journalFontId, "Use arrow keys to select an illness",
        X(LeftStartX), Y(HeaderY - 60.0f),
        text,
        0.2f, 0.2f, 0.2f,
        1);

    for (int i = 0; i < (int)IllnessList.size(); ++i)
    {
        const char* name = GetFromText[i].title.c_str();
        float IllnessY = ListY - i * ListStep;

        if (i == gSelected)
        {
            AEGfxPrint(journalFontId, name, 
                X(LeftStartX), Y(IllnessY),
                list,
                0.55f, 0.05f, 0.05f, // bright red
                1);
        }
        else
        {
            AEGfxPrint(journalFontId, name, 
                X(LeftStartX), Y(IllnessY),
                listSelected,
                0.2f, 0.2f, 0.2f, // dark gray
                1);
        }
    }
    // ------------------------------------------------------------------------------------------------------

    // ============================================================
    // RIGHT SIDE
    // Consists of 2 pages. Toggle RIGHT LEFT ARROW keys.
    // ============================================================
    ClampSelected();
    ILLNESSES chosen = IllnessList[gSelected];
    TextStruct& TextData = GetFromText[gSelected];

    // Mouse position
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWorldX = (float)mouseX - SCREEN_WIDTH_HALF;
    float mouseWorldY = SCREEN_HEIGHT_HALF - (float)mouseY;

	// Hover state reset
    hasHovered = false;
    hoveredIndex = -1;

    // Header
    AEGfxPrint(journalFontId, TextData.title.c_str(),
        X(RightStartX), Y(HeaderY),
        Header,
        0.55f, 0.05f, 0.05f, // bright red
        1);

    // ---------------------------------------------------------------------------
    // PAGE 1
    // ---------------------------------------------------------------------------
    if (gRightPage == 0)
    {
        // Description
        AEGfxPrint(journalFontId, TextData.desc.c_str(),
            X(RightStartX), Y(HeaderY - 60.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1);
        AEGfxPrint(journalFontId, TextData.desc2.c_str(),
            X(RightStartX), Y(HeaderY - 100.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1);

        // Subheader
        AEGfxPrint(journalFontId, "Symptoms",
            X(RightStartX), Y(SubHeaderY),
            SubHeader,
            0, 0, 0,
            1);

        AEGfxPrint(journalFontId, "Only 3 out of all will occur during a shift",
            X(RightStartX), Y(SubHeaderY - 55.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1);

        // Draw anomalies in a 1x2 grid
        for (int i = 0; i < 2; ++i)
        {
			SymptomStruct entry = GetJournalSymptomEntry(chosen, i); // chosen illness, symptom index
            AEGfxTexture* SymptomImages = entry.texture;
            ANOMALYID SymptomID         = entry.anomalyId;

            int col = i % 2;
            int row = i / 2;

            float x = gridStartX + col * colGap;
            float y = (SubHeaderY - 240.0f);

            bool hovered    = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);
            bool observed   = Journal_HasObserved(SymptomID);

            // Check if mouse is over this image slot
            if (hovered)
            {
                hasHovered = true;
                hoveredIndex = i;
            }

            if (observed && SymptomImages)
            {
                // Discovered anomaly image
                // Rotate slightly when hovered
                if (hovered)
                {
                    DrawTextureMeshRotated(squareMesh, SymptomImages, x, y, imgW, imgH, tiltAngle, 1.0f);
                }
                else
                {
                    DrawTextureMesh(squareMesh, SymptomImages, x, y, imgW, imgH, 1.0f);
                }
            }
            else
            {
                // Undiscovered anomaly image
                // Still allow hover detection, but do not rotate
                DrawTextureMesh(squareMesh, Undiscovered, x, y, imgW, imgH, 1.0f);
            }
        } // ----------------------------------------------------------------------------

        // Navigate to next page controls
        AEGfxPrint(symbolsFontId, ">",
            X(650.0f), Y(-370.0f),
            SubHeader,
            0, 0, 0,
            1);
    }

    // ---------------------------------------------------------------------------
    // PAGE 2
    // ---------------------------------------------------------------------------
    else
    {
        // Draw anomalies in a 2x2 grid
        for (int i = 2; i < 6; ++i)
        {
            SymptomStruct entry = GetJournalSymptomEntry(chosen, i);
            AEGfxTexture* SymptomImages = entry.texture;
            ANOMALYID SymptomID         = entry.anomalyId;

            int col = (i - 2) % 2;
            int row = (i - 2) / 2;

            float x = gridStartX + col * colGap;
            float y = (150.0f) - row * rowGap;

            bool hovered    = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);
            bool observed   = Journal_HasObserved(SymptomID);

            // Check if mouse is over this image slot
            if (hovered)
            {
                hasHovered = true;
                hoveredIndex = i;
            }

            if (observed && SymptomImages)
            {
                // Discovered anomaly image
                // Rotate slightly when hovered
                if (hovered)
                {
                    DrawTextureMeshRotated(squareMesh, SymptomImages, x, y, imgW, imgH, tiltAngle, 1.0f);
                }
                else
                {
                    DrawTextureMesh(squareMesh, SymptomImages, x, y, imgW, imgH, 1.0f);
                }
            }
            else
            {
                // Undiscovered anomaly image
                // Still allow hover detection, but do not rotate
                DrawTextureMesh(squareMesh, Undiscovered, x, y, imgW, imgH, 1.0f);
            }
        } //----------------------------------------------------------------------------

        // Navigate to previous page controls
        AEGfxPrint(symbolsFontId, "<",
            X(650.0f), Y(-370.0f),
            SubHeader,
            0, 0, 0,
            1);

    }

    // ------------------------------------------------------------
    // Hover Info Area
    // Displays text describing the anomaly when the player hovers
    // over a discovered image in the journal grid.
    // ------------------------------------------------------------

    // Default text shown when nothing is hovered
    const char* HoverText = "Hover an entry to inspect";

    // Check if the mouse is hovering over a valid grid slot
    if (hoveredIndex >= 0 && hoveredIndex < 6)
    {
        // Retrieve the anomaly entry for this illness + slot
        SymptomStruct hoveredEntry = GetJournalSymptomEntry(chosen, hoveredIndex);

        // If the anomaly exists AND the player has discovered it
        if (hoveredEntry.anomalyId != ANOMALYID::None &&
            Journal_HasObserved(hoveredEntry.anomalyId))
        {
            // Show the anomaly description text
            HoverText = TextData.symptomText[hoveredIndex].c_str();
        }
        else
        {
            // Undiscovered anomaly -> hide the description
            HoverText = "???";
        }
    }

    AEGfxPrint(journalFontId, HoverText,
        X(RightStartX), Y(-300.0f),
        text,
        0, 0, 0,
        1);
}


void Journal_Unload()
{
    // unload textures
    UnloadTextureSafe(JournalBook);
    UnloadTextureSafe(Undiscovered);
    UnloadTextureSafe(gSound);

    UnloadTextureSafe(gWallParanoia);
    UnloadTextureSafe(gWallMania);
    UnloadTextureSafe(gWallSchizophrenia);
    UnloadTextureSafe(gWallDepression);
    UnloadTextureSafe(gWallDementia);
    UnloadTextureSafe(gWallAIW);
    UnloadTextureSafe(gWallInsomnia);
    UnloadTextureSafe(gWallOCD);
    UnloadTextureSafe(gWallScotophobia);

    UnloadTextureSafe(gDoorHand);
    UnloadTextureSafe(gDoorLight);
    UnloadTextureSafe(gDoorPosition);

    UnloadTextureSafe(gLiftParanoia);
    UnloadTextureSafe(gLiftMania);
    UnloadTextureSafe(gLiftSchizophrenia);
    UnloadTextureSafe(gLiftDepression);
    UnloadTextureSafe(gLiftDementia);
    UnloadTextureSafe(gLiftAIW);
    UnloadTextureSafe(gLiftInsomnia);
    UnloadTextureSafe(gLiftOCD);
    UnloadTextureSafe(gLiftScotophobia);

    UnloadTextureSafe(gLightParanoia);
    UnloadTextureSafe(gLightMania);
    UnloadTextureSafe(gLightSchizophrenia);
    UnloadTextureSafe(gLightDepression);
    UnloadTextureSafe(gLightDementia);
    UnloadTextureSafe(gLightAIW);
    UnloadTextureSafe(gLightInsomnia);
    UnloadTextureSafe(gLightOCD);
    UnloadTextureSafe(gLightScotophobia);

    UnloadTextureSafe(gFrameGlitch);
    UnloadTextureSafe(gFrameShift);
    UnloadTextureSafe(gFrameDistort);

    // destroy font
    if (journalFontId >= 0)
    {
        AEGfxDestroyFont(journalFontId);
        journalFontId = -1;
    }

    // reset UI state
    gOpen = false;
    gSelected = 0;
}

// ============================================================
// PUBLIC GETTERS
// ============================================================

bool Journal_IsOpen()
{
    return gOpen;
}

void Journal_Clear()
{
    std::fill(gObserved.begin(), gObserved.end(), false);
}

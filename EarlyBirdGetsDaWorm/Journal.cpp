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
const float UI_SCALE    = 0.7f;        // journal UI bigger or smaller

static bool Open                = false;
static bool hasHovered          = false;
static bool HoverTab1           = false;
static bool HoverTab2           = false;

static bool  JournalHasUnread = false;
static float JournalAlertTimer = 0.0f;

static int  Selected            = 0;
static int  RightPage           = 0;    // 0 = page 1, 1 = page 2
static int  HoveredIndex        = -1;

constexpr int ILLNESS_COUNT = 9;
constexpr int SYMPTOM_COUNT = 6;

static f32 JournalIconAlpha         = 1.0f;
static bool JournalIconHover        = false;
static f32 JournalIconHoverSpeed    = 10.0f;

// ============================================================
// DECLARATIONS
// ============================================================

// Dimensions -------------------------------------------------
static f32 HeaderY;
static f32 SubHeaderY;

// Tab positions
static f32 TabX;
static f32 Tab1Y;
static f32 Tab2Y;

// Pages X positions
static f32 LeftStartX;
static f32 RightStartX;

// Anomalies list
static f32 ListY;
static f32 ListStep;
static f32 IllnessHitX;

// Image grid layout
static f32 imgW;
static f32 imgH;

static f32 colGap;
static f32 rowGap;

static f32 gridStartX;
static f32 gridStartY;

static f32 tiltAngle;

// Sizing ----------------------------------------------------
static f32 Header;
static f32 SubHeader;
static f32 text;
static f32 list;
static f32 listSelected;

static f32 IllnessHitW;
static f32 IllnessHitH;

static f32 TabW;
static f32 TabH;

void Journal_Initialize()
{
    // Layout
    HeaderY = 380.0f;
    SubHeaderY = 170.0f;

    // Tab positions
    TabX = 870.0f;
    Tab1Y = 310.0f;
    Tab2Y = 180.0f;

    // Pages X positions
    LeftStartX = -700.0f;
    RightStartX = 80.0f;

    // Anomalies list
    ListY = 230.0f;
    ListStep = 70.0f;
    IllnessHitX = LeftStartX + 140.0f;
    IllnessHitW = 380.0f;
    IllnessHitH = ListStep * 0.9f;

    // Image grid layout
    imgW = 200.0f;
    imgH = 220.0f;

    colGap = 220.0f;
    rowGap = 220.0f;

    gridStartX = (RightStartX - 30.0f) + imgW * 0.5f;
    gridStartY = ListY - imgH * 0.5f;

    tiltAngle = 0.12f;

    // Sizing ----------------------------------------------------
    Header = 1.2f;
    SubHeader = 0.90f;
    text = 0.55f;
    list = 0.90f;
    listSelected = 0.80f;

    // Tabs
    TabW = 350.0f;
    TabH = 120.0f;
}

// ============================================================
// TEXTURES
// ============================================================
static AEGfxTexture* JournalIcon = nullptr;
static AEGfxTexture* AlertIcon = nullptr;
static AEGfxTexture* JournalBack = nullptr;
static AEGfxTexture* JournalFront = nullptr; 
static AEGfxTexture* TabEntry1 = nullptr;
static AEGfxTexture* TabEntry2 = nullptr;

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

static AEGfxTexture* gFrameShakes = nullptr;
static AEGfxTexture* gFrameMania = nullptr;
static AEGfxTexture* gFrameDepression = nullptr;
static AEGfxTexture* gFrameAIW = nullptr;
static AEGfxTexture* gFrameInsomnia = nullptr;
static AEGfxTexture* gFrameOCD = nullptr;

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

// Is the mouse point within a rectangle centered at (cx, cy)?
static bool Journal_PointInRect(float px, float py, float cx, float cy, float w, float h)
{
    float left = cx - w * 0.5f;
    float right = cx + w * 0.5f;
    float bottom = cy - h * 0.5f;
    float top = cy + h * 0.5f;

    return (px >= left && px <= right && py >= bottom && py <= top);
}

static void ClampSelected()
{
    if (Selected < 0)
        Selected = 0;

    if (Selected >= (int)IllnessList.size())
        Selected = (int)IllnessList.size() - 1;
}

static void DrawTab(
    AEGfxVertexList* mesh,
    AEGfxTexture* tex,
    s8 fontId,
    const char* label,
    float x, float y,
    bool hovered,
    bool selected)
{
    float scale = 1.0f;
    if (selected) scale += 0.05f;
    if (hovered)  scale += 0.06f;

    if (tex)
    {
        DrawTextureMesh(mesh,
            tex,
            x * UI_SCALE,
            y * UI_SCALE,
            TabW * scale * UI_SCALE,
            TabH * scale * UI_SCALE,
            1.0f);
    }

    AEGfxPrint(fontId, label,
        ((x-45.0f) * UI_SCALE) / SCREEN_WIDTH_HALF,
        ((y-15.0f) * UI_SCALE) / SCREEN_HEIGHT_HALF,
        SubHeader * scale,
        0.55f, 0.05f, 0.05f,
        1.0f);
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
	// If file doesn't exist or fails to open, return false
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
        case 2: return { gFrameShakes,        ANOMALYID::Frame_Shakes };
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
        case 2: return { gFrameMania,         ANOMALYID::Frame_Mania };
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
        case 2: return { gFrameDepression,     ANOMALYID::Frame_Depression };
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
        case 2: return { gFrameShakes,         ANOMALYID::Frame_Shakes };
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
        case 2: return { gFrameShakes,         ANOMALYID::Frame_Shakes };
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
        case 2: return { gFrameAIW,            ANOMALYID::Frame_AIW };
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
        case 2: return { gFrameInsomnia,       ANOMALYID::Frame_Insomnia };
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
        case 2: return { gFrameOCD,            ANOMALYID::Frame_OCD };
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
        case 2: return { gFrameShakes,         ANOMALYID::Frame_Shakes };
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
// Journal_HasObserved(ANOMALYID::Wall_Paranoia) -> Observed[(int)id] = true;
static std::vector<bool> Observed ((int)ANOMALYID::None + 1, false);
static std::vector<bool> Unread((int)ANOMALYID::None + 1, false);

// Used in all anomly files to report that the player has observed a specific anomaly
void Journal_ReportAnomaly(ANOMALYID id)
{
    if (id == ANOMALYID::None)
    {
        return;
    }

    int index = (int)id; // index for the Observed vector, based on the anomaly ID enum value

    // Ensure index is within bounds
    if (index < 0 || index >= (int)Observed.size())
    {
        return;
    }

    // Only trigger unread alert the first time it is discovered
    if (!Observed[index])
    {
        Observed[index] = true;
        Unread[index] = true;
        JournalHasUnread = true;
    }
}

// Check if an anomaly has been observed 
bool Journal_HasObserved(ANOMALYID id)
{
	// If there isn't any anomaly, return false
    if (id == ANOMALYID::None)
    {
        return false;
    }

	int index = (int)id; // index for the Observed vector, based on the anomaly ID enum value

    // Ensure index is within bounds
    if (index < 0 || index >= (int)Observed.size())
    {
        return false;
    }

	// Return the observed status of the anomaly from the Observed vector
    return Observed[index];
}

bool Journal_EvidenceHasUnread(ANOMALYID id)
{
    if (id == ANOMALYID::None)
    {
        return false;
    }

    int index = (int)id;

    if (index < 0 || index >= (int)Unread.size())
    {
        return false;
    }

    return Unread[index];
}

bool Journal_IllnessHasUnread(ILLNESSES illness)
{
    for (int i = 0; i < SYMPTOM_COUNT; ++i)
    {
        SymptomStruct entry = GetJournalSymptomEntry(illness, i);

        if (entry.anomalyId != ANOMALYID::None &&
            Journal_EvidenceHasUnread(entry.anomalyId))
        {
            return true;
        }
    }

    return false;
}

bool Journal_PageHasUnread(ILLNESSES illness, int page)
{
    int start = (page == 0) ? 0 : 2;
    int end = (page == 0) ? 2 : 6;

    for (int i = start; i < end; ++i)
    {
        SymptomStruct entry = GetJournalSymptomEntry(illness, i);

        if (entry.anomalyId != ANOMALYID::None &&
            Journal_EvidenceHasUnread(entry.anomalyId))
        {
            return true;
        }
    }

    return false;
}

bool Journal_HasNewContent()
{
    for (int i = 0; i < (int)Unread.size(); ++i)
    {
        if (Unread[i])
        {
            return true;
        }
    }
    return false;
}

void Journal_MarkAsRead(ANOMALYID id)
{
    if (id == ANOMALYID::None)
    {
        return;
    }

    int index = (int)id;

    if (index < 0 || index >= (int)Unread.size())
    {
        return;
    }

    Unread[index] = false;
    JournalHasUnread = Journal_HasNewContent();
}

// ============================================================
void Journal_Load()
{
    symbolsFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    journalFontId = AEGfxCreateFont(Assets::Fonts::Handwriting1, 50);

    AlertIcon = LoadTextureChecked(Assets::Equipment::AlertIcon);
    JournalIcon = LoadTextureChecked(Assets::Equipment::JournalIcon);
    JournalBack = LoadTextureChecked(Assets::Equipment::JournalBack);
    JournalFront = LoadTextureChecked(Assets::Equipment::JournalFront);
    TabEntry1 = LoadTextureChecked(Assets::Equipment::Tab);
    TabEntry2 = LoadTextureChecked(Assets::Equipment::Tab);

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

    gFrameShakes = LoadTextureChecked(Assets::Equipment::FrameShakes);
    gFrameMania = LoadTextureChecked(Assets::Equipment::FrameMania);
    gFrameDepression = LoadTextureChecked(Assets::Equipment::FrameDepression);
    gFrameAIW = LoadTextureChecked(Assets::Equipment::FrameAIW);
    gFrameInsomnia = LoadTextureChecked(Assets::Equipment::FrameInsomnia);
    gFrameOCD = LoadTextureChecked(Assets::Equipment::FrameOCD);

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

void Journal_Update(bool liftActive, bool guideActive, bool pagerActive, f32 dt)
{
    f32 journalDt{ (f32)AEFrameRateControllerGetFrameTime() };

    if (JournalHasUnread)
        JournalAlertTimer += journalDt;
    else
        JournalAlertTimer = 0.0f;

    // Small Icon Hover and Select
    JournalIconHover = IsAreaHovered(
        AEGfxGetWinMinX() + 1500.0f,
        AEGfxGetWinMinY() + 750.0f,
        100.0f,
        63.0f);

    HoverAnimation(JournalIconAlpha, JournalIconHover, JournalIconHoverSpeed, dt, 1.0f, 0.5f);

    static bool justToggled = false;

    bool clicked = AEInputCheckTriggered(AEVK_LBUTTON);
    bool pressedI = AEInputCheckTriggered(AEVK_I);

    bool OutsideJournal = !IsAreaClickedByMouse(
        0.0f * UI_SCALE, 0.0f * UI_SCALE,
        2000.0f * UI_SCALE, 1050.0f * UI_SCALE
    );

    // OPEN
    if (((JournalIconHover && clicked) || pressedI)
        && !liftActive && !guideActive && !pagerActive)
    {
        Open = !Open;
        justToggled = true;

        AudioManager_PlaySFX(SFX_JOURNAL_OPEN, 1.0f);

        if (Open)
            RightPage = 0;
    }

    // EARLY EXIT
    if (!Open)
        return;

    // CLOSE
    if (!justToggled && OutsideJournal && clicked)
    {
        AudioManager_PlaySFX(SFX_JOURNAL_OPEN, 1.0f);
        Open = false;
    }

    // Reset AFTER all logic
    if (justToggled)
    {
        justToggled = false;
    }


    // ------------------------------------------------------------
    // Mouse position
    // ------------------------------------------------------------
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWorldX = (float)mouseX - SCREEN_WIDTH_HALF;
    float mouseWorldY = SCREEN_HEIGHT_HALF - (float)mouseY;

    // ------------------------------------------------------------
    // Tab hover
    // ------------------------------------------------------------
    float tab1HoverW = TabW * ((RightPage == 0) ? 1.05f : 1.0f) * UI_SCALE;
    float tab1HoverH = TabH * ((RightPage == 0) ? 1.05f : 1.0f) * UI_SCALE;

    float tab2HoverW = TabW * ((RightPage == 1) ? 1.05f : 1.0f) * UI_SCALE;
    float tab2HoverH = TabH * ((RightPage == 1) ? 1.05f : 1.0f) * UI_SCALE;

    HoverTab1 = IsAreaHovered(
        TabX * UI_SCALE,
        Tab1Y * UI_SCALE,
        tab1HoverW,
        tab1HoverH);

    HoverTab2 = IsAreaHovered(
        TabX * UI_SCALE,
        Tab2Y * UI_SCALE,
        tab2HoverW,
        tab2HoverH);

    // ------------------------------------------------------------
    // Mouse click
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (HoverTab1 && RightPage != 0)
        {
            RightPage = 0;
            AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
        }
        else if (HoverTab2 && RightPage != 1)
        {
            RightPage = 1;
            AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
        }
        else
        {
            for (int i = 0; i < (int)IllnessList.size(); ++i)
            {
                float illnessY = ListY - i * ListStep;

                bool hovered = IsAreaHovered(
                    IllnessHitX * UI_SCALE,
                    illnessY * UI_SCALE,
                    IllnessHitW * UI_SCALE,
                    IllnessHitH * UI_SCALE);

                if (hovered)
                {
                    if (Selected != i)
                    {
                        Selected = i;
                        RightPage = 0;
                        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
                    }
                    break;
                }
            }
        }
    }

    // ------------------------------------------------------------
    // Keyboard illness selection
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_UP))
    {
        --Selected;
        RightPage = 0;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    if (AEInputCheckTriggered(AEVK_DOWN))
    {
        ++Selected;
        RightPage = 0;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    ClampSelected();

    // ------------------------------------------------------------
    // Keyboard page switch
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_LEFT) && RightPage > 0)
    {
        --RightPage;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    if (AEInputCheckTriggered(AEVK_RIGHT) && RightPage < 1)
    {
        ++RightPage;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }
}

// ============================================================
// JOURNAL ICON
// ============================================================

void Journal_DrawSmallIcon(AEGfxVertexList* squareMesh)
{
    f32 iconX = AEGfxGetWinMinX() + 1500.0f;
    f32 iconY = AEGfxGetWinMinY() + 750.0f;

    DrawTextureMesh(squareMesh, JournalIcon, iconX, iconY, 100.0f, 63.0f, JournalIconAlpha);

    // Bouncing Alert Icon
    if (JournalHasUnread && AlertIcon)
    {
        f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

        DrawTextureMesh(squareMesh,
            AlertIcon,
            iconX + 25.0f,
            iconY + 25.0f + bounceY,
            25.0f,
            25.0f,
            1.0f);
    }

    if (JournalIconHover && !Open)
    {
        AEGfxPrint(symbolsFontId, "Journal",
            textPosition(-30.0f, -380.0f).first,
            textPosition(-30.0f, -380.0f).second,
            1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f);
    }
}

// ============================================================
// BOOK DRAW (two pages)
// ============================================================
void Journal_Draw(AEGfxVertexList* squareMesh)
{
    if (!Open) return;
    if (journalFontId < 0) return;

    auto X = [&](float x) { return (x * UI_SCALE) / SCREEN_WIDTH_HALF; };
    auto Y = [&](float y) { return (y * UI_SCALE) / SCREEN_HEIGHT_HALF; };

    // ------------------------------------------------------------
    // Mouse position
    // ------------------------------------------------------------
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWorldX = (float)mouseX - SCREEN_WIDTH_HALF;
    float mouseWorldY = SCREEN_HEIGHT_HALF - (float)mouseY;

    // ------------------------------------------------------------
    // Current illness data
    // ------------------------------------------------------------
    ClampSelected();
    ILLNESSES chosen = IllnessList[Selected];
    TextStruct& TextData = GetFromText[Selected];
    bool Tab1HasUnread = Journal_PageHasUnread(chosen, 0);
    bool Tab2HasUnread = Journal_PageHasUnread(chosen, 1);

    // ------------------------------------------------------------
    // Hover reset for symptom images
    // ------------------------------------------------------------
    hasHovered = false;
    HoveredIndex = -1;

    // ============================================================
    // LAYERS
    // Back -> Tabs -> Front -> Text / Images
    // ============================================================

    // Book back
    DrawTextureMesh(squareMesh,
        JournalBack,
        0.0f * UI_SCALE,
        0.0f * UI_SCALE,
        1800.0f * UI_SCALE,
        1050.0f * UI_SCALE,
        1.0f);

    // Tabs
    DrawTab(squareMesh, TabEntry1, journalFontId, "PAGE I",
        TabX, Tab1Y,
        HoverTab1,
        RightPage == 0);

    DrawTab(squareMesh, TabEntry2, journalFontId, "PAGE II",
        TabX, Tab2Y,
        HoverTab2,
        RightPage == 1);

    // Bouncing Alert Icon
    if (Tab1HasUnread && AlertIcon)
    {
        f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

        DrawTextureMesh(squareMesh,
            AlertIcon,
            (TabX + 150.0f)             * UI_SCALE,
            (Tab1Y + 50.0f + bounceY)   * UI_SCALE,
            25.0f,
            25.0f,
            1.0f);
    }

    if (Tab2HasUnread && AlertIcon)
    {
        f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

        DrawTextureMesh(squareMesh,
            AlertIcon,
            (TabX + 150.0f)             * UI_SCALE,
            (Tab2Y + 50.0f + bounceY)   * UI_SCALE,
            25.0f,
            25.0f,
            1.0f);
    }

    // Front page
    DrawTextureMesh(squareMesh,
        JournalFront,
        0.0f * UI_SCALE,
        0.0f * UI_SCALE,
        1800.0f * UI_SCALE,
        1050.0f * UI_SCALE,
        1.0f);

    // ============================================================
    // LEFT SIDE
    // ============================================================
    AEGfxPrint(journalFontId, "Illnesses",
        X(LeftStartX), Y(HeaderY),
        Header,
        0, 0, 0,
        1.0f);

    AEGfxPrint(journalFontId, "Select an illness below",
        X(LeftStartX), Y(HeaderY - 60.0f),
        text,
        0.2f, 0.2f, 0.2f,
        1.0f);

    for (int i = 0; i < (int)IllnessList.size(); ++i)
    {
        const char* name = GetFromText[i].title.c_str();
        float illnessY = ListY - i * ListStep;

        bool hovered = IsAreaHovered(
            IllnessHitX * UI_SCALE,
            illnessY * UI_SCALE,
            IllnessHitW * UI_SCALE,
            IllnessHitH * UI_SCALE);

        bool selected = (i == Selected);
        bool illnessHasUnread = Journal_IllnessHasUnread(IllnessList[i]);

        float scale = 1.0f;
        if (selected) scale += 0.15f;
        if (hovered)  scale += 0.08f;

        if (selected)
        {
            AEGfxPrint(journalFontId, name,
                X(LeftStartX), Y(illnessY),
                list * scale,
                0.55f, 0.05f, 0.05f,
                1.0f);
        }
        else
        {
            AEGfxPrint(journalFontId, name,
                X(LeftStartX), Y(illnessY),
                listSelected * scale,
                0.2f, 0.2f, 0.2f,
                1.0f);
        }

        // Bouncing Alert Icon
        if (illnessHasUnread && AlertIcon)
        {
            // amplitude                   //frequency   
            f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

            float textWorldX = LeftStartX * UI_SCALE;
            float textWorldY = illnessY * UI_SCALE;

            DrawTextureMesh(squareMesh,
                AlertIcon,
                (textWorldX - 20.0f),
                (textWorldY + 15.0f) + bounceY,
                25.0f,
                25.0f,
                1.0f);
        }
    }

    // ============================================================
    // RIGHT SIDE
    // ============================================================
    AEGfxPrint(journalFontId, TextData.title.c_str(),
        X(RightStartX), Y(HeaderY),
        Header,
        0, 0, 0,
        1.0f);

    if (RightPage == 0)
    {
        AEGfxPrint(journalFontId, TextData.desc.c_str(),
            X(RightStartX), Y(HeaderY - 60.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1.0f);

        AEGfxPrint(journalFontId, TextData.desc2.c_str(),
            X(RightStartX), Y(HeaderY - 100.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1.0f);

        AEGfxPrint(journalFontId, "Symptoms",
            X(RightStartX), Y(SubHeaderY),
            SubHeader,
            0, 0, 0,
            1.0f);

        AEGfxPrint(journalFontId, "Only 3 out of all will occur during a shift",
            X(RightStartX), Y(SubHeaderY - 55.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1.0f);

        for (int i = 0; i < 2; ++i)
        {
            SymptomStruct entry = GetJournalSymptomEntry(chosen, i);
            AEGfxTexture* symptomTex = entry.texture;
            ANOMALYID symptomID = entry.anomalyId;

            int col = i % 2;
            float x = gridStartX + col * colGap;
            float y = SubHeaderY - 240.0f;

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);
            bool observed = Journal_HasObserved(symptomID);

            if (hovered)
            {
                hasHovered = true;
                HoveredIndex = i;

                if (observed)
                {
                    Journal_MarkAsRead(symptomID);
                }
            }

            bool hasUnread = Journal_EvidenceHasUnread(symptomID);

            if (observed && symptomTex)
            {
                if (hovered)
                    DrawTextureMeshRotated(squareMesh, symptomTex, x, y, imgW, imgH, tiltAngle, 1.0f);
                else
                    DrawTextureMesh(squareMesh, symptomTex, x, y, imgW, imgH, 1.0f);

                if (hasUnread)
                {
                              // amplitude                   //frequency
                    f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

                    DrawTextureMesh(squareMesh,
                        AlertIcon,
                        (x + imgW - 120.0f),
                        (y + imgH - 120.0f) + bounceY,
                        25.0f,
                        25.0f,
                        1.0f);
                }
            }
            else
            {
                DrawTextureMesh(squareMesh, Undiscovered, x, y, imgW, imgH, 1.0f);
            }
        }
    }
    else
    {
        for (int i = 2; i < 6; ++i)
        {
            SymptomStruct entry = GetJournalSymptomEntry(chosen, i);
            AEGfxTexture* symptomTex = entry.texture;
            ANOMALYID symptomID = entry.anomalyId;

            int col = (i - 2) % 2;
            int row = (i - 2) / 2;

            float x = gridStartX + col * colGap;
            float y = 130.0f - row * rowGap;

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);
            bool observed = Journal_HasObserved(symptomID);

            if (hovered)
            {
                hasHovered = true;
                HoveredIndex = i;

                if (observed)
                {
                    Journal_MarkAsRead(symptomID);
                }
            }

            bool hasUnread = Journal_EvidenceHasUnread(symptomID);

            if (observed && symptomTex)
            {
                if (hovered)
                    DrawTextureMeshRotated(squareMesh, symptomTex, x, y, imgW, imgH, tiltAngle, 1.0f);
                else
                    DrawTextureMesh(squareMesh, symptomTex, x, y, imgW, imgH, 1.0f);

                if (hasUnread)
                {
                    // amplitude                   //frequency
                    f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

                    DrawTextureMesh(squareMesh,
                        AlertIcon,
                        (x + imgW - 120.0f),
                        (y + imgH - 120.0f) + bounceY,
                        25.0f,
                        25.0f,
                        1.0f);
                }
            }
            else
            {
                DrawTextureMesh(squareMesh, Undiscovered, x, y, imgW, imgH, 1.0f);
            }
        }
    }

    // ============================================================
    // HOVER INFO AREA
    // ============================================================
    const char* HoverText = "Hover an entry to inspect";

    if (HoveredIndex >= 0 && HoveredIndex < 6)
    {
        SymptomStruct hoveredEntry = GetJournalSymptomEntry(chosen, HoveredIndex);

        if (hoveredEntry.anomalyId != ANOMALYID::None &&
            Journal_HasObserved(hoveredEntry.anomalyId))
        {
            HoverText = TextData.symptomText[HoveredIndex].c_str();
        }
        else
        {
            HoverText = "???";
        }
    }

    AEGfxPrint(journalFontId, HoverText,
        X(RightStartX), Y(-350.0f),
        text,
        0, 0, 0,
        1.0f);

    // ============================================================
	// "PRESS [I] TO CLOSE" INFO
    // ============================================================
    if (Open)
    {
        f32 infoX{ textPosition(-220.0f, -410.0f).first };
        f32 infoY{ textPosition(-220.0f, -410.0f).second };

        if (IsTutorialFinished() || !Doing_Tutorial()) {
            AEGfxPrint(symbolsFontId,
                "PRESS [I] TO CLOSE JOURNAL",
                infoX,
                infoY,
                1.0f,
                1.0f, 1.0f, 1.0f,
                1.0f);
        }
    }
    
}

void Journal_Unload()
{
    // unload textures
    UnloadTextureSafe(AlertIcon);
    UnloadTextureSafe(JournalIcon);
    UnloadTextureSafe(JournalBack);
    UnloadTextureSafe(JournalFront);
    UnloadTextureSafe(TabEntry1);
    UnloadTextureSafe(TabEntry2);

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

    UnloadTextureSafe(gFrameShakes);
    UnloadTextureSafe(gFrameMania);
    UnloadTextureSafe(gFrameDepression);
    UnloadTextureSafe(gFrameAIW);
    UnloadTextureSafe(gFrameInsomnia);
    UnloadTextureSafe(gFrameOCD);

    // destroy font
    if (journalFontId >= 0)
    {
        AEGfxDestroyFont(journalFontId);
        journalFontId = -1;
    }

    // reset UI state
    Open = false;
    Selected = 0;
}

// ============================================================
// PUBLIC GETTERS
// ============================================================

bool Journal_IsOpen()
{
    return Open;
}

void Journal_Clear()
{
    std::fill(Observed.begin(), Observed.end(), false);
    std::fill(Unread.begin(), Unread.end(), false);
    JournalHasUnread = false;
    JournalAlertTimer = 0.0f;
}

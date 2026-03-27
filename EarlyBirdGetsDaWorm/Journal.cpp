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
const float UI_SCALE    = 0.7f;     // journal UI bigger or smaller

static bool Open = false;
static bool hasHovered = false;
static bool ShowHowTo = true;      // first time open
static bool ClickHowTo = false;

static int  Selected = 0;
static int  HoveredIndex = -1;

static bool  JournalHasUnread = false;
static float JournalAlertTimer = 0.0f;

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

// Pages X positions
static f32 LeftStartX;
static f32 RightStartX;

// Illness tab layout
static f32 IllnessTabX;
static f32 IllnessTabW;
static f32 IllnessTabH;

// Symptom page layout
static f32 LeftPageSymptomX;
static f32 LeftPageSymptomY;
static f32 RightPageSymptomX;
static f32 RightPageSymptomY;

// Illness list
static f32 ListX;
static f32 ListY;
static f32 ListStep;
static f32 IllnessHitX;

// How to use tab layout
static f32 HowToTabX;
static f32 HowToTabY;
static f32 HowToTabW;
static f32 HowToTabH;

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
    // Layout ----------------------------------------------------
    HeaderY = 350.0f;
    SubHeaderY = HeaderY - 200.0f;

    // Pages X positions
	LeftStartX = -680.0f;
	RightStartX = 90.0f;

    // Illness list / tab layout  
    ListY = 330.0f;
    ListStep = 85.0f;

    IllnessTabX = -900.0f;
    IllnessTabW = 340.0f;
    IllnessTabH = 100.0f;

    IllnessHitX = IllnessTabX;
    IllnessHitW = IllnessTabW;
    IllnessHitH = IllnessTabH;

	// How to use tab layout
    HowToTabX = -150.0f;      
    HowToTabY = 450.0f;
    HowToTabW = 180.0f;
    HowToTabH = 130.0f;

    // Symptom image layout
    imgW = 190.0f;
    imgH = 210.0f;

    colGap = 210.0f;
    rowGap = 220.0f;
    
    // left page = 4 symptoms
    LeftPageSymptomX = LeftStartX + 300.0f;
    LeftPageSymptomY = -40.0f;

    // right page = 2 symptoms
    RightPageSymptomX = RightStartX + 70.0f;
    RightPageSymptomY = 180.0f;

    tiltAngle = 0.12f;

    // Sizing ----------------------------------------------------
    Header = 1.2f;
    SubHeader = 0.90f;
    text = 0.55f;
    list = 0.70f;
    listSelected = 0.72f;
}
// ============================================================
// TEXTURES
// ============================================================
static AEGfxTexture* JournalIcon = nullptr;
static AEGfxTexture* AlertIcon = nullptr;
static AEGfxTexture* JournalBack = nullptr;
static AEGfxTexture* JournalFront = nullptr; 
static AEGfxTexture* MarkEntry = nullptr;
static AEGfxTexture* TabEntry = nullptr;

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

// ============================================================
// JOURNAL ENTRIES [100 - 319]
// ============================================================

// ---------------------------------------------
// HOW TO USE PAGE 
// ---------------------------------------------
struct HelpTextData
{
    std::string title;
    std::vector<std::string> leftLines;
    std::vector<std::string> rightLines;
};

static bool LoadHelpText(const std::string& filePath, HelpTextData& data)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cout << "Failed to open help file\n";
        return false;
    }

    data.title.clear();
    data.leftLines.clear();
    data.rightLines.clear();

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if (key == "LEFT")
        {
            std::stringstream ss(value);
            std::string segment;

            while (std::getline(ss, segment, '|'))
            {
                data.leftLines.push_back(segment);
            }
        }
        else if (key == "RIGHT")
        {
            std::stringstream ss(value);
            std::string segment;

            while (std::getline(ss, segment, '|'))
            {
                data.rightLines.push_back(segment);
            }
        }
    }

    return true;
}

static HelpTextData HelpData;

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
        size_t eq = line.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

		// Extract key and value, then store in the appropriate field of outData
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
    MarkEntry = LoadTextureChecked(Assets::Equipment::Mark);
    TabEntry = LoadTextureChecked(Assets::Equipment::Tab);

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

    if (!LoadHelpText("Assets/Equipment/logs/Help.txt", HelpData))
    {
        std::cout << "Help text failed to load!\n";
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

    if (Open && ShowHowTo)
    {
        ClickHowTo = true;
        ShowHowTo = false;
    }


    // ------------------------------------------------------------
    // Mouse position
    // ------------------------------------------------------------
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWorldX = (float)mouseX - SCREEN_WIDTH_HALF;
    float mouseWorldY = SCREEN_HEIGHT_HALF - (float)mouseY;

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // ------------------------------------------------------------
        // How To Use tab click
        // ------------------------------------------------------------
        bool hoveredHowTo = IsAreaHovered(
            HowToTabX * UI_SCALE,
            HowToTabY * UI_SCALE,
            HowToTabW * UI_SCALE,
            HowToTabH * UI_SCALE);

        if (hoveredHowTo)
        {
            ClickHowTo = true;
            AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
        }

        // ------------------------------------------------------------
        // Illness tab click
        // ------------------------------------------------------------
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
                    AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
                }

                // IMPORTANT: leave how-to page when illness is clicked
                ClickHowTo = false;
                break;
            }
        }
    }

    // ------------------------------------------------------------
    // Keyboard illness selection
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_UP))
    {
        --Selected;
        ClickHowTo = false;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    if (AEInputCheckTriggered(AEVK_DOWN))
    {
        ++Selected;
        ClickHowTo = false;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    ClampSelected();
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

    // ------------------------------------------------------------
    // Hover reset for symptom images
    // ------------------------------------------------------------
    hasHovered = false;
    HoveredIndex = -1;

    // ============================================================
    // BACK BOOK
    // ============================================================
    DrawTextureMesh(squareMesh,
        JournalBack,
        0.0f * UI_SCALE,
        0.0f * UI_SCALE,
        1800.0f * UI_SCALE,
        1050.0f * UI_SCALE,
        1.0f);

    // ============================================================
    // ILLNESS TABS
    // ============================================================

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
        float scale = 0.75f;
        float drawX = IllnessTabX;

        // move left when hovered or selected
        if (hovered) drawX -= 10.0f;
        if (selected && !ClickHowTo) drawX -= 8.0f;

        // ============================================================
        // DRAW TAB IMAGE
        // ============================================================
        DrawTextureMesh(squareMesh,
            MarkEntry,
            drawX * UI_SCALE,
            illnessY * UI_SCALE,
            IllnessTabW * scale * UI_SCALE,
            IllnessTabH * scale * UI_SCALE,
            1.0f);

        // ============================================================
        // DRAW TEXT ON TOP
        // ============================================================
        float textX = drawX - 105.0f;
        float textY = illnessY - 12.0f;

        if (selected && !ClickHowTo)
        {
            AEGfxPrint(journalFontId, name,
                (textX * UI_SCALE) / SCREEN_WIDTH_HALF,
                (textY * UI_SCALE) / SCREEN_HEIGHT_HALF,
                list * scale,
                0.55f, 0.05f, 0.05f,
                1.0f);
        }
        else
        {
            AEGfxPrint(journalFontId, name,
                (textX * UI_SCALE) / SCREEN_WIDTH_HALF,
                (textY * UI_SCALE) / SCREEN_HEIGHT_HALF,
                listSelected * scale,
                0.2f, 0.2f, 0.2f,
                1.0f);
        }
        
        // ============================================================
        // UNREAD ICON "!"
        // ============================================================
        bool illnessHasUnread = Journal_IllnessHasUnread(IllnessList[i]);

        if (illnessHasUnread && AlertIcon)
        {
            f32 bounceY = 3.0f * sinf(JournalAlertTimer * 3.5f);

            DrawTextureMesh(squareMesh,
                AlertIcon,
                (IllnessTabX - 130.0f) * UI_SCALE,
                (illnessY + 25.0f + bounceY) * UI_SCALE,
                25.0f,
                25.0f,
                1.0f);
        }
    }

    // ============================================================
    // FRONT BOOK
    // ============================================================
    DrawTextureMesh(squareMesh,
        JournalFront,
        0.0f * UI_SCALE,
        0.0f * UI_SCALE,
        1800.0f * UI_SCALE,
        1050.0f * UI_SCALE,
        1.0f);

    // ============================================================
    // HELP TAB
    // ============================================================
    bool hoveredHowTo = IsAreaHovered(
        HowToTabX * UI_SCALE,
        HowToTabY * UI_SCALE,
        HowToTabW * UI_SCALE,
        HowToTabH * UI_SCALE);

    float helpScale = 0.8f;
    if (hoveredHowTo)
        helpScale += 0.08f;

    // draw flipped tab
    DrawTextureMesh(squareMesh,
        TabEntry,
        HowToTabX * UI_SCALE,
        HowToTabY * UI_SCALE,
        -HowToTabW * helpScale * UI_SCALE,
        HowToTabH * helpScale * UI_SCALE,
        1.0f);

    // draw text
    AEGfxPrint(journalFontId, "HELP",
        X(HowToTabX - 30.0f),
        Y(HowToTabY),
        0.6f * helpScale,
        0.2f, 0.2f, 0.2f,
        1.0f);

    if (ClickHowTo)
    {
        // ============================================================
        // HOW TO USE PAGE
        // ============================================================

        float StartY = HeaderY - 80.0f;
        float spacing = 60.0f;

        AEGfxPrint(journalFontId, "How To Use",
            X(LeftStartX),
            Y(HeaderY),
            Header,
            0, 0, 0,
            1.0f);

        // Left Page
        for (int i = 0; i < (int)HelpData.leftLines.size(); ++i)
        {
            AEGfxPrint(journalFontId, HelpData.leftLines[i].c_str(),
                X(LeftStartX), Y(StartY - i * spacing),
                text,
                0.2f, 0.2f, 0.2f,
                1.0f);
        }

        // Right page
        for (int i = 0; i < (int)HelpData.rightLines.size(); ++i)
        {
            AEGfxPrint(journalFontId, HelpData.rightLines[i].c_str(),
                X(RightStartX), Y(StartY - i * spacing),
                text,
                0.2f, 0.2f, 0.2f,
                1.0f);
        }
    }
    else
    {
        // ============================================================
        // LEFT PAGE : description + 2 symptoms
        // ============================================================
        AEGfxPrint(journalFontId, TextData.title.c_str(),
            X(LeftStartX), Y(HeaderY),
            Header,
            0, 0, 0,
            1.0f);

        AEGfxPrint(journalFontId, TextData.desc.c_str(),
            X(LeftStartX), Y(HeaderY - 60.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1.0f);

        AEGfxPrint(journalFontId, TextData.desc2.c_str(),
            X(LeftStartX), Y(HeaderY - 100.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1.0f);

        AEGfxPrint(journalFontId, "Evidence",
            X(LeftStartX), Y(SubHeaderY),
            SubHeader,
            0, 0, 0,
            1.0f);

        for (int i = 0; i < 2; ++i)
        {
            SymptomStruct entry = GetJournalSymptomEntry(chosen, i);
            AEGfxTexture* symptomTex = entry.texture;
            ANOMALYID symptomID = entry.anomalyId;

            float x = LeftPageSymptomX + i * colGap;
            float y = LeftPageSymptomY;

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);
            bool observed = Journal_HasObserved(symptomID);

            if (hovered)
            {
                hasHovered = true;
                HoveredIndex = i;

                if (observed)
                    Journal_MarkAsRead(symptomID);
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

        // ============================================================
        // RIGHT PAGE : remaining 4 symptoms
        // ============================================================

        for (int i = 2; i < 6; ++i)
        {
            SymptomStruct entry = GetJournalSymptomEntry(chosen, i);
            AEGfxTexture* symptomTex = entry.texture;
            ANOMALYID symptomID = entry.anomalyId;

            int localIndex = i - 2;
            int col = localIndex % 2;
            int row = localIndex / 2;

            float x = RightPageSymptomX + col * colGap;
            float y = RightPageSymptomY - row * rowGap;

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);
            bool observed = Journal_HasObserved(symptomID);

            if (hovered)
            {
                hasHovered = true;
                HoveredIndex = i;

                if (observed)
                    Journal_MarkAsRead(symptomID);
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
    }

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
    UnloadTextureSafe(TabEntry);

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

    // destroy fonts
    if (symbolsFontId >= 0)
    {
        AEGfxDestroyFont(symbolsFontId);
        symbolsFontId = -1;
    }
    if (journalFontId >= 0)
    {
        AEGfxDestroyFont(journalFontId);
        journalFontId = -1;
    }

    // reset UI state
    Open = false;
    Selected = 0;

    // free string data loaded from illness text files
    for (int i = 0; i < ILLNESS_COUNT; i++)
        GetFromText[i] = TextStruct{};

    Observed.assign(Observed.size(), false);
    Unread.assign(Unread.size(), false);
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

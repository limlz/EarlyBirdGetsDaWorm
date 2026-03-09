#include "pch.hpp"
#include <algorithm>
#include <vector>

// ============================================================
// STATE
// ============================================================

static s8 journalFontId = -1;       // font handle used by AEGfxPrint
const float UI_SCALE = 0.7f;        // tweak this to make the journal UI bigger or smaller

static bool gOpen = false;
static bool gPrevI = false;
static int  gSelected = 0;

// TEXTURES
static AEGfxTexture* JournalBook = nullptr;
static AEGfxTexture* gWallCrack1 = nullptr;
static AEGfxTexture* gWallCrack2 = nullptr;
static AEGfxTexture* gWallCrack3 = nullptr;
static AEGfxTexture* gWallCrack4 = nullptr;
static AEGfxTexture* gWallDrawing1 = nullptr;
static AEGfxTexture* gWallDrawing2 = nullptr;
static AEGfxTexture* gWallDrawing3 = nullptr;
static AEGfxTexture* gLeftHand = nullptr;
static AEGfxTexture* gRightHand = nullptr;

static AEGfxTexture* gDoorSlam = nullptr;
static AEGfxTexture* gLightChange = nullptr;
static AEGfxTexture* gKnock = nullptr;

static AEGfxTexture* gLightParanoia = nullptr;
static AEGfxTexture* gLightMania = nullptr;
static AEGfxTexture* gLightDepression = nullptr;
static AEGfxTexture* gLightDementia = nullptr;
static AEGfxTexture* gLightSchizophrenia = nullptr;
static AEGfxTexture* gLightAIW = nullptr;
static AEGfxTexture* gLightInsomnia = nullptr;
static AEGfxTexture* gLightScotophobia = nullptr;

static AEGfxTexture* gFrameGlitch = nullptr;
static AEGfxTexture* gFrameShift = nullptr;
static AEGfxTexture* gFrame_Distort = nullptr;

AEGfxTexture* GetAnomalyJournalTexture(ANOMALYID id)
{
    switch (id)
    {
    case ANOMALYID::Wall_Crack1:        return gWallCrack1;
    case ANOMALYID::Wall_Crack2:        return gWallCrack2;
    case ANOMALYID::Wall_Crack3:        return gWallCrack3;
    case ANOMALYID::Wall_Crack4:        return gWallCrack4;
    case ANOMALYID::Wall_Drawing1:      return gWallDrawing1;
    case ANOMALYID::Wall_Drawing2:      return gWallDrawing2;
    case ANOMALYID::Wall_Drawing3:      return gWallDrawing3;
    case ANOMALYID::Wall_LeftHand:      return gLeftHand;
    case ANOMALYID::Wall_RightHand:     return gRightHand;

    case ANOMALYID::Door_HandSlam:      return gDoorSlam;
    case ANOMALYID::Door_LightChange:   return gLightChange;
    case ANOMALYID::Door_Knock:         return gKnock;

    case ANOMALYID::Light_Mania:        return gLightMania;
    case ANOMALYID::Light_Depression:   return gLightDepression;
    case ANOMALYID::Light_Dementia:     return gLightDementia;
    case ANOMALYID::Light_Schizophrenia:return gLightSchizophrenia;
    case ANOMALYID::Light_AIW:          return gLightAIW;
    case ANOMALYID::Light_Insomnia:     return gLightInsomnia;
    case ANOMALYID::Light_Scotophobia:  return gLightScotophobia;

    case ANOMALYID::Frame_Glitch:       return gFrameGlitch;
    case ANOMALYID::Frame_Shift:        return gFrameShift;
    case ANOMALYID::Frame_Distort:      return gFrame_Distort;
    default:                            return nullptr;
    }
}

// ============================================================
// STORAGE: observed anomalies
// ============================================================

static std::vector<ANOMALYID> gObserved;

// ============================================================
// DICTIONARY: illness -> anomaly list
// ============================================================

struct IllnessInfo
{
    ILLNESSES illness;
    std::vector<ANOMALYID> anomalies;
};

static const std::vector<IllnessInfo> gIllnessDictionary =
{
    { ILLNESSES::PARANOIA,      { ANOMALYID::Door_Knock,       ANOMALYID::Wall_LeftHand,    ANOMALYID::Door_LightChange } },
    { ILLNESSES::MANIA,         { ANOMALYID::Door_HandSlam,    ANOMALYID::Wall_Crack2,      ANOMALYID::Light_Mania } },
    { ILLNESSES::DEPRESSION,    { ANOMALYID::Wall_Drawing1,    ANOMALYID::Frame_Shift,      ANOMALYID::Light_Depression } },
    { ILLNESSES::DEMENTIA,      { ANOMALYID::Door_LightChange, ANOMALYID::Wall_Drawing3,    ANOMALYID::Light_Dementia } },
    { ILLNESSES::SCHIZOPHRENIA, { ANOMALYID::Frame_Shift,      ANOMALYID::Door_HandSlam,    ANOMALYID::Light_Schizophrenia } },
    { ILLNESSES::AIW_SYNDROME,  { ANOMALYID::Frame_Distort,    ANOMALYID::Wall_Drawing2,    ANOMALYID::Light_AIW } },
    { ILLNESSES::INSOMNIA,      { ANOMALYID::Door_Knock,       ANOMALYID::Wall_Crack1,      ANOMALYID::Light_Insomnia } },
    { ILLNESSES::OCD,           { ANOMALYID::Door_HandSlam,    ANOMALYID::Wall_Crack4,      ANOMALYID::Frame_Glitch } },
    { ILLNESSES::SCOTOPHOBIA,   { ANOMALYID::Door_LightChange, ANOMALYID::Wall_LeftHand,    ANOMALYID::Light_Scotophobia } },
};

static const std::vector<ILLNESSES> gIllnessList =
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

// ============================================================
// HELPERS: dictionary + labels
// ============================================================

static const std::vector<ANOMALYID>* GetAnomaliesForIllness(ILLNESSES illness)
{
    for (const auto& e : gIllnessDictionary)
        if (e.illness == illness)
            return &e.anomalies;
    return nullptr;
}

static int gExtraGhostAnoms = 0;

void Journal_AddExtraGhostAnomaly()
{
    ++gExtraGhostAnoms;
}

int Journal_GetExtraGhostAnomalies()
{
    return gExtraGhostAnoms;
}

// ============================================================
// MATCHING LOGIC: observed anomalies -> illness OR ghost
// ============================================================

static const char* IllnessText(ILLNESSES i)
{
    switch (i)                                            
    {
    case ILLNESSES::PARANOIA:       return "PARANOIA";      
    case ILLNESSES::MANIA:          return "MANIA";         
    case ILLNESSES::DEPRESSION:     return "DEPRESSION";    
    case ILLNESSES::DEMENTIA:       return "DEMENTIA";      
    case ILLNESSES::SCHIZOPHRENIA:  return "SCHIZOPHRENIA"; 
	case ILLNESSES::AIW_SYNDROME:   return "AIW SYNDROME";  
	case ILLNESSES::INSOMNIA:       return "INSOMNIA";      
    case ILLNESSES::OCD:            return "OCD";          
	case ILLNESSES::SCOTOPHOBIA:    return "SCOTOPHOBIA";   
    default:                        return "UNKNOWN";       
    }
}

static const char* AnomalyText(ANOMALYID id)
{
    switch (id)
    {
    case ANOMALYID::Wall_Crack1:         return "Crack Type I";
    case ANOMALYID::Wall_Crack2:         return "Crack Type II";
    case ANOMALYID::Wall_Crack3:         return "Crack Type III";
    case ANOMALYID::Wall_Crack4:         return "Crack Type IV";
    case ANOMALYID::Wall_Drawing1:       return "Drawing Type I";
    case ANOMALYID::Wall_Drawing2:       return "Drawing Type II";
    case ANOMALYID::Wall_Drawing3:       return "Drawing Type III";
    case ANOMALYID::Wall_LeftHand:       return "Left Handprint";
    case ANOMALYID::Wall_RightHand:      return "Right Handprint";

    case ANOMALYID::Door_HandSlam:       return "Handprint on window";
    case ANOMALYID::Door_LightChange:    return "Window light change";
    case ANOMALYID::Door_Knock:          return "Knocking sound";

    case ANOMALYID::Light_Mania:         return "Rapid Pulse (Warm)";
    case ANOMALYID::Light_Depression:    return "Slow Pulse (Blue)";
    case ANOMALYID::Light_Dementia:      return "Slow Pulse (Warm)";
    case ANOMALYID::Light_Schizophrenia: return "Eratic Flashes (Teal)";
    case ANOMALYID::Light_AIW:           return "Slow Pulse";
    case ANOMALYID::Light_Insomnia:      return "Eratic Flashes";
    case ANOMALYID::Light_Scotophobia:   return "Rapid Pulse";  

    case ANOMALYID::Frame_Glitch:        return "Paintings glitch";
    case ANOMALYID::Frame_Shift:         return "Frames shift position";
    case ANOMALYID::Frame_Distort:       return "Paintings distort";

    default:                            return "Unknown anomaly";
    }
}

// Exact match helper: observed must equal the illness anomaly list (order doesn't matter)
static bool Journal_ExactMatchIllness(ILLNESSES illness)
{
    const auto* expected = GetAnomaliesForIllness(illness);
    if (!expected) return false;

    // If no evidence, we cannot conclude anything
    if (gObserved.empty()) return false;

    // Must match exact count (this is what makes "extra anomaly" => ghost)
    if (gObserved.size() != expected->size()) return false;

    // Compare as sets (sort copies)
    std::vector<ANOMALYID> a = gObserved;
    std::vector<ANOMALYID> b = *expected;

    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    return a == b;
}

// Ghost rule:
// If observed anomalies match ANY illness BUT with exactly +1 extra anomaly -> ghost evidence.
static bool Journal_MatchIllnessPlusOneExtra(ILLNESSES illness)
{
    const auto* expected = GetAnomaliesForIllness(illness);
    if (!expected) return false;

    if (gObserved.empty()) return false;

    // Must be exactly ONE extra anomaly beyond the illness set
    if (gObserved.size() != expected->size() + 1) return false;

    // Check expected is subset of observed
    for (ANOMALYID need : *expected)
    {
        if (std::find(gObserved.begin(), gObserved.end(), need) == gObserved.end())
            return false;
    }
    return true;
}

// Returns true if evidence matches any illness exactly, and outputs that illness (HUMAN)
bool Journal_TryDeduceHumanIllness(ILLNESSES& outIllness)
{
    for (ILLNESSES ill : gIllnessList)
    {
        if (Journal_ExactMatchIllness(ill))
        {
            outIllness = ill;
            return true; // HUMAN
        }
    }
    return false;
}

// Returns true if evidence is "illness + 1 extra anomaly" (GHOST)
bool Journal_IsGhostEvidence()
{
    for (ILLNESSES ill : gIllnessList)
    {
        if (Journal_MatchIllnessPlusOneExtra(ill))
            return true; // GHOST evidence
    }
    return false;

    // ghost evidence = illness evidence + 1 extra anomaly
    return (Journal_GetExtraGhostAnomalies() >= 1);
}

void Journal_Load()
{
    journalFontId = AEGfxCreateFont(Assets::Fonts::Handwriting1, 50);
    JournalBook = LoadTextureChecked(Assets::Equipment::JournalBook);
	gWallCrack1 = LoadTextureChecked(Assets::Wall_Anomaly::Crack1);
	gWallCrack2 = LoadTextureChecked(Assets::Wall_Anomaly::Crack2);
	gWallCrack3 = LoadTextureChecked(Assets::Wall_Anomaly::Crack3);
	gWallCrack4 = LoadTextureChecked(Assets::Wall_Anomaly::Crack4);
	gWallDrawing1 = LoadTextureChecked(Assets::Wall_Anomaly::Drawing1);
	gWallDrawing2 = LoadTextureChecked(Assets::Wall_Anomaly::Drawing2);
	gWallDrawing3 = LoadTextureChecked(Assets::Wall_Anomaly::Drawing3);
	gLeftHand = LoadTextureChecked(Assets::Wall_Anomaly::LeftHand);
	gRightHand = LoadTextureChecked(Assets::Wall_Anomaly::RightHand);

	gDoorSlam = LoadTextureChecked(Assets::Equipment::DoorSlam);

    gLightMania = LoadTextureChecked(Assets::Equipment::LightMania);
    gLightDepression = LoadTextureChecked(Assets::Equipment::LightDepression);
    gLightDementia = LoadTextureChecked(Assets::Equipment::LightDementia);
    gLightSchizophrenia = LoadTextureChecked(Assets::Equipment::LightSchizophrenia);
    gLightAIW = LoadTextureChecked(Assets::Equipment::LightAIW);
    gLightInsomnia = LoadTextureChecked(Assets::Equipment::LightInsomnia);
    gLightScotophobia = LoadTextureChecked(Assets::Equipment::LightScotophobia);

	gFrameGlitch = LoadTextureChecked(Assets::Equipment::FrameGlitch);
	gFrameShift = LoadTextureChecked(Assets::Equipment::FrameShift);
	gFrame_Distort = LoadTextureChecked(Assets::Equipment::FrameDistort);
}

void Journal_Unload()
{
    // unload textures
    UnloadTextureSafe(JournalBook);

    UnloadTextureSafe(gWallCrack1);
    UnloadTextureSafe(gWallCrack2);
    UnloadTextureSafe(gWallCrack3);
    UnloadTextureSafe(gWallCrack4);
    UnloadTextureSafe(gWallDrawing1);
    UnloadTextureSafe(gWallDrawing2);
    UnloadTextureSafe(gWallDrawing3);
    UnloadTextureSafe(gLeftHand);
    UnloadTextureSafe(gRightHand);

    UnloadTextureSafe(gDoorSlam);
    UnloadTextureSafe(gLightChange);
    UnloadTextureSafe(gKnock);

    UnloadTextureSafe(gLightMania);
    UnloadTextureSafe(gLightDepression);
    UnloadTextureSafe(gLightDementia);
    UnloadTextureSafe(gLightSchizophrenia);
    UnloadTextureSafe(gLightAIW);
    UnloadTextureSafe(gLightInsomnia);
    UnloadTextureSafe(gLightScotophobia);

    UnloadTextureSafe(gFrameGlitch);
    UnloadTextureSafe(gFrameShift);
    UnloadTextureSafe(gFrame_Distort);

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
// CORE JOURNAL API
// ============================================================

void Journal_ReportAnomaly(ANOMALYID id)
{
    if (!Journal_HasObserved(id))
        gObserved.push_back(id);
}

bool Journal_HasObserved(ANOMALYID id)
{
    return std::find(gObserved.begin(), gObserved.end(), id) != gObserved.end();
}

void Journal_Clear()
{
    // (keep your existing clears)
    gExtraGhostAnoms = 0;
    gObserved.clear();
}

// ============================================================
// BOOK UI STATE 
// ============================================================

bool Journal_IsOpen()
{
    return gOpen;
}

static void ClampSelected()
{
    if (gSelected < 0) gSelected = 0;
    if (gSelected >= (int)gIllnessList.size()) gSelected = (int)gIllnessList.size() - 1;
}

void Journal_Update()
{
    // Toggle book with I
    if (AEInputCheckTriggered(AEVK_I))
    {
        gOpen = !gOpen;
        AudioManager_PlaySFX(SFX_JOURNAL_OPEN, 1.0f);
    }

    if (!gOpen) return;

    // Keyboard navigation (optional)
    if (AEInputCheckTriggered(AEVK_UP))
    {
        gSelected--;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    if (AEInputCheckTriggered(AEVK_DOWN))
    {
        gSelected++;
        AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
    }

    ClampSelected();
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
        1700.0f * UI_SCALE,
        950.0f * UI_SCALE,
        1.0f);

    if (journalFontId < 0) return;

    auto X = [&](float x) { return (x * UI_SCALE) / SCREEN_WIDTH_HALF; };
    auto Y = [&](float y) { return (y * UI_SCALE) / SCREEN_HEIGHT_HALF; };

    // LEFT PAGE: illness list -----------------------------------------------------------------------------
    float startX = -650.0f;
    float startY = 180.0f;
    float rowStep = 55.0f;

    // Header
    AEGfxPrint(journalFontId, "ILLNESSES", X(startX), Y(300.0f), 
		1.0f, 
        0, 0, 0, // black
        1);
    AEGfxPrint(journalFontId, "use UpDown keys", X(startX), Y(250.0f), 
        0.60f, 
		0.15f, 0.15f, 0.15f, // dark gray
        1);

    for (int i = 0; i < (int)gIllnessList.size(); ++i)
    {
        const char* name = IllnessText(gIllnessList[i]);
        float y = startY - i * rowStep;

        if (i == gSelected)
        {
            AEGfxPrint(journalFontId, name, X(startX), Y(y), 
                0.95f, 
				0.55f, 0.05f, 0.05f, // bright red
                1);
        }
        else
        {
            AEGfxPrint(journalFontId, name, X(startX), Y(y),
                0.8f, 
                0.2f, 0.2f, 0.2f, // dark gray
                1);
        }
    }
    // ------------------------------------------------------------------------------------------------------

    // RIGHT PAGE: selected illness + anomalies -------------------------------------------------------------
    ClampSelected();
    ILLNESSES chosen = gIllnessList[gSelected];

    AEGfxPrint(journalFontId, "ANOMALIES", X(60.0f), Y(300.0f), 
        1.0f, 
		0, 0, 0, // black
        1);
    AEGfxPrint(journalFontId, IllnessText(chosen), X(60.0f), Y(250.0f), 
        0.8f, 
		0.2f, 0.2f, 0.2f, // dark gray
        1);

    const auto* list = GetAnomaliesForIllness(chosen);

    float imgX = 120.0f;
    float imgY = 100.0f;

    float imgW = 120.0f;
    float imgH = 85.0f;

    float rowGap = 115.0f;

    if (!list || list->empty())
    {
        AEGfxPrint( journalFontId, "(No mapping found)", X(imgX), Y(imgY), 
            0.75f, 
			0.2f, 0.2f, 0.2f, // dark gray
            1);
    }
    else
    {
        for (int i = 0; i < (int)list->size(); ++i)
        {
            float y = imgY - i * rowGap;

            ANOMALYID anomaly = (*list)[i];
            AEGfxTexture* tex = GetAnomalyJournalTexture(anomaly);

            // image frame
            //DrawSquareMesh(squareMesh, imgX, y, imgW + 8.0f, imgH + 8.0f, COLOR_WHITE);

            // image
            if (tex)
            {
                DrawTextureMesh(squareMesh, tex, imgX, y, imgW, imgH, 1.0f);
            }
            else
            {
                DrawSquareMesh(squareMesh, imgX, y, imgW, imgH, 0xFFBBBBBB);
            }

            // label beside image
            AEGfxPrint(
				journalFontId,                  // font
				AnomalyText(anomaly),           // text
				X(imgX + 170.0f),               // position label to right of image
				Y((y + 50.0f) - i * 70.0f),     //80.0f is an offset and 70.0f is a row gap
				0.65f,                          // scale
				0, 0, 0,                        // black text
				1                               // alpha
            );
        }
    }
    // -----------------------------------------------------------------------------------------------------------
}
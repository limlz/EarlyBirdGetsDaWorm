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
static AEGfxTexture* gLightFlicker = nullptr;
static AEGfxTexture* gLightDim = nullptr;
static AEGfxTexture* gLightOff= nullptr;
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
    case ANOMALYID::Light_Flicker:      return gLightFlicker;
    case ANOMALYID::Light_Dim:          return gLightDim;
    case ANOMALYID::Light_Off:          return gLightOff;
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
    { ILLNESSES::PARANOIA,      { ANOMALYID::Door_Knock,        ANOMALYID::Wall_LeftHand,     ANOMALYID::Light_Flicker } },
    { ILLNESSES::MANIA,         { ANOMALYID::Door_HandSlam,     ANOMALYID::Wall_Crack2,       ANOMALYID::Frame_Distort } },
    { ILLNESSES::DEPRESSION,    { ANOMALYID::Light_Dim,         ANOMALYID::Wall_Drawing1,     ANOMALYID::Frame_Shift } },
    { ILLNESSES::DEMENTIA,      { ANOMALYID::Door_LightChange,  ANOMALYID::Wall_Drawing3,     ANOMALYID::Frame_Distort } },
    { ILLNESSES::SCHIZOPHRENIA, { ANOMALYID::Frame_Glitch,      ANOMALYID::Door_HandSlam,     ANOMALYID::Wall_RightHand } },
    { ILLNESSES::AIW_SYNDROME,  { ANOMALYID::Frame_Distort,     ANOMALYID::Wall_Drawing2,     ANOMALYID::Door_Knock } },
    { ILLNESSES::INSOMNIA,      { ANOMALYID::Light_Flicker,     ANOMALYID::Door_Knock,        ANOMALYID::Wall_Crack1 } },
    { ILLNESSES::OCD,           { ANOMALYID::Door_HandSlam,     ANOMALYID::Wall_Crack4,       ANOMALYID::Frame_Shift } },
    { ILLNESSES::SCOTOPHOBIA,   { ANOMALYID::Light_Off,         ANOMALYID::Door_LightChange,  ANOMALYID::Wall_LeftHand } },
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
    case ANOMALYID::Wall_Crack1:        return "Crack Type 1";
    case ANOMALYID::Wall_Crack2:        return "Crack Type 2";
    case ANOMALYID::Wall_Crack3:        return "Crack Type 3";
    case ANOMALYID::Wall_Crack4:        return "Crack Type 4";
    case ANOMALYID::Wall_Drawing1:      return "Drawing Type 1";
    case ANOMALYID::Wall_Drawing2:      return "Drawing Type 2";
    case ANOMALYID::Wall_Drawing3:      return "Drawing Type 3";
    case ANOMALYID::Wall_LeftHand:      return "Left Handprint";
    case ANOMALYID::Wall_RightHand:     return "Right Handprint";

    case ANOMALYID::Door_HandSlam:      return "Handprint on window";
    case ANOMALYID::Door_LightChange:   return "Window light change";
    case ANOMALYID::Door_Knock:         return "Knocking sound";

    case ANOMALYID::Light_Flicker:      return "Lights flicker";
    case ANOMALYID::Light_Dim:          return "Lights dim";
    case ANOMALYID::Light_Off:          return "Lights go out";

    case ANOMALYID::Frame_Glitch:       return "Paintings glitch";
    case ANOMALYID::Frame_Shift:        return "Frames shift position";
    case ANOMALYID::Frame_Distort:      return "Paintings distort";

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
	gDoorSlam = LoadTextureChecked(Assets::Equipment::HandSlam);
	//gLightChange = LoadTextureChecked(Assets::Equipment::LightChange);
	//gKnock = LoadTextureChecked(Assets::Equipment::Knock);
	//gLightFlicker = LoadTextureChecked(Assets::Equipment::Flicker);
	//gLightDim = LoadTextureChecked(Assets::Equipment::Dim);
	//gLightOff = LoadTextureChecked(Assets::Equipment::Off);
	//gFrameGlitch = LoadTextureChecked(Assets::Equipment::Glitch);
	//gFrameShift = LoadTextureChecked(Assets::Equipment::Shift);
	//gFrame_Distort = LoadTextureChecked(Assets::Equipment::Distort);
}

void Journal_Unload()
{
    // unload texture
    if (JournalBook)
    {
        AEGfxTextureUnload(JournalBook);
        JournalBook = nullptr;
    }

    // reset font handle so we don't use stale id
    journalFontId = -1;

    // optional: reset UI state
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
        gOpen = !gOpen;

    if (!gOpen) return;

    // Keyboard navigation (optional)
    if (AEInputCheckTriggered(AEVK_UP))   gSelected--;
    if (AEInputCheckTriggered(AEVK_DOWN)) gSelected++;

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
            DrawSquareMesh(squareMesh, imgX, y, imgW + 8.0f, imgH + 8.0f, COLOR_WHITE);

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
				journalFontId,          // font
				AnomalyText(anomaly),   // text
				X(imgX + 170.0f),       // position label to right of image
				Y(y - imgH * 0.25f),    // position label to right of image
				0.65f,                  // scale
				0, 0, 0,                // black text
				1                       // alpha
            );
        }
    }
    // -----------------------------------------------------------------------------------------------------------
}
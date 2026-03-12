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

static AEGfxTexture* gWallParanoia = nullptr;
static AEGfxTexture* gWallMania = nullptr;
static AEGfxTexture* gWallDepression = nullptr;
static AEGfxTexture* gWallDementia = nullptr;
static AEGfxTexture* gWallSchizophrenia = nullptr;
static AEGfxTexture* gWallAIW = nullptr;
static AEGfxTexture* gWallInsomnia = nullptr;
static AEGfxTexture* gWallOCD = nullptr;
static AEGfxTexture* gWallScotophobia = nullptr;

static AEGfxTexture* gDoorSlam = nullptr;
static AEGfxTexture* gLightChange = nullptr;
static AEGfxTexture* gKnock = nullptr;

static AEGfxTexture* gLightParanoia = nullptr;
static AEGfxTexture* gLightMania = nullptr;
static AEGfxTexture* gLightDepression = nullptr;
static AEGfxTexture* gLightDementia = nullptr;
static AEGfxTexture* gLightSchizophrenia = nullptr;
static AEGfxTexture* gLight = nullptr;

static AEGfxTexture* gLiftDementia = nullptr;
static AEGfxTexture* gLiftOCD = nullptr;
static AEGfxTexture* gLiftAIW = nullptr;
static AEGfxTexture* gLiftDepression = nullptr;
static AEGfxTexture* gLiftMania = nullptr;

static AEGfxTexture* gFrameGlitch = nullptr;
static AEGfxTexture* gFrameShift = nullptr;
static AEGfxTexture* gFrame_Distort = nullptr;

AEGfxTexture* GetAnomalyJournalTexture(ANOMALYID id)
{
    switch (id)
    {
    case ANOMALYID::Wall_Paranoia:          return gWallParanoia;
    case ANOMALYID::Wall_Mania:             return gWallMania;
    case ANOMALYID::Wall_Depression:        return gWallDepression;
    case ANOMALYID::Wall_Dementia:          return gWallDementia;
    case ANOMALYID::Wall_Schizophrenia:     return gWallSchizophrenia;
    case ANOMALYID::Wall_AIW:               return gWallAIW;
    case ANOMALYID::Wall_Insomnia:          return gWallInsomnia;
    case ANOMALYID::Wall_OCD:               return gWallOCD;
    case ANOMALYID::Wall_Scotophobia:       return gWallScotophobia;

    case ANOMALYID::Door_HandSlam:          return gDoorSlam;
    case ANOMALYID::Door_LightChange:       return gLightChange;
    case ANOMALYID::Door_Knock:             return gKnock;

    case ANOMALYID::Light_Mania:            return gLightMania;
    case ANOMALYID::Light_Depression:       return gLightDepression;
    case ANOMALYID::Light_Dementia:         return gLightDementia;
    case ANOMALYID::Light_Schizophrenia:    return gLightSchizophrenia;
    case ANOMALYID::Light_AIW:              return gLight;
    case ANOMALYID::Light_Insomnia:         return gLight;
    case ANOMALYID::Light_Scotophobia:      return gLight;


    case ANOMALYID::Lift_Mania:             return gLiftMania;
    case ANOMALYID::Lift_Depression:        return gLiftDepression;
    case ANOMALYID::Lift_Dementia:          return gLight;
    case ANOMALYID::Lift_OCD:               return gLight;
    case ANOMALYID::Lift_AIW:               return gLight;

    case ANOMALYID::Frame_Glitch:           return gFrameGlitch;
    case ANOMALYID::Frame_Shift:            return gFrameShift;
    case ANOMALYID::Frame_Distort:          return gFrame_Distort;
    default:                                return nullptr;
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
    { ILLNESSES::PARANOIA,      { ANOMALYID::Door_Knock,        ANOMALYID::Frame_Shift ,    ANOMALYID::Door_LightChange,     ANOMALYID::Wall_Paranoia       } },
    { ILLNESSES::MANIA,         { ANOMALYID::Lift_Mania,        ANOMALYID::Frame_Glitch,    ANOMALYID::Light_Mania,          ANOMALYID::Wall_Mania          } },
    { ILLNESSES::DEPRESSION,    { ANOMALYID::Lift_Depression,   ANOMALYID::Frame_Shift,     ANOMALYID::Light_Depression,     ANOMALYID::Wall_Depression     } },
    { ILLNESSES::DEMENTIA,      { ANOMALYID::Lift_Dementia,     ANOMALYID::Frame_Shift,     ANOMALYID::Light_Dementia,       ANOMALYID::Wall_Dementia       } },
    { ILLNESSES::SCHIZOPHRENIA, { ANOMALYID::Door_HandSlam,     ANOMALYID::Frame_Glitch,    ANOMALYID::Light_Schizophrenia,  ANOMALYID::Wall_Schizophrenia  } },
    { ILLNESSES::AIW_SYNDROME,  { ANOMALYID::Lift_AIW,          ANOMALYID::Frame_Distort,   ANOMALYID::Light_AIW,            ANOMALYID::Wall_AIW            } },
    { ILLNESSES::INSOMNIA,      { ANOMALYID::Door_Knock,        ANOMALYID::Frame_Glitch,    ANOMALYID::Light_Insomnia,       ANOMALYID::Wall_Insomnia       } },
    { ILLNESSES::OCD,           { ANOMALYID::Door_HandSlam,     ANOMALYID::Frame_Shift,     ANOMALYID::Lift_OCD,             ANOMALYID::Wall_OCD            } },
    { ILLNESSES::SCOTOPHOBIA,   { ANOMALYID::Door_LightChange,  ANOMALYID::Frame_Glitch,    ANOMALYID::Light_Scotophobia,    ANOMALYID::Wall_Scotophobia    } }
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

static bool Journal_PointInRect(float px, float py, float cx, float cy, float w, float h)
{
    float left = cx - w * 0.5f;
    float right = cx + w * 0.5f;
    float bottom = cy - h * 0.5f;
    float top = cy + h * 0.5f;

    return (px >= left && px <= right && py >= bottom && py <= top);
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

//static const char* AnomalyText(ANOMALYID id)
//{
//    switch (id)
//    {
//    case ANOMALYID::Wall_Paranoia:       return "Crack Type I";
//    case ANOMALYID::Wall_Mania:          return "Crack Type II";
//    case ANOMALYID::Wall_Depression:     return "Crack Type III";
//    case ANOMALYID::Wall_Dementia:       return "Crack Type IV";
//    case ANOMALYID::Wall_Schizophernia:  return "Drawing Type I";
//    case ANOMALYID::Wall_AIW:            return "Drawing Type II";
//    case ANOMALYID::Wall_Insomnia:       return "Drawing Type III";
//    case ANOMALYID::Wall_OCD:            return "Left Handprint";
//    case ANOMALYID::Wall_Scotophobia:    return "Right Handprint";
//
//    case ANOMALYID::Door_HandSlam:       return "Handprint on window";
//    case ANOMALYID::Door_LightChange:    return "Window light change";
//    case ANOMALYID::Door_Knock:          return "Knocking sound";
//
//    case ANOMALYID::Light_Mania:         return "Rapid Pulse (Warm)";
//    case ANOMALYID::Light_Depression:    return "Slow Pulse (Blue)";
//    case ANOMALYID::Light_Dementia:      return "Slow Pulse (Warm)";
//    case ANOMALYID::Light_Schizophrenia: return "Eratic Flashes (Teal)";
//    case ANOMALYID::Light_AIW:           return "Slow Pulse";
//    case ANOMALYID::Light_Insomnia:      return "Eratic Flashes";
//    case ANOMALYID::Light_Scotophobia:   return "Rapid Pulse";  
//
//    case ANOMALYID::Frame_Glitch:        return "Paintings glitch";
//    case ANOMALYID::Frame_Shift:         return "Frames shift position";
//    case ANOMALYID::Frame_Distort:       return "Paintings distort";
//
//    default:                            return "Unknown anomaly";
//    }
//}

static const char* AnomalyDescription(ANOMALYID id)
{
    switch (id)
    {
    case ANOMALYID::Wall_Paranoia:       return "Thin wall cracks appear suddenly.";
    case ANOMALYID::Wall_Mania:          return "Jagged cracks spread across the wall.";
    case ANOMALYID::Wall_Depression:     return "Heavy dark cracks sink into the surface.";
    case ANOMALYID::Wall_Dementia:       return "Broken fracture lines form unpredictably.";
    case ANOMALYID::Wall_Schizophrenia:  return "Strange drawings appear on the wall.";
    case ANOMALYID::Wall_AIW:            return "Distorted childlike drawings cover the wall.";
    case ANOMALYID::Wall_Insomnia:       return "Messy repeated sketches appear.";
    case ANOMALYID::Wall_OCD:            return "A left handprint stains the wall.";
    case ANOMALYID::Wall_Scotophobia:    return "A right handprint appears in the dark.";

    case ANOMALYID::Door_HandSlam:       return "A hand violently slams against the window.";
    case ANOMALYID::Door_LightChange:    return "The door window light changes unnaturally.";
    case ANOMALYID::Door_Knock:          return "Knocking can be heard from the door.";

    case ANOMALYID::Light_Mania:         return "Rapid Pulse (Warm)";
    case ANOMALYID::Light_Depression:    return "Slow Pulse (Blue)";
    case ANOMALYID::Light_Dementia:      return "Slow Pulse (Warm)";
    case ANOMALYID::Light_Schizophrenia: return "Eratic Flashes (Teal)";
    case ANOMALYID::Light_AIW:           return "Slow Pulse";
    case ANOMALYID::Light_Insomnia:      return "Eratic Flashes";
    case ANOMALYID::Light_Scotophobia:   return "Rapid Pulse";  


    case ANOMALYID::Lift_Mania:          return "Rapid Pulse (Warm)";
    case ANOMALYID::Lift_Depression:     return "Slow Pulse (Blue)";
    case ANOMALYID::Lift_Dementia:       return "Slow Pulse";
    case ANOMALYID::Lift_AIW:            return "Eratic Flashes";
    case ANOMALYID::Lift_OCD:            return "Rapid Pulse";

    case ANOMALYID::Frame_Glitch:        return "The painting flickers and glitches.";
    case ANOMALYID::Frame_Shift:         return "The frame moves out of place.";
    case ANOMALYID::Frame_Distort:       return "The painting stretches and distorts.";

    default: return "Unknown anomaly.";
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

	gWallParanoia = LoadTextureChecked(Assets::Equipment::WallParanoia);
	gWallMania = LoadTextureChecked(Assets::Equipment::WallMania);
	gWallDepression = LoadTextureChecked(Assets::Equipment::WallDepression);
	gWallDementia = LoadTextureChecked(Assets::Equipment::WallDementia);
	gWallSchizophrenia = LoadTextureChecked(Assets::Equipment::WallSchizophrenia);
	gWallAIW = LoadTextureChecked(Assets::Equipment::WallAIW);
	gWallInsomnia = LoadTextureChecked(Assets::Equipment::WallInsomnia);
	gWallOCD = LoadTextureChecked(Assets::Equipment::WallOCD);
	gWallScotophobia = LoadTextureChecked(Assets::Equipment::WallScotophobia);

	gDoorSlam = LoadTextureChecked(Assets::Equipment::DoorSlam);

    gLightMania = LoadTextureChecked(Assets::Equipment::LightMania);
    gLightDepression = LoadTextureChecked(Assets::Equipment::LightDepression);
    gLightDementia = LoadTextureChecked(Assets::Equipment::LightDementia);
    gLightSchizophrenia = LoadTextureChecked(Assets::Equipment::LightSchizophrenia);
    gLight = LoadTextureChecked(Assets::Equipment::Light);

    gLiftMania = LoadTextureChecked(Assets::Equipment::LiftMania);
    gLiftDepression = LoadTextureChecked(Assets::Equipment::LiftDepression);
    gLiftDementia = LoadTextureChecked(Assets::Equipment::LiftDementia);
    gLiftAIW = LoadTextureChecked(Assets::Equipment::LiftAIW);
    gLiftOCD = LoadTextureChecked(Assets::Equipment::LiftOCD);

	gFrameGlitch = LoadTextureChecked(Assets::Equipment::FrameGlitch);
	gFrameShift = LoadTextureChecked(Assets::Equipment::FrameShift);
	gFrame_Distort = LoadTextureChecked(Assets::Equipment::FrameDistort);
}

void Journal_Unload()
{
    // unload textures
    UnloadTextureSafe(JournalBook);

    UnloadTextureSafe(gWallParanoia);
    UnloadTextureSafe(gWallMania);
    UnloadTextureSafe(gWallSchizophrenia);
    UnloadTextureSafe(gWallDepression);
    UnloadTextureSafe(gWallDementia);
    UnloadTextureSafe(gWallAIW);
    UnloadTextureSafe(gWallInsomnia);
    UnloadTextureSafe(gWallOCD);
    UnloadTextureSafe(gWallScotophobia);

    UnloadTextureSafe(gDoorSlam);
    UnloadTextureSafe(gLightChange);
    UnloadTextureSafe(gKnock);

    UnloadTextureSafe(gLightMania);
    UnloadTextureSafe(gLightDepression);
    UnloadTextureSafe(gLightDementia);
    UnloadTextureSafe(gLightSchizophrenia);
    UnloadTextureSafe(gLight);


    UnloadTextureSafe(gLiftMania);
    UnloadTextureSafe(gLiftDepression);
	UnloadTextureSafe(gLiftDementia);
	UnloadTextureSafe(gLiftAIW);
	UnloadTextureSafe(gLiftOCD);

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

    // Dimensions -----------------------------------------------------------------------------------------
    float HeaderY = 350.0f;     
    float SubHeaderY = 350.0f;
    
    // Pages X positions
    float LeftStartX = -650.0f;
    float RightStartX = 80.0f;

    // Anomalies list
    float ListY = 230.0f;
    float ListStep = 55.0f;

    // 2x2 grid layout
    float imgW = 180.0f;
    float imgH = 200.0f;

    float colGap = 210.0f;
    float rowGap = 210.0f;

    float gridStartX = (RightStartX-20.0f)   + imgW * 0.5f;
    float gridStartY = (ListY-10.0f)        - imgH * 0.5f;

    float tiltAngle = 0.12f;

    // LEFT PAGE: illness list -----------------------------------------------------------------------------
	// Header & Subheader
    AEGfxPrint(journalFontId, "ILLNESSES", X(LeftStartX), Y(HeaderY),
		1.0f, 
        0, 0, 0, // black
        1);

    for (int i = 0; i < (int)gIllnessList.size(); ++i)
    {
        const char* name = IllnessText(gIllnessList[i]);
        float IllnessY = ListY - i * ListStep;

        if (i == gSelected)
        {
            AEGfxPrint(journalFontId, name, X(LeftStartX), Y(IllnessY),
                0.95f, 
				0.55f, 0.05f, 0.05f, // bright red
                1);
        }
        else
        {
            AEGfxPrint(journalFontId, name, X(LeftStartX), Y(IllnessY),
                0.8f, 
                0.2f, 0.2f, 0.2f, // dark gray
                1);
        }
    }
    // ------------------------------------------------------------------------------------------------------

    // RIGHT PAGE: selected illness + anomalies -------------------------------------------------------------
    ClampSelected();
    ILLNESSES chosen = gIllnessList[gSelected];

    // Header & Subheader
    AEGfxPrint(journalFontId, "ANOMALIES", X(RightStartX), Y(HeaderY),
        1.0f,
        0, 0, 0,
        1);

    const auto* list = GetAnomaliesForIllness(chosen);

    // Mouse position
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWorldX = (float)mouseX - SCREEN_WIDTH_HALF;
    float mouseWorldY = SCREEN_HEIGHT_HALF - (float)mouseY;

    // Hover state
    bool hasHovered = false;
    ANOMALYID hoveredAnomaly = ANOMALYID::None; // temporary default

    if (!list || list->empty())
    {
        AEGfxPrint(journalFontId, "(No mapping found)",
            X(gridStartX), Y(gridStartY),
            0.75f,
            0.2f, 0.2f, 0.2f,
            1);
    }
    else
    {
        for (int i = 0; i < (int)list->size(); ++i)
        {
            ANOMALYID anomaly = (*list)[i];
            AEGfxTexture* tex = GetAnomalyJournalTexture(anomaly);

            int col = i % 2;
            int row = i / 2;

            float x = gridStartX + col * colGap;
            float y = gridStartY - row * rowGap;

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);

            if (tex)
            {
                if (hovered)
                {
                    hasHovered = true;
                    hoveredAnomaly = anomaly;

                    DrawTextureMeshRotated(squareMesh, tex, x, y, imgW, imgH, tiltAngle, 1.0f);
                }
                else
                {
                    DrawTextureMesh(squareMesh, tex, x, y, imgW, imgH, 1.0f);
                }
            }
            else
            {
                DrawSquareMesh(squareMesh, x, y, imgW, imgH, 0xFFBBBBBB);
            }
        }

        // fixed hover info area under the grid
        if (hasHovered)
        {
            AEGfxPrint(journalFontId, AnomalyDescription(hoveredAnomaly),
                X(RightStartX), Y(-350.0f),
                0.55f,
                0.2f, 0.2f, 0.2f,
                1);
        }
        else
        {
            AEGfxPrint(journalFontId, "Hover over an image",
                X(RightStartX), Y(-350.0f),
                0.55f,
                0.2f, 0.2f, 0.2f,
                1);
        }
    }
    // -----------------------------------------------------------------------------------------------------------
}
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

// optional: journal background texture
static AEGfxTexture* JournalBook = nullptr;

// ============================================================
// MOUSE COORDS
// ============================================================
// Convert mouse (pixels) to "UI world" coords where
// center of screen is (0,0),
// left edge is -SCREEN_WIDTH_HALF, right edge +SCREEN_WIDTH_HALF,
// top edge +SCREEN_HEIGHT_HALF, bottom edge -SCREEN_HEIGHT_HALF.
static void GetMouseUI(float& outX, float& outY)
{
    // --- REPLACE THIS LINE if your function name differs ---
    // Expected: mx,my in pixels with (0,0) at TOP-LEFT.
    s32 mx = 0, my = 0;
    AEInputGetCursorPosition(&mx, &my);

    outX = (float)mx - SCREEN_WIDTH_HALF;
    outY = SCREEN_HEIGHT_HALF - (float)my;
}

static bool PointInRect(float px, float py, float cx, float cy, float w, float h)
{
    const float halfW = w * 0.5f;
    const float halfH = h * 0.5f;
    return (px >= cx - halfW && px <= cx + halfW &&
        py >= cy - halfH && py <= cy + halfH);
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
    { ILLNESSES::PARANOIA,      { ANOMALYID::Light_Flicker, ANOMALYID::Door_Knock,     ANOMALYID::Door_ShadowMove, ANOMALYID::Frame_Glitch } },
    { ILLNESSES::MANIA,         { ANOMALYID::Light_Dim,     ANOMALYID::Door_HandSlam,  ANOMALYID::Frame_Shift } },
    { ILLNESSES::DEPRESSION,    { ANOMALYID::Light_Dim,     ANOMALYID::Frame_Distort } },
    { ILLNESSES::DEMENTIA,      { ANOMALYID::Wall_Crack2,   ANOMALYID::Frame_Shift,    ANOMALYID::Door_ShadowMove } },
    { ILLNESSES::SCHIZOPHRENIA, { ANOMALYID::Light_Flicker, ANOMALYID::Frame_Glitch,   ANOMALYID::Door_HandSlam } },
    { ILLNESSES::AIW_SYNDROME,  { ANOMALYID::Frame_Distort, ANOMALYID::Light_Flicker } },
    { ILLNESSES::INSOMNIA,      { ANOMALYID::Door_Knock,    ANOMALYID::Light_Flicker } },
    { ILLNESSES::OCD,           { ANOMALYID::Wall_Crack1,   ANOMALYID::Door_HandSlam } },
    { ILLNESSES::SCOTOPHOBIA,   { ANOMALYID::Light_Off,     ANOMALYID::Door_ShadowMove } },
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

// ============================================================
// MATCHING LOGIC: observed anomalies -> illness OR ghost
// ============================================================

// [ADDED] Exact match helper: observed must equal the illness anomaly list (order doesn't matter)
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

// [ADDED] Returns true if evidence matches any illness, and outputs that illness
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
    return false; // not matching any -> GHOST
}

// [ADDED] Convenience: returns ALL if ghost (no match)
ILLNESSES Journal_DeduceIllnessOrGhost()
{
    ILLNESSES matched{};
    if (Journal_TryDeduceHumanIllness(matched))
        return matched;

    return ILLNESSES::ALL; // GHOST
}

static const char* IllnessText(ILLNESSES i)
{
    switch (i)
    {
    case ILLNESSES::PARANOIA:       return "Paranoia";
    case ILLNESSES::MANIA:          return "Mania";
    case ILLNESSES::DEPRESSION:     return "Depression";
    case ILLNESSES::DEMENTIA:       return "Dementia";
    case ILLNESSES::SCHIZOPHRENIA:  return "Schizophrenia";
    case ILLNESSES::AIW_SYNDROME:   return "AIW Syndrome";
    case ILLNESSES::INSOMNIA:       return "Insomnia";
    case ILLNESSES::OCD:            return "OCD";
    case ILLNESSES::SCOTOPHOBIA:    return "Scotophobia";
    case ILLNESSES::NONE:           return "None";
    case ILLNESSES::ALL:            return "Ghost";
    default:                        return "Unknown";
    }
}

static const char* AnomalyText(ANOMALYID id)
{
    switch (id)
    {
    case ANOMALYID::Wall_Crack1:     return "Crack (type 1)";
    case ANOMALYID::Wall_Crack2:     return "Crack (type 2)";
    case ANOMALYID::Wall_Crack3:     return "Crack (type 3)";
    case ANOMALYID::Wall_Crack4:     return "Crack (type 4)";
    case ANOMALYID::Wall_Drawing1:   return "Drawing appears (type 1)";
    case ANOMALYID::Wall_Drawing2:   return "Drawing appears (type 2)";
    case ANOMALYID::Wall_Drawing3:   return "Drawing appears (type 3)";
    case ANOMALYID::Wall_LeftHand:   return "Left handprint on wall";
    case ANOMALYID::Wall_RightHand:  return "Right handprint on wall";

    case ANOMALYID::Door_HandSlam:   return "Handprint slam on door";
    case ANOMALYID::Door_ShadowMove: return "Shadow moves behind door";
    case ANOMALYID::Door_Knock:      return "Knocking sound";

    case ANOMALYID::Light_Flicker:   return "Lights flicker";
    case ANOMALYID::Light_Dim:       return "Lights dim";
    case ANOMALYID::Light_Off:       return "Lights go out";

    case ANOMALYID::Frame_Glitch:    return "Paintings glitch";
    case ANOMALYID::Frame_Shift:     return "Frames shift position";
    case ANOMALYID::Frame_Distort:   return "Paintings distort";

    default:                         return "Unknown anomaly";
    }
}

void Journal_Load()
{
    journalFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    JournalBook = LoadTextureChecked(Assets::Background::JournalBook);
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
    gObserved.clear();
}

// ============================================================
// BOOK UI STATE (Phasmo-style)
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

    // Enter: jump to current illness (optional)
    if (AEInputCheckTriggered(AEVK_RETURN))
    {
        ILLNESSES cur = Player_GetCurrentIllness();
        for (int i = 0; i < (int)gIllnessList.size(); ++i)
            if (gIllnessList[i] == cur) { gSelected = i; break; }
    }

    // Mouse click: pick the row you clicked
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        float mx = 0.0f, my = 0.0f;
        GetMouseUI(mx, my); // your helper converting mouse pixels -> UI coords

        const float startX = -700.0f;
        const float startY = 180.0f;
        const float rowStep = 55.0f;

        const float boxW = 420.0f; // tweak
        const float boxH = 50.0f;  // tweak

        for (int i = 0; i < (int)gIllnessList.size(); ++i)
        {
            float rowY = startY - i * rowStep;

            float cx = startX + boxW * 0.5f;
            float cy = rowY;

            if (PointInRect(mx, my, cx, cy, boxW, boxH))
            {
                gSelected = i;
                break;
            }
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
        1700.0f * UI_SCALE,
        950.0f * UI_SCALE,
        1.0f);

    if (journalFontId < 0) return;

    auto X = [&](float x) { return (x * UI_SCALE) / SCREEN_WIDTH_HALF; };
    auto Y = [&](float y) { return (y * UI_SCALE) / SCREEN_HEIGHT_HALF; };

    // tabs line (vibe)
    AEGfxPrint(journalFontId, "Home   Items   Overview   Media   Evidence   Illnesses   Pause",
        X(-720.0f), Y(380.0f), 0.65f, 0.15f, 0.15f, 0.15f, 1);

    // left header
    AEGfxPrint(journalFontId, "Illness Types", X(-700.0f), Y(300.0f), 1.0f, 0, 0, 0, 1);
    AEGfxPrint(journalFontId, "Select an illness to view anomalies:",
        X(-700.0f), Y(250.0f), 0.65f, 0.15f, 0.15f, 0.15f, 1);

    // left list
    float startX = -700.0f;
    float startY = 180.0f;
    float rowStep = 55.0f;

    for (int i = 0; i < (int)gIllnessList.size(); ++i)
    {
        const char* name = IllnessText(gIllnessList[i]);
        float y = startY - i * rowStep;

        if (i == gSelected)
        {
            // red underline highlight
            DrawSquareMesh(squareMesh, startX + 120.0f, y - 18.0f, 260.0f, 8.0f, COLOR_RED);
            AEGfxPrint(journalFontId, name, X(startX), Y(y), 0.8f, 0.1f, 0.1f, 0.1f, 1);
        }
        else
        {
            AEGfxPrint(journalFontId, name, X(startX), Y(y), 0.8f, 0.2f, 0.2f, 0.2f, 1);
        }
    }

    // right page detail
    ClampSelected();
    ILLNESSES chosen = gIllnessList[gSelected];

    AEGfxPrint(journalFontId, IllnessText(chosen), X(160.0f), Y(300.0f), 1.0f, 0, 0, 0, 1);
    AEGfxPrint(journalFontId, "Possible anomalies:", X(160.0f), Y(250.0f), 0.7f, 0.15f, 0.15f, 0.15f, 1);

    const auto* list = GetAnomaliesForIllness(chosen);

    float ax = 160.0f;
    float ay = 190.0f;

    if (!list || list->empty())
    {
        AEGfxPrint(journalFontId, "(No mapping found)", X(ax), Y(ay), 0.75f, 0.2f, 0.2f, 0.2f, 1);
    }
    else
    {
        for (int i = 0; i < (int)list->size(); ++i)
        {
            char line[128]{};
            sprintf_s(line, "• %s", AnomalyText((*list)[i]));
            AEGfxPrint(journalFontId, line, X(ax), Y(ay - i * 50.0f), 0.75f, 0.2f, 0.2f, 0.2f, 1);
        }
    }

    // footer hint
    AEGfxPrint(journalFontId, "I: close | Up/Down: navigate | Enter: jump to current illness",
        X(-700.0f), Y(-410.0f), 0.6f, 0.15f, 0.15f, 0.15f, 1);
}
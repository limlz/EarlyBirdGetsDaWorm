#include "pch.hpp"
#include <algorithm>
#include <vector>
#include <fstream>
#include <string>

// ============================================================
// STATE
// ============================================================

static s8 journalFontId = -1;       // font handle used by AEGfxPrint
const float UI_SCALE = 0.7f;        // tweak this to make the journal UI bigger or smaller

static bool gOpen = false;
static bool gPrevI = false;
static int  gSelected = 0;
static int  gRightPage = 0;         // 0 = page 1, 1 = page 2
bool        hasHovered = false;
int         hoveredIndex = -1;

// ============================================================
// TEXTURES
// ============================================================
static AEGfxTexture* JournalBook = nullptr;

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
static AEGfxTexture* gDoorOCD = nullptr;

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
static AEGfxTexture* gFrame_Distort = nullptr;

// ------------------------------------------------------------------------------
// Syncing each index to image
// case 0: Wall 
// case 1: Light
// case 2: Frames
// case 3: Door
// case 4: Lift
// case 5: Sound
static AEGfxTexture* GetJournalSymptomTexture(ILLNESSES illness, int index)
{
    switch (illness)
    {
    case ILLNESSES::PARANOIA:
        switch (index)
        {
        case 0: return gWallParanoia;
        case 1: return gLightParanoia;
        case 2: return nullptr;
        case 3: return gDoorLight;
        case 4: return gLiftParanoia; 
        case 5: return gSound; 
        default: return nullptr;
        }

    case ILLNESSES::MANIA:
        switch (index)
        {
        case 0: return gWallMania;
        case 1: return gLightMania;
        case 2: return nullptr;
        case 3: return gDoorHand;
        case 4: return gLiftMania;
        case 5: return gSound;
        default: return nullptr;
        }

    case ILLNESSES::DEPRESSION:
        switch (index)
        {
        case 0: return gWallDepression;
        case 1: return gLightDepression;
        case 2: return nullptr;
        case 3: return nullptr;
        case 4: return gLiftDepression;
        case 5: return gSound;
        default: return nullptr;
        }
    case ILLNESSES::DEMENTIA:
        switch (index)
        {
        case 0: return gWallDementia;
        case 1: return gLightDementia;
        case 2: return nullptr;
        case 3: return nullptr;
        case 4: return gLiftDementia;
        case 5: return gSound;
        default: return nullptr;
        }

    case ILLNESSES::SCHIZOPHRENIA:
        switch (index)
        {
        case 0: return gWallSchizophrenia;
        case 1: return gLightSchizophrenia;
        case 2: return nullptr;
        case 3: return gDoorHand;
        case 4: return gLiftSchizophrenia;
        case 5: return gSound;
        default: return nullptr;
        }

    case ILLNESSES::AIW_SYNDROME:
        switch (index)
        {
        case 0: return gWallAIW;
        case 1: return gLightAIW;
        case 2: return nullptr;
        case 3: return nullptr;
        case 4: return gLiftAIW;
        case 5: return gSound;
        default: return nullptr;
        }

    case ILLNESSES::INSOMNIA:
        switch (index)
        {
        case 0: return gWallInsomnia;
        case 1: return gLightInsomnia;
        case 2: return nullptr;
        case 3: return gDoorLight;
        case 4: return gLiftInsomnia;
        case 5: return gSound;
        default: return nullptr;
        }

    case ILLNESSES::OCD:
        switch (index)
        {
        case 0: return gWallOCD;
        case 1: return gLightOCD;
        case 2: return nullptr;
        case 3: return gDoorOCD;
        case 4: return gLiftOCD;
        case 5: return gSound;
        default: return nullptr;
        }

    case ILLNESSES::SCOTOPHOBIA:
        switch (index)
        {
        case 0: return gWallScotophobia;
        case 1: return gLightScotophobia;
        case 2: return nullptr;
        case 3: return gDoorLight;
        case 4: return gLiftScotophobia;
        case 5: return gSound;
        default: return nullptr;
        }

    default:
        return nullptr;
    }
}

// ============================================================
// HELPERS
// ============================================================

// Is the mouse point within the rectangle centered?
static bool Journal_PointInRect(float px, float py, float cx, float cy, float w, float h)
{
    float left = cx - w * 0.5f;
    float right = cx + w * 0.5f;
    float bottom = cy - h * 0.5f;
    float top = cy + h * 0.5f;

    return (px >= left && px <= right && py >= bottom && py <= top);
}

// ============================================================
// LOADING FROM TXT FILE
// ============================================================

struct JournalTextData
{
    std::string title;
    std::string desc;
    std::string symptomText[6];
};

static JournalTextData gJournalTextData[9];

// Convert illness enum to the corresponding text file path in Assets/Journal/logs
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
static bool LoadJournalTextFile(const char* filePath, JournalTextData& outData)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

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

        if (key == "TITLE")         outData.title = value;
        else if (key == "DESC")     outData.desc = value;
        else if (key == "SYMPTOM1") outData.symptomText[0] = value;
        else if (key == "SYMPTOM2") outData.symptomText[1] = value;
        else if (key == "SYMPTOM3") outData.symptomText[2] = value;
        else if (key == "SYMPTOM4") outData.symptomText[3] = value;
        else if (key == "SYMPTOM5") outData.symptomText[4] = value;
        else if (key == "SYMPTOM6") outData.symptomText[5] = value;
    }

    return true;
}
// ============================================================
// STORAGE: observed anomalies
// ============================================================

static std::vector<ANOMALYID> gObserved;

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

void Journal_Load()
{
    journalFontId = AEGfxCreateFont(Assets::Fonts::Handwriting1, 50);
    JournalBook = LoadTextureChecked(Assets::Equipment::JournalBook);

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
    gFrame_Distort = LoadTextureChecked(Assets::Equipment::FrameDistort);

	gDoorHand = LoadTextureChecked(Assets::Equipment::DoorHand);
    gDoorLight = LoadTextureChecked(Assets::Equipment::DoorLight);
    gDoorOCD = LoadTextureChecked(Assets::Equipment::DoorOCD);

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
    for (int i = 0; i < (int)gIllnessList.size(); ++i)
    {
        ILLNESSES illness = gIllnessList[i];
        const char* path = GetJournalTextFilePath(illness);

        if (LoadJournalTextFile(path, gJournalTextData[i]))
        {
            std::cout << "Loaded journal: " << gJournalTextData[i].title << std::endl;
        }
    }
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

    UnloadTextureSafe(gDoorHand);
    UnloadTextureSafe(gDoorLight);
    UnloadTextureSafe(gDoorOCD);

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
    //gExtraGhostAnoms = 0;
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
    float SubHeaderY    = 200.0f;

    // Pages X positions
    float LeftStartX    = -700.0f;
    float RightStartX   = 80.0f;

    // Anomalies list
    float ListY         = 250.0f;
    float ListStep      = 70.0f;

    // 2x2 grid layout
    float imgW          = 190.0f;
    float imgH          = 210.0f;

    float colGap        = 220.0f;
    float rowGap        = 220.0f;

    float gridStartX    = (RightStartX - 20.0f) + imgW * 0.5f;
    float gridStartY    = (ListY)-imgH * 0.5f;

    float tiltAngle     = 0.12f;

    // Sizing -----------------------------------------------------------------------------------------
    float Header        = 1.0f;
    float SubHeader     = 0.75f;
    float text          = 0.50f;
    float list          = 0.90f;
    float listSelected  = 0.80f;

    // ============================================================
    // LEFT SIDE
    // ============================================================
    // Header & Subheader
    AEGfxPrint(journalFontId, "Illnesses", X(LeftStartX), Y(HeaderY),
        Header,
        0, 0, 0, // black
        1);

    for (int i = 0; i < (int)gIllnessList.size(); ++i)
    {
        const char* name = IllnessText(gIllnessList[i]);
        float IllnessY = ListY - i * ListStep;

        if (i == gSelected)
        {
            AEGfxPrint(journalFontId, name, X(LeftStartX), Y(IllnessY),
                list,
                0.55f, 0.05f, 0.05f, // bright red
                1);
        }
        else
        {
            AEGfxPrint(journalFontId, name, X(LeftStartX), Y(IllnessY),
                listSelected,
                0.2f, 0.2f, 0.2f, // dark gray
                1);
        }
    }
    // ------------------------------------------------------------------------------------------------------

    // ============================================================
    // RIGHT SIDE
    // Consists of 2 pages. Toggle [A] [D] keys.
    // ============================================================
    ClampSelected();
    ILLNESSES chosen = gIllnessList[gSelected];
    JournalTextData& textData = gJournalTextData[gSelected];

    hasHovered = false;
    hoveredIndex = -1;

    // Header
    AEGfxPrint(journalFontId, textData.title.c_str(), 
        X(RightStartX), Y(HeaderY),
        Header,
        0, 0, 0,
        1);

    // Mouse position
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWorldX = (float)mouseX - SCREEN_WIDTH_HALF;
    float mouseWorldY = SCREEN_HEIGHT_HALF - (float)mouseY;

    // ---------------------------------------------------------------------------
    // PAGE 1
    if (gRightPage == 0)
    {
        // Description
        AEGfxPrint(journalFontId, textData.desc.c_str(),
            X(RightStartX), Y(HeaderY - 55.0f),
            text,
            0, 0, 0,
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
            0, 0., 0,
            1);

        // Draw anomalies in a 2x2 grid
        for (int i = 0; i < 2; ++i)
        {
            AEGfxTexture* tex = GetJournalSymptomTexture(chosen, i);

            int col = i % 2;
            int row = i / 2;

            float x = gridStartX + col * colGap;
            float y = (SubHeaderY - 220.0f);

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);

            if (tex)
            {
                if (hovered)
                {
                    hasHovered = true;
                    hoveredIndex = i;

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
        } // ----------------------------------------------------------------------------

        // Hover info area
        const char* hoverText = "Hover an image";

        if (hoveredIndex >= 0 && hoveredIndex < 6)
        {
            hoverText = textData.symptomText[hoveredIndex].c_str();
        }

        AEGfxPrint(journalFontId, hoverText,
            X(RightStartX), Y(-300.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1);

        // Navigate to next page controls
        AEGfxPrint(journalFontId, "[D] >",
            X(520.0f), Y(-350.0f),
            SubHeader,
            0.2f, 0.2f, 0.2f,
            1);
    }

    // ---------------------------------------------------------------------------
    // PAGE 2
    else
    {
        // Draw anomalies in a 2x2 grid
        for (int i = 2; i < 6; ++i)
        {
            AEGfxTexture* tex = GetJournalSymptomTexture(chosen, i);

            int col = (i - 2) % 2;
            int row = (i - 2) / 2;

            float x = gridStartX + col * colGap;
            float y = (150.0f) - row * rowGap;

            bool hovered = Journal_PointInRect(mouseWorldX, mouseWorldY, x, y, imgW, imgH);

            if (tex)
            {
                if (hovered)
                {
                    hasHovered = true;
                    hoveredIndex = i;   // keep real index here

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

        // Hover info area
        const char* hoverText = "Hover an image";

        if (hoveredIndex >= 0 && hoveredIndex < 6)
        {
            hoverText = textData.symptomText[hoveredIndex].c_str();
        }

        AEGfxPrint(journalFontId, hoverText,
            X(RightStartX), Y(-300.0f),
            text,
            0.2f, 0.2f, 0.2f,
            1);

        // Navigate to previous page controls
        AEGfxPrint(journalFontId, "< [A]",
            X(520.0f), Y(-350.0f),
            SubHeader,
            0.2f, 0.2f, 0.2f,
            1);

    }
}
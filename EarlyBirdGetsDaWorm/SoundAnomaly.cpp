#include "pch.hpp"
#include "settings.hpp"

// ================================================================
// HELPERS
// ================================================================

static float RandRange(float lo, float hi)
{
    return lo + (hi - lo) * ((float)(rand() % 1000) / 999.0f);
}

static inline const SoundTimes& SndCfg(ILLNESSES ill)
{
    return Config::soundTable[static_cast<int>(ill)];
}

// Restores BGM/SFX/Walking volumes to whatever the settings sliders say.
// Call this whenever an illness modified the global audio state.
static void RestoreAudioLevels()
{
    float master = Settings_GetMasterVolume();
    AudioManager_SetBGMVolume(Settings_GetBGMVolume() * master);
    AudioManager_SetSFXVolume(Settings_GetSFXVolume() * master);
    AudioManager_SetWalkingVolume(Settings_GetSFXVolume() * master);
}

// ================================================================
// STATE — common
// ================================================================

static bool  gWasCarrying  = false;

// Ghost layer
static float gGhost_Timer  = 0.0f;
static float gGhost_NextAt = 0.0f;

// ================================================================
// STATE — per-illness
// ================================================================

// PARANOIA
static float gPar_Timer            = 0.0f;
static float gPar_NextAt           = 0.0f;
static bool  gPar_WasWalking       = false;
static bool  gPar_ExtraStepPending = false;

// MANIA
static float gMan_Timer       = 0.0f;
static float gMan_NextAt      = 0.0f;
static bool  gMan_SpikeActive = false;
static float gMan_SpikeTimer  = 0.0f;
static bool  gMan_PanRight    = false;

// DEPRESSION
static float gDep_Timer      = 0.0f;
static float gDep_NextAt     = 0.0f;
static bool  gDep_BGMSlugged = false;

// DEMENTIA
static float gDem_Timer        = 0.0f;
static float gDem_NextAt       = 0.0f;
static bool  gDem_Stuttering   = false;
static float gDem_StutterTimer = 0.0f;
static int   gDem_StutterCount = 0;

// SCHIZOPHRENIA
static float gSch_Timer      = 0.0f;
static float gSch_NextAt     = 0.0f;
static bool  gSch_CutPending = false;
static float gSch_CutTimer   = 0.0f;

// AIW
static float gAiw_Timer   = 0.0f;
static float gAiw_NextAt  = 0.0f;
static bool  gAiw_BigNext = true;

// INSOMNIA
static float gIns_CarryTime      = 0.0f;
static float gIns_HeartTimer     = 0.0f;
static float gIns_HeartInterval  = 1.5f;

// OCD
static float   gOcd_Timer        = 0.0f;
static float   gOcd_NextAt       = 0.0f;
static int     gOcd_RepeatCount  = 0;
static float   gOcd_RepeatTimer  = 0.0f;
static SoundID gOcd_RepeatSfx    = SFX_DOOR_KNOCK;
static float   gOcd_RepeatPitch  = 1.0f;
static float   gOcd_RepeatVol    = 0.4f;

// SCOTOPHOBIA
static bool gSco_WasDark = false;

// ================================================================
// ON PICKUP / ON DELIVER
// ================================================================

static void ResetIllnessState()
{
    gGhost_Timer  = 0.0f;
    gGhost_NextAt = RandRange(SndCfg(ILLNESSES::GHOST).minInterval, SndCfg(ILLNESSES::GHOST).maxInterval);

    gPar_Timer = gPar_NextAt = 0.0f;
    gPar_WasWalking = gPar_ExtraStepPending = false;

    gMan_Timer = gMan_NextAt = gMan_SpikeTimer = 0.0f;
    gMan_SpikeActive = false;
    gMan_PanRight = false;

    gDep_Timer = gDep_NextAt = 0.0f;
    gDep_BGMSlugged = false;

    gDem_Timer = gDem_NextAt = gDem_StutterTimer = 0.0f;
    gDem_StutterCount = 0;
    gDem_Stuttering = false;

    gSch_Timer = gSch_NextAt = gSch_CutTimer = 0.0f;
    gSch_CutPending = false;

    gAiw_Timer = gAiw_NextAt = 0.0f;
    gAiw_BigNext = true;

    gIns_CarryTime = gIns_HeartTimer = 0.0f;
    gIns_HeartInterval = 1.5f;

    gOcd_Timer = gOcd_NextAt = gOcd_RepeatTimer = 0.0f;
    gOcd_RepeatCount = 0;

    if (gSco_WasDark) AudioManager_StopSFX();
    gSco_WasDark = false;
}

static void OnPatientPickup(ILLNESSES effective)
{
    ResetIllnessState();

    // Set first-event warmup timers (2 seconds by default)
    gPar_NextAt = 2.0f;
    gMan_NextAt = 2.0f;
    gDep_NextAt = 5.0f;
    gDem_NextAt = 3.0f;
    gSch_NextAt = 3.0f;
    gAiw_NextAt = 2.5f;
    gOcd_NextAt = 3.0f;
    gIns_HeartTimer = -1.5f; // first beat after 1.5s warmup

    // Depression: immediately slow the BGM
    if (effective == ILLNESSES::DEPRESSION)
    {
        float master = Settings_GetMasterVolume();
        AudioManager_SetBGMVolume(Settings_GetBGMVolume() * master * 0.5f);
        gDep_BGMSlugged = true;
    }

    (void)effective;
}

static void OnPatientDelivered()
{
    // Undo any illness-level audio modifications
    RestoreAudioLevels();
    ResetIllnessState();
}

// ================================================================
// PER-ILLNESS UPDATE FUNCTIONS
// ================================================================

// ---- PARANOIA -----------------------------------------------
// * Phantom footsteps slightly behind the player while walking
// * One extra ghost step the moment the player stops
// * Quiet whispers while standing still
static void UpdateParanoia(float dt)
{
    bool walking = Player_IsWalking();

    // Detect walk→stop transition: queue one extra phantom step
    if (gPar_WasWalking && !walking)
    {
        gPar_ExtraStepPending = true;
        gPar_Timer  = 0.0f;
        gPar_NextAt = RandRange(0.2f, 0.5f);
    }
    gPar_WasWalking = walking;

    gPar_Timer += dt;
    if (gPar_Timer < gPar_NextAt) return;
    gPar_Timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::PARANOIA);
    if (gPar_ExtraStepPending)
    {
        // One last phantom footstep after the player stops
        AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.3f, RandRange(0.9f, 1.05f), 0);
        gPar_ExtraStepPending = false;
        gPar_NextAt = RandRange(t.minInterval, t.maxInterval);
    }
    else if (walking)
    {
        // Phantom footsteps trailing behind while moving
        AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.2f, RandRange(0.88f, 1.05f), 0);
        gPar_NextAt = RandRange(1.2f, 2.8f);
    }
    else
    {
        // Whisper from an empty corner while standing
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, t.volume, RandRange(0.85f, 1.0f), 0);
        gPar_NextAt = RandRange(t.minInterval, t.maxInterval);
    }
}

// ---- MANIA --------------------------------------------------
// * Erratic high-pitch scratching
// * Sudden SFX volume spike then snap back
// * Fast whispers that bounce between positions
static void UpdateMania(float dt)
{
    // Resolve the spike cooldown
    if (gMan_SpikeActive)
    {
        gMan_SpikeTimer -= dt;
        if (gMan_SpikeTimer <= 0.0f)
        {
            gMan_SpikeActive = false;
            RestoreAudioLevels();
        }
    }

    gMan_Timer += dt;
    if (gMan_Timer < gMan_NextAt) return;
    gMan_Timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::MANIA);
    switch (rand() % 3)
    {
    case 0: // Erratic scratching at high pitch
        AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, t.volume, 2.0f, 0);
        gMan_NextAt = RandRange(1.5f, 3.5f);
        break;

    case 1: // Audio spike — double SFX volume for 0.15 s then snap back
        AudioManager_SetSFXVolume(Settings_GetSFXVolume() * Settings_GetMasterVolume() * 2.0f);
        AudioManager_PlaySFX(SFX_LIGHT_FLICKER, 1.0f, RandRange(1.5f, 2.0f), 0);
        gMan_SpikeActive = true;
        gMan_SpikeTimer  = 0.15f;
        gMan_NextAt      = RandRange(t.minInterval, t.maxInterval);
        break;

    case 2: // Manic mumbling — fast whispers alternating "left/right" via pitch shift
        gMan_PanRight = !gMan_PanRight;
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, t.volume,
            gMan_PanRight ? RandRange(1.6f, 2.0f) : RandRange(0.6f, 0.9f), 0);
        gMan_NextAt = RandRange(1.0f, 2.5f);
        break;
    }
}

// ---- DEPRESSION ---------------------------------------------
// * BGM slows/dulls on pickup (volume halved, already done in OnPatientPickup)
// * Heavy breathing from the wheelchair
// * Muffled sobbing + distant knocking combo
static void UpdateDepression(float dt)
{
    gDep_Timer += dt;
    if (gDep_Timer < gDep_NextAt) return;
    gDep_Timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::DEPRESSION);
    switch (rand() % 2)
    {
    case 0: // Heavy slow exhale
        AudioManager_PlaySFX(SFX_EXHALE, t.volume, 1.0f, 0);
        gDep_NextAt = RandRange(t.minInterval, t.maxInterval);
        break;

    case 1: // Muffled sobbing + distant knocking
        AudioManager_PlaySFX(SFX_DOOR_KNOCKING, t.volume * 0.5f, 0.6f, 0);
        AudioManager_PlaySFX(SFX_CRYING, t.volume, 1.0f, 0);
        gDep_NextAt = RandRange(t.minInterval, t.maxInterval);
        break;
    }
}

// ---- DEMENTIA -----------------------------------------------
// * SFX_LIFT plays at the end of the hallway (no elevator there)
// * Wheelchair squeak gets stuck in a 3–5 stutter glitch
static void UpdateDementia(float dt)
{
    // Drive ongoing stutter
    if (gDem_Stuttering)
    {
        gDem_StutterTimer -= dt;
        if (gDem_StutterTimer <= 0.0f)
        {
            if (gDem_StutterCount > 0)
            {
                AudioManager_PlaySFX(SFX_WHEELCHAIR_SQUEAK, SndCfg(ILLNESSES::DEMENTIA).volume, RandRange(0.8f, 1.3f), 0);
                gDem_StutterCount--;
                gDem_StutterTimer = 0.18f;
            }
            else
            {
                gDem_Stuttering = false;
            }
        }
    }

    gDem_Timer += dt;
    if (gDem_Timer < gDem_NextAt) return;
    gDem_Timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::DEMENTIA);
    switch (rand() % 2)
    {
    case 0: // Audio displacement — lift ding from the wrong end of the hall
        AudioManager_PlaySFX(SFX_ELEVATOR_PRESS, t.volume, RandRange(0.85f, 1.15f), 0);
        gDem_NextAt = RandRange(t.minInterval, t.maxInterval);
        break;

    case 1: // Looping glitch — wheelchair stutter burst
        if (!gDem_Stuttering)
        {
            gDem_Stuttering   = true;
            gDem_StutterCount = 3 + rand() % 3;
            gDem_StutterTimer = 0.05f;
        }
        gDem_NextAt = RandRange(t.minInterval, t.maxInterval);
        break;
    }
}

// ---- SCHIZOPHRENIA ------------------------------------------
// * Commanding voice — a loud non-spatial whisper right at the player
// * Fake jumpscare — deafening scream cut to silence after 0.2 s
static void UpdateSchizophrenia(float dt)
{
    // Resolve the scream cut-off
    if (gSch_CutPending)
    {
        gSch_CutTimer -= dt;
        if (gSch_CutTimer <= 0.0f)
        {
            AudioManager_StopSFX(); // dead silence after the fake jumpscare
            gSch_CutPending = false;
        }
    }

    gSch_Timer += dt;
    if (gSch_Timer < gSch_NextAt) return;
    gSch_Timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::SCHIZOPHRENIA);
    switch (rand() % 2)
    {
    case 0: // Commanding voices — loud close whisper, not spatial
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, 1.0f, RandRange(0.9f, 1.1f), 0);
        gSch_NextAt = RandRange(t.minInterval, t.maxInterval);
        break;

    case 1: // Fake jumpscare — sudden scream cut to silence in 0.2 s
        AudioManager_PlaySFX(SFX_SCREAMING, 1.5f, RandRange(0.9f, 1.2f), 0);
        gSch_CutPending = true;
        gSch_CutTimer   = 0.2f;
        gSch_NextAt     = RandRange(t.minInterval + 2.0f, t.maxInterval + 2.0f);
        break;
    }
}

// ---- AIW SYNDROME -------------------------------------------
// * Giant's Footsteps — deep pitched-down thud (you feel tiny)
// * The Mouse — tiny high-pitch footstep sounds (everything feels huge)
static void UpdateAIW(float dt)
{
    gAiw_Timer += dt;
    if (gAiw_Timer < gAiw_NextAt) return;
    gAiw_Timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::AIW_SYNDROME);
    if (gAiw_BigNext)
    {
        // Giant's Footsteps — massive low thud
        AudioManager_PlaySFX(SFX_PLANT_THUD, t.volume, 0.35f, 0);
        gAiw_NextAt = RandRange(t.minInterval, t.maxInterval);
    }
    else
    {
        // The Mouse — tiny scurrying sounds at high pitch, quiet
        AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.2f, 2.2f, 0);
        gAiw_NextAt = RandRange(2.0f, 4.0f);
    }
    gAiw_BigNext = !gAiw_BigNext;
}

// ---- INSOMNIA -----------------------------------------------
// * Rhythmic heartbeat that speeds up the longer the patient is carried
// * Occasional tinnitus burst (high-pitch flicker sound)
static void UpdateInsomnia(float dt)
{
    gIns_CarryTime += dt;

    // Heartbeat interval starts at 1.5 s, shrinks toward 0.4 s over 90 s of carrying
    float target = 1.5f - (gIns_CarryTime / 90.0f) * 1.1f;
    if (target < 0.4f) target = 0.4f;
    gIns_HeartInterval += (target - gIns_HeartInterval) * dt * 0.8f;

    gIns_HeartTimer += dt;
    if (gIns_HeartTimer < gIns_HeartInterval) return;
    gIns_HeartTimer = 0.0f;

    // Volume rises with speed (louder as heart races)
    float vol = 0.2f + (1.5f - gIns_HeartInterval) * 0.35f;
    AudioManager_PlaySFX(SFX_HEARTBEAT, vol, 1.0f, 0);

    // 1-in-5 chance of a tinnitus ringing burst alongside the beat
    if (rand() % 5 == 0)
    {
        AudioManager_PlaySFX(SFX_LIGHT_FLICKER, SndCfg(ILLNESSES::INSOMNIA).volume, 3.5f, 0);
    }
}

// ---- OCD ----------------------------------------------------
// * Rule of Three — every sound repeats exactly two more times after the first
// * Rhythmic ticking that gets louder when the player stops moving
static void UpdateOCD(float dt)
{
    // Drive pending repeats
    if (gOcd_RepeatCount > 0)
    {
        gOcd_RepeatTimer -= dt;
        if (gOcd_RepeatTimer <= 0.0f)
        {
            AudioManager_PlaySFX(gOcd_RepeatSfx, gOcd_RepeatVol, gOcd_RepeatPitch, 0);
            gOcd_RepeatCount--;
            gOcd_RepeatTimer = 0.5f;
        }
    }

    gOcd_Timer += dt;
    if (gOcd_Timer < gOcd_NextAt) return;
    gOcd_Timer = 0.0f;

    // Ticking is louder when the player is standing still
    bool walking         = Player_IsWalking();
    float vol            = walking ? 0.15f : 0.45f;
    float pitch          = 1.0f;

    // File contains 3 ticks — play once, no code-driven repeats needed
    AudioManager_PlaySFX(SFX_CLOCK_TICK, vol, pitch, 0);
    gOcd_RepeatCount = 0;

    const SoundTimes& t = SndCfg(ILLNESSES::OCD);
    gOcd_NextAt = RandRange(t.minInterval, t.maxInterval);
}

// ---- SCOTOPHOBIA --------------------------------------------
// * The Whisper — looping whisper while in darkness, cuts off abruptly in light
static void UpdateScotophobia(float /*dt*/, float camX, s8 floorNum)
{
    bool isDark = Lighting_IsPlayerInDarkness(camX, floorNum);

    if (isDark && !gSco_WasDark)
    {
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, SndCfg(ILLNESSES::SCOTOPHOBIA).volume, 1.0f, -1);
        gSco_WasDark = true;
    }
    else if (!isDark && gSco_WasDark)
    {
        AudioManager_StopSFX();
        gSco_WasDark = false;
    }
}

// ================================================================
// LIFECYCLE
// ================================================================

void SoundAnomaly_Initialize()
{
    RestoreAudioLevels();
    gWasCarrying = false;
    ResetIllnessState();
}

void SoundAnomaly_Update(float dt, float camX, s8 floorNum)
{
    bool hasPatient = Player_HasPatient();

    // ── Patient picked up ────────────────────────────────────────
    if (hasPatient && !gWasCarrying)
    {
        gWasCarrying = true;
        ILLNESSES current   = Player_GetTrueDiagnosis();
        ILLNESSES effective = (current == ILLNESSES::GHOST)
                              ? Player_GetAnomalyIllness()
                              : current;
        OnPatientPickup(effective);
    }

    // ── Patient delivered ────────────────────────────────────────
    if (!hasPatient && gWasCarrying)
    {
        gWasCarrying = false;
        OnPatientDelivered();
    }

    if (!hasPatient) return;
    if (!CentralPool_IsSystemActive(AnomalyFiles::Sound_isActive)) return;

    ILLNESSES current   = Player_GetTrueDiagnosis();
    bool      isGhost   = (current == ILLNESSES::GHOST);
    ILLNESSES effective = isGhost ? Player_GetAnomalyIllness() : current;

    // ── Per-illness behaviour ────────────────────────────────────
    switch (effective)
    {
    case ILLNESSES::PARANOIA:      UpdateParanoia(dt);      break;
    case ILLNESSES::MANIA:         UpdateMania(dt);         break;
    case ILLNESSES::DEPRESSION:    UpdateDepression(dt);    break;
    case ILLNESSES::DEMENTIA:      UpdateDementia(dt);      break;
    case ILLNESSES::SCHIZOPHRENIA: UpdateSchizophrenia(dt); break;
    case ILLNESSES::AIW_SYNDROME:  UpdateAIW(dt);           break;
    case ILLNESSES::INSOMNIA:      UpdateInsomnia(dt);      break;
    case ILLNESSES::OCD:           UpdateOCD(dt);           break;
    case ILLNESSES::SCOTOPHOBIA:   UpdateScotophobia(dt, camX, floorNum);   break;
    default: break;
    }

    // ── Ghost underlayer: deep whisper on top of the mimic sound ─
    if (isGhost)
    {
        gGhost_Timer += dt;
        if (gGhost_Timer >= gGhost_NextAt)
        {
            gGhost_Timer  = 0.0f;
            gGhost_NextAt = RandRange(SndCfg(ILLNESSES::GHOST).minInterval, SndCfg(ILLNESSES::GHOST).maxInterval);
            AudioManager_PlaySFX(SFX_VOICE_WHISPER,
                Config::soundTable[10].volume,
                RandRange(0.4f, 0.7f), 0);
        }
    }
}

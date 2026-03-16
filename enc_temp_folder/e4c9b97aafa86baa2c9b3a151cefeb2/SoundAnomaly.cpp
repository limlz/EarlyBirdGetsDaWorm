#include "pch.hpp"
#include "sound_anomaly.hpp"
#include "settings.hpp"

// Global instance
SoundAnomalySystem SoundAnomaly;

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

// ================================================================
// PRIVATE — audio level restore
// Restores BGM/SFX/Walking volumes to whatever the settings sliders say.
// Call this whenever an illness modified the global audio state.
// ================================================================

void SoundAnomalySystem::RestoreAudioLevels()
{
    float master = Settings_GetMasterVolume();
    AudioManager_SetBGMVolume(Settings_GetBGMVolume() * master);
    AudioManager_SetSFXVolume(Settings_GetSFXVolume() * master);
    AudioManager_SetWalkingVolume(Settings_GetSFXVolume() * master);
}

// ================================================================
// PRIVATE — state reset
// ================================================================

void SoundAnomalySystem::ResetState()
{
    ghost = GhostState{};
    ghost.next_at = RandRange(SndCfg(ILLNESSES::GHOST).minInterval,
                              SndCfg(ILLNESSES::GHOST).maxInterval);

    par = ParanoiaState{};
    man = ManiaState{};
    dep = DepressionState{};
    dem = DementiaState{};
    sch = SchizophreniaState{};
    aiw = AiwState{};
    ins = InsomniaState{};
    ocd = OcdState{};

    if (sco.was_dark) AudioManager_StopSFX(); // cut the looping whisper if it was playing
    sco = ScotophobiaState{};
}

// ================================================================
// PRIVATE — pickup / delivery
// ================================================================

void SoundAnomalySystem::OnPickup(ILLNESSES effective)
{
    ResetState();

    // Set first-event warmup timers (2 seconds by default)
    par.next_at     = 2.0f;
    man.next_at     = 2.0f;
    dep.next_at     = 5.0f;
    dem.next_at     = 3.0f;
    sch.next_at     = 3.0f;
    aiw.next_at     = 2.5f;
    ocd.next_at     = 3.0f;
    ins.heart_timer = -1.5f; // first beat fires after 1.5s warmup

    // Depression: immediately slow the BGM on pickup
    if (effective == ILLNESSES::DEPRESSION)
    {
        float master = Settings_GetMasterVolume();
        AudioManager_SetBGMVolume(Settings_GetBGMVolume() * master * 0.5f);
        dep.bgm_slugged = true;
    }
}

void SoundAnomalySystem::OnDelivered()
{
    // Undo any illness-level audio modifications
    RestoreAudioLevels();
    ResetState();
}

// ================================================================
// PRIVATE — per-illness updates
// ================================================================

// ---- PARANOIA --------------------------------------------------
// * Phantom footsteps slightly behind the player while walking
// * One extra ghost step the moment the player stops
// * Quiet whispers while standing still
void SoundAnomalySystem::UpdateParanoia(float dt)
{
    bool walking = Player_IsWalking();

    // Detect walk→stop transition: queue one extra phantom step
    if (par.was_walking && !walking)
    {
        par.extra_step_pending = true;
        par.timer   = 0.0f;
        par.next_at = RandRange(0.2f, 0.5f);
    }
    par.was_walking = walking;

    par.timer += dt;
    if (par.timer < par.next_at) return;
    par.timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::PARANOIA);
    if (par.extra_step_pending)
    {
        // One last phantom footstep after the player stops
        AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.3f, RandRange(0.9f, 1.05f), 0);
        par.extra_step_pending = false;
        par.next_at = RandRange(t.minInterval, t.maxInterval);
    }
    else if (walking)
    {
        // Phantom footsteps trailing behind while moving
        AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.2f, RandRange(0.88f, 1.05f), 0);
        par.next_at = RandRange(1.2f, 2.8f);
    }
    else
    {
        // Whisper from an empty corner while standing
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, t.volume, RandRange(0.85f, 1.0f), 0);
        par.next_at = RandRange(t.minInterval, t.maxInterval);
    }
}

// ---- MANIA -----------------------------------------------------
// * Erratic high-pitch scratching
// * Sudden SFX volume spike then snap back
// * Fast whispers that bounce between positions
void SoundAnomalySystem::UpdateMania(float dt)
{
    // Resolve the spike cooldown
    if (man.spike_active)
    {
        man.spike_timer -= dt;
        if (man.spike_timer <= 0.0f)
        {
            man.spike_active = false;
            RestoreAudioLevels();
        }
    }

    man.timer += dt;
    if (man.timer < man.next_at) return;
    man.timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::MANIA);
    switch (rand() % 3)
    {
    case 0: // Erratic scratching at high pitch
        AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, t.volume, 2.0f, 0);
        man.next_at = RandRange(1.5f, 3.5f);
        break;

    case 1: // Audio spike — double SFX volume for 0.15s then snap back
        AudioManager_SetSFXVolume(Settings_GetSFXVolume() * Settings_GetMasterVolume() * 2.0f);
        AudioManager_PlaySFX(SFX_LIGHT_FLICKER, 1.0f, RandRange(1.5f, 2.0f), 0);
        man.spike_active = true;
        man.spike_timer  = 0.15f;
        man.next_at      = RandRange(t.minInterval, t.maxInterval);
        break;

    case 2: // Manic mumbling — fast whispers alternating "left/right" via pitch shift
        man.pan_right = !man.pan_right;
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, t.volume,
            man.pan_right ? RandRange(1.6f, 2.0f) : RandRange(0.6f, 0.9f), 0);
        man.next_at = RandRange(1.0f, 2.5f);
        break;
    }
}

// ---- DEPRESSION ------------------------------------------------
// * BGM slows/dulls on pickup (volume halved, done in OnPickup)
// * Heavy breathing from the wheelchair
// * Muffled sobbing + distant knocking combo
void SoundAnomalySystem::UpdateDepression(float dt)
{
    dep.timer += dt;
    if (dep.timer < dep.next_at) return;
    dep.timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::DEPRESSION);
    switch (rand() % 2)
    {
    case 0: // Heavy slow exhale
        AudioManager_PlaySFX(SFX_EXHALE, t.volume, 1.0f, 0);
        dep.next_at = RandRange(t.minInterval, t.maxInterval);
        break;

    case 1: // Muffled sobbing + distant knocking
        AudioManager_PlaySFX(SFX_DOOR_KNOCKING, t.volume * 0.5f, 0.6f, 0);
        AudioManager_PlaySFX(SFX_CRYING, t.volume, 1.0f, 0);
        dep.next_at = RandRange(t.minInterval, t.maxInterval);
        break;
    }
}

// ---- DEMENTIA --------------------------------------------------
// * SFX_LIFT plays at the end of the hallway (no elevator there)
// * Wheelchair squeak gets stuck in a 3–5 stutter glitch
void SoundAnomalySystem::UpdateDementia(float dt)
{
    // Drive ongoing stutter
    if (dem.stuttering)
    {
        dem.stutter_timer -= dt;
        if (dem.stutter_timer <= 0.0f)
        {
            if (dem.stutter_count > 0)
            {
                AudioManager_PlaySFX(SFX_WHEELCHAIR_SQUEAK,
                    SndCfg(ILLNESSES::DEMENTIA).volume, RandRange(0.8f, 1.3f), 0);
                dem.stutter_count--;
                dem.stutter_timer = 0.18f;
            }
            else
            {
                dem.stuttering = false;
            }
        }
    }

    dem.timer += dt;
    if (dem.timer < dem.next_at) return;
    dem.timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::DEMENTIA);
    switch (rand() % 2)
    {
    case 0: // Audio displacement — lift ding from the wrong end of the hall
        AudioManager_PlaySFX(SFX_ELEVATOR_PRESS, t.volume, RandRange(0.85f, 1.15f), 0);
        dem.next_at = RandRange(t.minInterval, t.maxInterval);
        break;

    case 1: // Looping glitch — wheelchair stutter burst
        if (!dem.stuttering)
        {
            dem.stuttering    = true;
            dem.stutter_count = 3 + rand() % 3;
            dem.stutter_timer = 0.05f;
        }
        dem.next_at = RandRange(t.minInterval, t.maxInterval);
        break;
    }
}

// ---- SCHIZOPHRENIA ---------------------------------------------
// * Commanding voice — a loud non-spatial whisper right at the player
// * Fake jumpscare — deafening scream cut to silence after 0.2s
void SoundAnomalySystem::UpdateSchizophrenia(float dt)
{
    // Resolve the scream cut-off
    if (sch.cut_pending)
    {
        sch.cut_timer -= dt;
        if (sch.cut_timer <= 0.0f)
        {
            AudioManager_StopSFX(); // dead silence after the fake jumpscare
            sch.cut_pending = false;
        }
    }

    sch.timer += dt;
    if (sch.timer < sch.next_at) return;
    sch.timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::SCHIZOPHRENIA);
    switch (rand() % 2)
    {
    case 0: // Commanding voices — loud close whisper, not spatial
        AudioManager_PlaySFX(SFX_VOICE_WHISPER, 1.0f, RandRange(0.9f, 1.1f), 0);
        sch.next_at = RandRange(t.minInterval, t.maxInterval);
        break;

    case 1: // Fake jumpscare — sudden scream cut to silence in 0.2s
        AudioManager_PlaySFX(SFX_SCREAMING, 1.5f, RandRange(0.9f, 1.2f), 0);
        sch.cut_pending = true;
        sch.cut_timer   = 0.2f;
        sch.next_at     = RandRange(t.minInterval + 2.0f, t.maxInterval + 2.0f);
        break;
    }
}

// ---- AIW SYNDROME ----------------------------------------------
// * Giant's Footsteps — deep pitched-down thud (you feel tiny)
// * The Mouse — tiny high-pitch footstep sounds (everything feels huge)
void SoundAnomalySystem::UpdateAIW(float dt)
{
    aiw.timer += dt;
    if (aiw.timer < aiw.next_at) return;
    aiw.timer = 0.0f;

    const SoundTimes& t = SndCfg(ILLNESSES::AIW_SYNDROME);
    if (aiw.big_next)
    {
        // Giant's Footsteps — massive low thud
        AudioManager_PlaySFX(SFX_PLANT_THUD, t.volume, 0.35f, 0);
        aiw.next_at = RandRange(t.minInterval, t.maxInterval);
    }
    else
    {
        // The Mouse — tiny scurrying sounds at high pitch, quiet
        AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.2f, 2.2f, 0);
        aiw.next_at = RandRange(2.0f, 4.0f);
    }
    aiw.big_next = !aiw.big_next;
}

// ---- INSOMNIA --------------------------------------------------
// * Rhythmic heartbeat that speeds up the longer the patient is carried
// * Occasional tinnitus burst (high-pitch flicker sound)
void SoundAnomalySystem::UpdateInsomnia(float dt)
{
    ins.carry_time += dt;

    // Heartbeat interval starts at 1.5s, shrinks toward 0.4s over 90s of carrying
    float target = 1.5f - (ins.carry_time / 90.0f) * 1.1f;
    if (target < 0.4f) target = 0.4f;
    ins.heart_interval += (target - ins.heart_interval) * dt * 0.8f;

    ins.heart_timer += dt;
    if (ins.heart_timer < ins.heart_interval) return;
    ins.heart_timer = 0.0f;

    // Volume rises with speed (louder as heart races)
    float vol = 0.2f + (1.5f - ins.heart_interval) * 0.35f;
    AudioManager_PlaySFX(SFX_HEARTBEAT, vol, 1.0f, 0);

    // 1-in-5 chance of a tinnitus ringing burst alongside the beat
    if (rand() % 5 == 0)
        AudioManager_PlaySFX(SFX_LIGHT_FLICKER, SndCfg(ILLNESSES::INSOMNIA).volume, 3.5f, 0);
}

// ---- OCD -------------------------------------------------------
// * Rule of Three — every sound repeats exactly two more times after the first
// * Rhythmic ticking that gets louder when the player stops moving
void SoundAnomalySystem::UpdateOCD(float dt)
{
    // Drive pending repeats
    if (ocd.repeat_count > 0)
    {
        ocd.repeat_timer -= dt;
        if (ocd.repeat_timer <= 0.0f)
        {
            AudioManager_PlaySFX(ocd.repeat_sfx, ocd.repeat_vol, ocd.repeat_pitch, 0);
            ocd.repeat_count--;
            ocd.repeat_timer = 0.5f;
        }
    }

    ocd.timer += dt;
    if (ocd.timer < ocd.next_at) return;
    ocd.timer = 0.0f;

    // Ticking is louder when the player is standing still
    bool  walking = Player_IsWalking();
    float vol     = walking ? 0.15f : 0.45f;

    // File contains 3 ticks — play once, no code-driven repeats needed
    AudioManager_PlaySFX(SFX_CLOCK_TICK, vol, 1.0f, 0);
    ocd.repeat_count = 0;

    const SoundTimes& t = SndCfg(ILLNESSES::OCD);
    ocd.next_at = RandRange(t.minInterval, t.maxInterval);
}

// ---- SCOTOPHOBIA -----------------------------------------------
// * The Whisper — looping whisper while in darkness, cuts off abruptly in light
void SoundAnomalySystem::UpdateScotophobia(float dt, float cam_x, s8 floor_num)
{
    (void)dt;
    bool is_dark = Lighting_IsPlayerInDarkness(cam_x, floor_num);

    if (is_dark && !sco.was_dark)
    {
        AudioManager_PlaySFX(SFX_VOICE_WHISPER,
            SndCfg(ILLNESSES::SCOTOPHOBIA).volume, 1.0f, -1);
        sco.was_dark = true;
    }
    else if (!is_dark && sco.was_dark)
    {
        AudioManager_StopSFX();
        sco.was_dark = false;
    }
}

// ================================================================
// PUBLIC — lifecycle
// ================================================================

void SoundAnomalySystem::Initialize()
{
    RestoreAudioLevels();
    was_carrying = false;
    ResetState();
}

void SoundAnomalySystem::Update(float dt, float cam_x, s8 floor_num)
{
    bool has_patient = Player_HasPatient();

    // ── Patient picked up ─────────────────────────────────────────
    if (has_patient && !was_carrying)
    {
        was_carrying = true;
        ILLNESSES current   = Player_GetTrueDiagnosis();
        ILLNESSES effective = (current == ILLNESSES::GHOST)
                              ? Player_GetAnomalyIllness()
                              : current;
        OnPickup(effective);
    }

    // ── Patient delivered ─────────────────────────────────────────
    if (!has_patient && was_carrying)
    {
        was_carrying = false;
        OnDelivered();
    }

    if (!has_patient) return;
    if (!CentralPool_IsSystemActive(AnomalyFiles::Sound_isActive)) return;

    ILLNESSES current   = Player_GetTrueDiagnosis();
    bool      is_ghost  = (current == ILLNESSES::GHOST);
    ILLNESSES effective = is_ghost ? Player_GetAnomalyIllness() : current;

    // ── Per-illness behaviour ─────────────────────────────────────
    switch (effective)
    {
    case ILLNESSES::PARANOIA:
        Journal_ReportAnomaly(ANOMALYID::Sound_Paranoia);
        UpdateParanoia(dt);
        break;
    case ILLNESSES::MANIA:
        Journal_ReportAnomaly(ANOMALYID::Sound_Mania);
        UpdateMania(dt);
        break;
    case ILLNESSES::DEPRESSION:
        Journal_ReportAnomaly(ANOMALYID::Sound_Depression);
        UpdateDepression(dt);
        break;
    case ILLNESSES::DEMENTIA:
        Journal_ReportAnomaly(ANOMALYID::Sound_Dementia);
        UpdateDementia(dt);
        break;
    case ILLNESSES::SCHIZOPHRENIA:
        Journal_ReportAnomaly(ANOMALYID::Sound_Schizophrenia);
        UpdateSchizophrenia(dt);
        break;
    case ILLNESSES::AIW_SYNDROME:
        Journal_ReportAnomaly(ANOMALYID::Sound_AIW);
        UpdateAIW(dt);
        break;
    case ILLNESSES::INSOMNIA:
        Journal_ReportAnomaly(ANOMALYID::Sound_Insomnia);
        UpdateInsomnia(dt);
        break;
    case ILLNESSES::OCD:
        Journal_ReportAnomaly(ANOMALYID::Sound_OCD);
        UpdateOCD(dt);
        break;
    case ILLNESSES::SCOTOPHOBIA:
        Journal_ReportAnomaly(ANOMALYID::Sound_Scotophobia);
        UpdateScotophobia(dt, cam_x, floor_num);
        break;
    default:
        break;
    }

    // ── Ghost underlayer: deep whisper on top of the mimic sound ──
    if (is_ghost)
    {
        ghost.timer += dt;
        if (ghost.timer >= ghost.next_at)
        {
            ghost.timer   = 0.0f;
            ghost.next_at = RandRange(SndCfg(ILLNESSES::GHOST).minInterval,
                                      SndCfg(ILLNESSES::GHOST).maxInterval);
            AudioManager_PlaySFX(SFX_VOICE_WHISPER,
                Config::soundTable[10].volume,
                RandRange(0.4f, 0.7f), 0);
        }
    }
}

#pragma once
#include "pch.hpp"

class SoundAnomalySystem
{
public:
    void Initialize();
    void Update(float dt, float cam_x, s8 floor_num);

private:
    // ── Pickup / delivery tracking ──────────────────────────────
    bool was_carrying = false;

    // ── Per-illness state structs ────────────────────────────────
    struct GhostState {
        float timer  = 0.0f;
        float next_at = 0.0f;
    };

    struct ParanoiaState {
        float timer            = 0.0f;
        float next_at          = 0.0f;
        bool  was_walking      = false;
        bool  extra_step_pending = false;
    };

    struct ManiaState {
        float timer        = 0.0f;
        float next_at      = 0.0f;
        bool  spike_active = false;
        float spike_timer  = 0.0f;
        bool  pan_right    = false;
    };

    struct DepressionState {
        float timer      = 0.0f;
        float next_at    = 0.0f;
        bool  bgm_slugged = false;
    };

    struct DementiaState {
        float timer         = 0.0f;
        float next_at       = 0.0f;
        bool  stuttering    = false;
        float stutter_timer = 0.0f;
        int   stutter_count = 0;
    };

    struct SchizophreniaState {
        float timer       = 0.0f;
        float next_at     = 0.0f;
        bool  cut_pending = false;
        float cut_timer   = 0.0f;
    };

    struct AiwState {
        float timer    = 0.0f;
        float next_at  = 0.0f;
        bool  big_next = true;
    };

    struct InsomniaState {
        float carry_time     = 0.0f;
        float heart_timer    = 0.0f;
        float heart_interval = 1.5f;
    };

    struct OcdState {
        float   timer        = 0.0f;
        float   next_at      = 0.0f;
        int     repeat_count = 0;
        float   repeat_timer = 0.0f;
        SoundID repeat_sfx   = SoundID::SFX_DOOR_KNOCK;
        float   repeat_pitch = 1.0f;
        float   repeat_vol   = 0.4f;
    };

    struct ScotophobiaState {
        bool was_dark = false;
    };

    // ── Instances ────────────────────────────────────────────────
    GhostState        ghost;
    ParanoiaState     par;
    ManiaState        man;
    DepressionState   dep;
    DementiaState     dem;
    SchizophreniaState sch;
    AiwState          aiw;
    InsomniaState     ins;
    OcdState          ocd;
    ScotophobiaState  sco;

    // ── Internal helpers ─────────────────────────────────────────
    void ResetState();
    void OnPickup(ILLNESSES effective);
    void OnDelivered();
    void RestoreAudioLevels();

    void UpdateParanoia     (float dt);
    void UpdateMania        (float dt);
    void UpdateDepression   (float dt);
    void UpdateDementia     (float dt);
    void UpdateSchizophrenia(float dt);
    void UpdateAIW          (float dt);
    void UpdateInsomnia     (float dt);
    void UpdateOCD          (float dt);
    void UpdateScotophobia  (float dt, float cam_x, s8 floor_num);
};

extern SoundAnomalySystem SoundAnomaly;

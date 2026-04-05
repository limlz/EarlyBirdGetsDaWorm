#pragma once
#include "pch.hpp"

/**
 * @brief System that manages sound-based anomaly effects tied to various illnesses.
 */
class SoundAnomalySystem
{
public:
    /**
     * @brief Initializes the sound anomaly system and resets all internal state.
     */
    void Initialize();

    /**
     * @brief Updates all active sound anomaly effects each frame.
     * @param dt        Delta time in seconds since the last frame.
     * @param cam_x     Current camera X position in world space.
     * @param floor_num Current floor number the player is on.
     */
    void Update(float dt, float cam_x, s8 floor_num);

private:
    bool was_carrying = false;

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

    /**
     * @brief Resets all anomaly state machines to their defaults.
     */
    void ResetState();

    /**
     * @brief Handles sound anomaly activation when the player picks up an item.
     * @param effective The illness type that determines which anomaly to activate.
     */
    void OnPickup(ILLNESSES effective);

    /**
     * @brief Handles cleanup when the carried item is delivered.
     */
    void OnDelivered();

    /**
     * @brief Restores all audio levels to their normal values after anomaly effects.
     */
    void RestoreAudioLevels();

    /**
     * @brief Updates the paranoia sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateParanoia     (float dt);

    /**
     * @brief Updates the mania sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateMania        (float dt);

    /**
     * @brief Updates the depression sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateDepression   (float dt);

    /**
     * @brief Updates the dementia sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateDementia     (float dt);

    /**
     * @brief Updates the schizophrenia sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateSchizophrenia(float dt);

    /**
     * @brief Updates the Alice in Wonderland syndrome sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateAIW          (float dt);

    /**
     * @brief Updates the insomnia sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateInsomnia     (float dt);

    /**
     * @brief Updates the OCD sound anomaly effect.
     * @param dt Delta time in seconds since the last frame.
     */
    void UpdateOCD          (float dt);

    /**
     * @brief Updates the scotophobia sound anomaly effect.
     * @param dt        Delta time in seconds since the last frame.
     * @param cam_x     Current camera X position in world space.
     * @param floor_num Current floor number the player is on.
     */
    void UpdateScotophobia  (float dt, float cam_x, s8 floor_num);
};

extern SoundAnomalySystem SoundAnomaly;

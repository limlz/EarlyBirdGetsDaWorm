#include "pch.hpp"

// ============================================================
// PER-ILLNESS SOUND PROFILE
// Which SFX to fire and the pitch range to randomise within.
// Index matches the ILLNESSES enum (0 = NONE ... 10 = GHOST).
// ============================================================
struct SoundProfile {
    SoundID sfx;
    float   pitchMin;
    float   pitchMax;
};

static const SoundProfile gProfiles[] = {
    /* 0: NONE          */ { SFX_VOICE_WHISPER,           1.0f, 1.0f  },
    /* 1: PARANOIA      */ { SFX_VOICE_WHISPER,           0.8f, 1.0f  }, // Hushed, low whispers
    /* 2: MANIA         */ { SFX_SCREAMING,               1.0f, 1.4f  }, // Chaotic high-pitched outbursts
    /* 3: DEPRESSION    */ { SFX_DOOR_KNOCKING,           0.6f, 0.8f  }, // Slow, muffled distant knocking
    /* 4: DEMENTIA      */ { SFX_PLANT_THUD,              0.7f, 1.3f  }, // Random disorienting thuds
    /* 5: SCHIZOPHRENIA */ { SFX_SCREAMING,               0.9f, 1.1f  }, // Mid-pitch clashing voices
    /* 6: AIW_SYNDROME  */ { SFX_VOICE_WHISPER,           0.5f, 1.5f  }, // Wildly pitch-warped whispers
    /* 7: INSOMNIA      */ { SFX_LIGHT_FLICKER,           0.9f, 1.1f  }, // Ticking/buzzing flicker sound
    /* 8: OCD           */ { SFX_DOOR_KNOCK,              1.0f, 1.0f  }, // Identical pitch every time (metronomic)
    /* 9: SCOTOPHOBIA   */ { SFX_VOICE_WHISPER,           0.7f, 0.9f  }, // Slow breathing-like low whisper
    /* 10: GHOST        */ { SFX_VOICE_WHISPER,           0.4f, 0.7f  }, // Deep ghost underlayer
};

// ============================================================
// STATE
// ============================================================
static float gEventTimer  = 0.0f;
static float gNextEventAt = 0.0f;
static bool  gWasCarrying = false; // Detects patient pickup to reset the timer cleanly

// ============================================================
// HELPERS
// ============================================================

static float RandRange(float lo, float hi) {
    return lo + (hi - lo) * ((float)(rand() % 1000) / 999.0f);
}

// Samples the next fire interval from Config for this illness index.
// Applies ghostMult when the patient is a ghost.
static float SampleInterval(int idx, bool isGhost) {
    float lo = Config::soundTable[idx].minInterval;
    float hi = Config::soundTable[idx].maxInterval;
    float t  = RandRange(lo, hi);
    if (isGhost) t *= Config::soundTable[idx].ghostMult;
    return t;
}

// ============================================================
// LIFECYCLE
// ============================================================

void SoundAnomaly_Initialize() {
    gEventTimer  = 0.0f;
    gNextEventAt = 0.0f;
    gWasCarrying = false;
}

void SoundAnomaly_Update(float dt) {
    bool hasPatient = Player_HasPatient();

    // Detect fresh pickup - reset and give a short warmup before first event
    if (hasPatient && !gWasCarrying) {
        gEventTimer  = 0.0f;
        gNextEventAt = 2.0f; // 2-second warmup so the sound doesn't fire the instant the patient is picked up
    }
    gWasCarrying = hasPatient;

    if (!hasPatient) return;
    if (!CentralPool_IsSystemActive(AnomalyFiles::Sound_isActive)) return;

    ILLNESSES current   = Player_GetCurrentIllness();
    bool      isGhost   = (current == ILLNESSES::GHOST);
    ILLNESSES effective = isGhost ? Player_GetMimicIllness() : current;
    int       idx       = static_cast<int>(effective);

    gEventTimer += dt;
    if (gEventTimer < gNextEventAt) return;

    // --- Fire primary sound event ---
    const SoundProfile& profile = gProfiles[idx];
    float pitch  = RandRange(profile.pitchMin, profile.pitchMax);
    float volume = Config::soundTable[idx].volume;
    AudioManager_PlaySFX(profile.sfx, volume, pitch, 0);

    // --- Ghost layer: deep whisper played underneath the mimic sound ---
    if (isGhost) {
        const SoundProfile& ghostProfile = gProfiles[10];
        float ghostPitch  = RandRange(ghostProfile.pitchMin, ghostProfile.pitchMax);
        float ghostVolume = Config::soundTable[10].volume;
        AudioManager_PlaySFX(ghostProfile.sfx, ghostVolume, ghostPitch, 0);
    }

    // Reset and schedule next event
    gEventTimer  = 0.0f;
    gNextEventAt = SampleInterval(idx, isGhost);
}

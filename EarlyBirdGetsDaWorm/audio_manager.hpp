#pragma once

enum class SoundID {

    /*************  1. BACKGROUND MUSIC (BGM)   *************/

    BGM_GAME_AMBIENCE,              // BGM_GAME_AMBIENCE.wav
    BGM_MAIN_MENU,                  // BGM_MAIN_MENU.wav
    BGM_RAIN,                       // BGM_RAIN.wav

    /****************  2. ENVIRONMENTAL SFX   ***************/

    SFX_DOOR_KNOCKING,              // SFX_DOOR_KNOCKING.wav
    SFX_LIGHT_FLICKER,              // SFX_LIGHT_BUZZING.wav
    SFX_PLANT_THUD,                 // SFX_PLANT_DROP_THONK.wav
    SFX_DOOR_KNOCK,                 // SFX_DOOR_KNOCK.wav

    /*************  3. PLAYER & CHARACTER SFX   *************/

    SFX_BOSS_FIGHT,                 // SFX_BOSS_FIGHT.wav
    SFX_PLAYER_WALKING,             // SFX_PLAYER_WALKING.wav
    SFX_SCREAMING,                  // SFX_SCREAMING.wav
    SFX_VOICE_WHISPER,              // SFX_VOICE_WHISPER.wav
    SFX_WHEELCHAIR_SQUEAK,          // SFX_WHEELCHAIR_SQUEAK.wav
    SFX_HEARTBEAT,                  // SFX_HEARTBEAT.wav
    SFX_EXHALE,                     // SFX_EXHALE.wav
    SFX_CRYING,                     // SFX_CRYING.wav
    SFX_CLOCK_TICK,                 // SFX_CLOCK_TICK.wav

    /*************  5. BOSS FIGHT SFX   *************/

    SFX_JUMP_GRUNT,                 // SFX_JUMP_GRUNT.wav
    SFX_WEOW,                       // SFX_WEOW.wav  (horror-less jump replacement)
    SFX_WATER_GUN_SQUIRT,           // SFX_WATER_GUN_SQUIRT.wav
    SFX_WATER_IMPACT,               // SFX_WATER_IMPACT.wav
    SFX_SHIELD_BREAK,               // SFX_SHIELD_BREAK.wav
    SFX_DODGE,                      // SFX_DODGE.wav

    /****************  4. UI & SYSTEM SFX   *****************/

    SFX_MAIN_MENU_WRITING_SCRATCH,  // SFX_MAIN_MENU_WRITING_SCRATCH.wav
    SFX_NEW_DAY,                    // SFX_NEW_DAY.wav
    SFX_PAGER_NOTIFICATIONS,        // SFX_PAGER_NOTIFICATIONS.wav
    SFX_SWOOSH_1,                   // SFX_SWOOSH_1.wav
    SFX_SWOOSH_2,                   // SFX_SWOOSH_2.wav
    SFX_SWOOSH_3,                   // SFX_SWOOSH_3.wav
	SFX_JOURNAL_SELECT,             // SFX_JOURNAL_SELECT.wav
	SFX_JOURNAL_OPEN,               // SFX_JOURNAL_OPEN.wav
    SFX_TUTORIAL_TYPING,
	SFX_AA,                         // SFX_AA.wav
	SFX_FAH,                        // SFX_FAH.wav
	SFX_ELEVATOR_PRESS,             // SFX_ELEVATOR_PRESS.wav
    SFX_ELEVATOR_CLOSE,

    SND_COUNT
};

void AudioManager_Init();
void AudioManager_LoadAll();
void AudioManager_Unload();

void AudioManager_PlayBGM(SoundID id, float volume);
void AudioManager_PlaySFX(SoundID id, float volume, float pitch = 1.f, s32 loops = 0);
void AudioManager_PlaySpatialSound(SoundID id, float anomalyX, float playerX);

void AudioManager_SetBGMVolume(float volume);
void AudioManager_SetSFXVolume(float volume);
void AudioManager_SetWalkingVolume(float volume);

void AudioManager_StopBGM();
void AudioManager_StopSFX();

// Dedicated walking group — stopping these never kills anomaly/door SFX
void AudioManager_PlayWalkingSFX(SoundID id, float volume, float pitch = 1.f, s32 loops = 0);
void AudioManager_StopWalkingSFX();
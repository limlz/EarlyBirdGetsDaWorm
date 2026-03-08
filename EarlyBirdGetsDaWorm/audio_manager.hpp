#pragma once

enum SoundID {
    
    /*************  1. BACKGROUND MUSIC (BGM)   *************/

    BGM_GAME_AMBIENCE,              // BGM_GAME_AMBIENCE.wav
    BGM_MAIN_MENU,                  // BGM_MAIN_MENU.wav
    BGM_RAIN,                       // BGM_RAIN.wav

    /****************  2. ENVIRONMENTAL SFX   ***************/

    SFX_DOOR_KNOCKING,              // SFX_DOOR_KNOCKING.wav
    SFX_LIFT,                       // SFX_LIFT.wav
    SFX_LIGHT_FLICKER,              // SFX_LIGHT_BUZZING.wav
    SFX_PLANT_THUD,                 // SFX_PLANT_DROP_THONK.wav
	SFX_DOOR_KNOCK,                 // SFX_DOOR_KNOCK.wav

    /*************  3. PLAYER & CHARACTER SFX   *************/

    SFX_BOSS_FIGHT,                 // SFX_BOSS_FIGHT.wav
    SFX_PLAYER_WALKING,             // SFX_PLAYER_WALKING.wav
    SFX_SCREAMING,                  // SFX_SCREAMING.wav
    SFX_VOICE_WHISPER,              // SFX_VOICE_WHISPER.wav
    SFX_WHEELCHAIR_SQUEAK,          // SFX_WHEELCHAIR_SQUEAK.wav

    /****************  4. UI & SYSTEM SFX   *****************/

    SFX_MAIN_MENU_WRITING_SCRATCH,  // SFX_MAIN_MENU_WRITING_SCRATCH.wav
    SFX_NEW_DAY,                    // SFX_NEW_DAY.wav
    SFX_PAGER_NOTIFICATIONS,        // SFX_PAGER_NOTIFICATIONS.wav
    SFX_SWOOSH_1,                   // SFX_SWOOSH_1.wav
    SFX_SWOOSH_2,                   // SFX_SWOOSH_2.wav
    SFX_SWOOSH_3,                   // SFX_SWOOSH_3.wav
    SFX_BUTTON_SELECT,              // SFX_BUTTON_SELECT.wav

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

void AudioManager_StopBGM();
void AudioManager_StopSFX();
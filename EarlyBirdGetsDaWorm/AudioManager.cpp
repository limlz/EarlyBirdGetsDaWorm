#include "pch.hpp"

static AEAudio soundBank[SND_COUNT];

static AEAudioGroup groupBGM;
static AEAudioGroup groupSFX;

void AudioManager_Init() {
    AEAudioInit();

    groupBGM = AEAudioCreateGroup();
    groupSFX = AEAudioCreateGroup();
}

void AudioManager_LoadAll() {
    // BGM
    soundBank[BGM_GAME_AMBIENCE] = AEAudioLoadSound("Assets/Audio/BGM_GAME_AMBIENCE.wav");
    soundBank[BGM_MAIN_MENU] = AEAudioLoadSound("Assets/Audio/BGM_MAIN_MENU.wav");
    soundBank[BGM_RAIN] = AEAudioLoadSound("Assets/Audio/BGM_ENVIRONMENT_RAIN.wav");

    // Environmental
    soundBank[SFX_DOOR_KNOCKING] = AEAudioLoadSound("Assets/Audio/SFX_DOOR_KNOCK.wav");
    soundBank[SFX_LIFT] = AEAudioLoadSound("Assets/Audio/SFX_LIFT.wav");
    soundBank[SFX_LIGHT_FLICKER] = AEAudioLoadSound("Assets/Audio/SFX_LIGHT_FLICKER.wav");
    soundBank[SFX_PLANT_THUD] = AEAudioLoadSound("Assets/Audio/SFX_PLANT_THUD.wav");
    soundBank[SFX_DOOR_KNOCK] = AEAudioLoadSound("Assets/Audio/SFX_PLANT_THUD.wav");

    // Characters
    soundBank[SFX_BOSS_FIGHT] = AEAudioLoadSound("Assets/Audio/SFX_BOSS_FIGHT.wav");
    soundBank[SFX_PLAYER_WALKING] = AEAudioLoadSound("Assets/Audio/SFX_PLAYER_WALK.wav");
    soundBank[SFX_SCREAMING] = AEAudioLoadSound("Assets/Audio/SFX_VOICE_SCREAMING.wav");
    soundBank[SFX_VOICE_WHISPER] = AEAudioLoadSound("Assets/Audio/SFX_VOICE_WHISPER.wav");
    soundBank[SFX_WHEELCHAIR_SQUEAK] = AEAudioLoadSound("Assets/Audio/SFX_WHEELCHAIR_SQUEAK.wav");

    // UI
    soundBank[SFX_MAIN_MENU_WRITING_SCRATCH] = AEAudioLoadSound("Assets/Audio/SFX_MAIN_MENU_WRITING_SCRATCH.wav");
    soundBank[SFX_NEW_DAY] = AEAudioLoadSound("Assets/Audio/SFX_UI_NEW_DAY.wav");
    soundBank[SFX_PAGER_NOTIFICATIONS] = AEAudioLoadSound("Assets/Audio/SFX_PAGER_BEEP.wav");
    soundBank[SFX_SWOOSH_1] = AEAudioLoadSound("Assets/Audio/SFX_UI_SWOOSH_1.wav");
    soundBank[SFX_SWOOSH_2] = AEAudioLoadSound("Assets/Audio/SFX_UI_SWOOSH_2.wav");
    soundBank[SFX_SWOOSH_3] = AEAudioLoadSound("Assets/Audio/SFX_UI_SWOOSH_3.wav");
    soundBank[SFX_BUTTON_SELECT] = AEAudioLoadSound("Assets/Audio/SFX_BUTTON_SELECT.wav");
}

void AudioManager_Unload() {
    AEAudioStopGroup(groupSFX);
    AEAudioStopGroup(groupBGM);
    for (int i = 0; i < SND_COUNT; ++i) {
        AEAudioUnloadAudio(soundBank[i]);
    }

    AEAudioUnloadAudioGroup(groupSFX);
    AEAudioUnloadAudioGroup(groupBGM);
}

void AudioManager_PlayBGM(SoundID id, float volume) {
    AEAudioPlay(soundBank[id], groupBGM, volume, 1.0f, -1);
}
void AudioManager_PlaySFX(SoundID id, float volume, float pitch, s32 loops) {
    AEAudioPlay(soundBank[id], groupSFX, volume, pitch, loops);
}

void AudioManager_PlaySpatialSound(SoundID id, float anomalyX, float playerX) {
    float distance = fabsf(anomalyX - playerX);
    float maxDistance = 500.0f;

    float volume = 1.0f - (distance / maxDistance);

    // Clamp values
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    // Only play if audible
    if (volume > 0.0f) {
        AudioManager_PlaySFX(id, volume);
    }
}

void AudioManager_SetBGMVolume(float volume) {
    AEAudioSetGroupVolume(groupBGM, volume);
}

void AudioManager_SetSFXVolume(float volume) {
    AEAudioSetGroupVolume(groupSFX, volume);
}

void AudioManager_StopBGM()
{
    AEAudioStopGroup(groupBGM);
}

void AudioManager_StopSFX()
{
    AEAudioStopGroup(groupSFX);
}
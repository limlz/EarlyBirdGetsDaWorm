#include "pch.hpp"

// --- AUDIO ENGINE DATA ---
static AEAudio      soundBank[SND_COUNT];

// --- CHANNEL ISOLATION ---
static AEAudioGroup groupBGM;
static AEAudioGroup groupSFX;
static AEAudioGroup groupWalking; // Separate group so stopping walk sounds never kills anomaly SFX

/*****************************************************************************************/
// Initializes the audio engine and creates the isolated audio channels.
/*****************************************************************************************/
void AudioManager_Init() {
    AEAudioInit();

    groupBGM = AEAudioCreateGroup();
    groupSFX = AEAudioCreateGroup();
    groupWalking = AEAudioCreateGroup();
}

/*****************************************************************************************/
// --- MEMORY OPTIMIZATION ---
// Centralized loading: Instead of having individual files constantly load and delete 
// their own .wav files (which causes lag spikes and memory fragmentation), the Audio Manager 
// pre-loads everything into RAM at startup. Other files simply trigger them via Enums.
/*****************************************************************************************/
void AudioManager_LoadAll() {
    // BGM
    soundBank[BGM_GAME_AMBIENCE] = AEAudioLoadSound("Assets/Audio/BGM_GAME_AMBIENCE.wav");
    soundBank[BGM_MAIN_MENU] = AEAudioLoadSound("Assets/Audio/BGM_MAIN_MENU.wav");
    soundBank[BGM_RAIN] = AEAudioLoadSound("Assets/Audio/BGM_ENVIRONMENT_RAIN.wav");

    // Environmental
    soundBank[SFX_DOOR_KNOCKING] = AEAudioLoadSound("Assets/Audio/SFX_DOOR_KNOCK.wav");
    soundBank[SFX_LIGHT_FLICKER] = AEAudioLoadSound("Assets/Audio/SFX_LIGHT_FLICKER.wav");
    soundBank[SFX_PLANT_THUD] = AEAudioLoadSound("Assets/Audio/SFX_PLANT_THUD.wav");
    soundBank[SFX_DOOR_KNOCK] = AEAudioLoadSound("Assets/Audio/SFX_DOOR_KNOCK.wav");

    // Characters
    soundBank[SFX_BOSS_FIGHT] = AEAudioLoadSound("Assets/Audio/SFX_BOSS_FIGHT.wav");
    soundBank[SFX_PLAYER_WALKING] = AEAudioLoadSound("Assets/Audio/SFX_PLAYER_WALK.wav");
    soundBank[SFX_SCREAMING] = AEAudioLoadSound("Assets/Audio/SFX_VOICE_SCREAM.wav");
    soundBank[SFX_VOICE_WHISPER] = AEAudioLoadSound("Assets/Audio/SFX_VOICE_WHISPER.wav");
    soundBank[SFX_WHEELCHAIR_SQUEAK] = AEAudioLoadSound("Assets/Audio/SFX_WHEELCHAIR_SQUEAK.wav");
    soundBank[SFX_HEARTBEAT] = AEAudioLoadSound("Assets/Audio/SFX_HEARTBEAT.wav");
    soundBank[SFX_EXHALE] = AEAudioLoadSound("Assets/Audio/SFX_EXHALE.wav");
    soundBank[SFX_CRYING] = AEAudioLoadSound("Assets/Audio/SFX_CRYING.wav");
    soundBank[SFX_CLOCK_TICK] = AEAudioLoadSound("Assets/Audio/SFX_CLOCK_TICK.wav");

    // UI
    soundBank[SFX_MAIN_MENU_WRITING_SCRATCH] = AEAudioLoadSound("Assets/Audio/SFX_MAIN_MENU_WRITING_SCRATCH.wav");
    soundBank[SFX_NEW_DAY] = AEAudioLoadSound("Assets/Audio/SFX_UI_NEW_DAY.wav");
    soundBank[SFX_PAGER_NOTIFICATIONS] = AEAudioLoadSound("Assets/Audio/SFX_PAGER_BEEP.wav");
    soundBank[SFX_SWOOSH_1] = AEAudioLoadSound("Assets/Audio/SFX_UI_SWOOSH_1.wav");
    soundBank[SFX_SWOOSH_2] = AEAudioLoadSound("Assets/Audio/SFX_UI_SWOOSH_2.wav");
    soundBank[SFX_SWOOSH_3] = AEAudioLoadSound("Assets/Audio/SFX_UI_SWOOSH_3.wav");
    soundBank[SFX_TUTORIAL_TYPING] = AEAudioLoadSound("Assets/Audio/SFX_TUTORIAL_TYPING.wav");
    soundBank[SFX_JOURNAL_SELECT] = AEAudioLoadSound("Assets/Audio/SFX_JOURNAL_SELECT.wav");
    soundBank[SFX_JOURNAL_OPEN] = AEAudioLoadSound("Assets/Audio/SFX_JOURNAL_OPEN.wav");
    soundBank[SFX_AA] = AEAudioLoadSound("Assets/Audio/SFX_AA.wav");
    soundBank[SFX_FAH] = AEAudioLoadSound("Assets/Audio/SFX_FAH.wav");
    soundBank[SFX_ELEVATOR_PRESS] = AEAudioLoadSound("Assets/Audio/SFX_ELEVATOR_PRESS.wav");
    soundBank[SFX_ELEVATOR_CLOSE] = AEAudioLoadSound("Assets/Audio/SFX_ELEVATOR_CLOSE.wav");
}

/*****************************************************************************************/
// --- MEMORY SAFETY ---
// Strictly stops all playing audio and unloads every single pointer when the game closes 
// to prevent catastrophic audio buffer memory leaks.
/*****************************************************************************************/
void AudioManager_Unload() {
    AEAudioStopGroup(groupSFX);
    AEAudioStopGroup(groupBGM);
    AEAudioStopGroup(groupWalking);

    for (int i = 0; i < SND_COUNT; ++i) {
        AEAudioUnloadAudio(soundBank[i]);
    }

    AEAudioUnloadAudioGroup(groupSFX);
    AEAudioUnloadAudioGroup(groupBGM);
    AEAudioUnloadAudioGroup(groupWalking);
}

/********************************* PLAYBACK FUNCTIONS ************************************/

void AudioManager_PlayBGM(SoundID id, float volume) {
    AEAudioPlay(soundBank[id], groupBGM, volume, 1.0f, -1);
}

void AudioManager_PlaySFX(SoundID id, float volume, float pitch, s32 loops) {
    AEAudioPlay(soundBank[id], groupSFX, volume, pitch, loops);
}

/*****************************************************************************************/
// --- 2D SPATIAL AUDIO MATH ---
// Dynamically scales the volume of a sound effect based on how far away it is from the camera.
/*****************************************************************************************/
void AudioManager_PlaySpatialSound(SoundID id, float anomalyX, float playerX) {
    // 1. Calculate absolute distance between the sound source and the player
    float distance = fabsf(anomalyX - playerX);
    float maxDistance = 500.0f; // Beyond this pixel range, the sound is completely silent

    // 2. Linear Attenuation: Creates a percentage from 1.0 (right next to player) to 0.0 (far away)
    float volume = 1.0f - (distance / maxDistance);

    // 3. Mathematical Clamping: Prevents glitches where volume becomes negative if anomalies 
    // spawn excessively far off-screen.
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    // 4. Optimization: Only command the audio engine to play the sound if it's actually audible
    if (volume > 0.0f) {
        AudioManager_PlaySFX(id, volume);
    }
}

/********************************* VOLUME CONTROLS ***************************************/

void AudioManager_SetBGMVolume(float volume) {
    AEAudioSetGroupVolume(groupBGM, volume);
}

void AudioManager_SetSFXVolume(float volume) {
    AEAudioSetGroupVolume(groupSFX, volume);
}

/********************************* HALT CONTROLS *****************************************/

void AudioManager_StopBGM()
{
    AEAudioStopGroup(groupBGM);
}

void AudioManager_StopSFX()
{
    AEAudioStopGroup(groupSFX);
}

/********************************* WALKING OVERRIDES *************************************/
// These isolated overrides allow the game to kill the looping wheelchair/footstep sounds 
// instantly during menus or boss fights without muting the active horror SFX in `groupSFX`.

void AudioManager_PlayWalkingSFX(SoundID id, float volume, float pitch, s32 loops)
{
    AEAudioPlay(soundBank[id], groupWalking, volume, pitch, loops);
}

void AudioManager_StopWalkingSFX()
{
    AEAudioStopGroup(groupWalking);
}

void AudioManager_SetWalkingVolume(float volume)
{
    AEAudioSetGroupVolume(groupWalking, volume);
}
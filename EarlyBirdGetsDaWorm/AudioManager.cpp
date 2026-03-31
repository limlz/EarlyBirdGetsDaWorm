#include "pch.hpp"

// --- AUDIO ENGINE DATA ---
static AEAudio      soundBank[static_cast<int>(SoundID::SND_COUNT)];

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
    soundBank[static_cast<int>(SoundID::BGM_GAME_AMBIENCE)] = AEAudioLoadSound("Assets/Audio/BGM_GAME_AMBIENCE.ogg");
    soundBank[static_cast<int>(SoundID::BGM_MAIN_MENU)] = AEAudioLoadSound("Assets/Audio/BGM_MAIN_MENU.ogg");

    // Environmental
    soundBank[static_cast<int>(SoundID::SFX_DOOR_KNOCKING)] = AEAudioLoadSound("Assets/Audio/SFX_DOOR_KNOCK.ogg");
    soundBank[static_cast<int>(SoundID::SFX_LIGHT_FLICKER)] = AEAudioLoadSound("Assets/Audio/SFX_LIGHT_FLICKER.ogg");
    soundBank[static_cast<int>(SoundID::SFX_PLANT_THUD)] = AEAudioLoadSound("Assets/Audio/SFX_PLANT_THUD.ogg");
    soundBank[static_cast<int>(SoundID::SFX_DOOR_KNOCK)] = AEAudioLoadSound("Assets/Audio/SFX_DOOR_KNOCK.ogg");

    // Characters
    soundBank[static_cast<int>(SoundID::SFX_BOSS_FIGHT)] = AEAudioLoadSound("Assets/Audio/SFX_BOSS_FIGHT.ogg");
    soundBank[static_cast<int>(SoundID::SFX_PLAYER_WALKING)] = AEAudioLoadSound("Assets/Audio/SFX_PLAYER_WALK.ogg");
    soundBank[static_cast<int>(SoundID::SFX_SCREAMING)] = AEAudioLoadSound("Assets/Audio/SFX_VOICE_SCREAM.ogg");
    soundBank[static_cast<int>(SoundID::SFX_VOICE_WHISPER)] = AEAudioLoadSound("Assets/Audio/SFX_VOICE_WHISPER.ogg");
    soundBank[static_cast<int>(SoundID::SFX_WHEELCHAIR_SQUEAK)] = AEAudioLoadSound("Assets/Audio/SFX_WHEELCHAIR_SQUEAK.ogg");
    soundBank[static_cast<int>(SoundID::SFX_HEARTBEAT)] = AEAudioLoadSound("Assets/Audio/SFX_HEARTBEAT.ogg");
    soundBank[static_cast<int>(SoundID::SFX_EXHALE)] = AEAudioLoadSound("Assets/Audio/SFX_EXHALE.ogg");
    soundBank[static_cast<int>(SoundID::SFX_CRYING)] = AEAudioLoadSound("Assets/Audio/SFX_CRYING.ogg");
    soundBank[static_cast<int>(SoundID::SFX_CLOCK_TICK)] = AEAudioLoadSound("Assets/Audio/SFX_CLOCK_TICK.ogg");

    // Boss Fight
    soundBank[static_cast<int>(SoundID::SFX_JUMP_GRUNT)]        = AEAudioLoadSound("Assets/Audio/SFX_JUMP_GRUNT.ogg");
    soundBank[static_cast<int>(SoundID::SFX_WEOW)]              = AEAudioLoadSound("Assets/Audio/SFX_WEOW.ogg");
    soundBank[static_cast<int>(SoundID::SFX_WATER_GUN_SQUIRT)]  = AEAudioLoadSound("Assets/Audio/SFX_WATER_GUN_SQUIRT.ogg");
    soundBank[static_cast<int>(SoundID::SFX_WATER_IMPACT)]      = AEAudioLoadSound("Assets/Audio/SFX_WATER_IMPACT.ogg");
    soundBank[static_cast<int>(SoundID::SFX_SHIELD_BREAK)]      = AEAudioLoadSound("Assets/Audio/SFX_SHIELD_BREAK.ogg");
    soundBank[static_cast<int>(SoundID::SFX_DODGE)]             = AEAudioLoadSound("Assets/Audio/SFX_DODGE.ogg");

    // UI
    soundBank[static_cast<int>(SoundID::SFX_MAIN_MENU_WRITING_SCRATCH)] = AEAudioLoadSound("Assets/Audio/SFX_MAIN_MENU_WRITING_SCRATCH.ogg");
    soundBank[static_cast<int>(SoundID::SFX_NEW_DAY)] = AEAudioLoadSound("Assets/Audio/SFX_UI_NEW_DAY.ogg");
    soundBank[static_cast<int>(SoundID::SFX_PAGER_NOTIFICATIONS)] = AEAudioLoadSound("Assets/Audio/SFX_PAGER_BEEP.ogg");
    soundBank[static_cast<int>(SoundID::SFX_TUTORIAL_TYPING)] = AEAudioLoadSound("Assets/Audio/SFX_TUTORIAL_TYPING.ogg");
    soundBank[static_cast<int>(SoundID::SFX_JOURNAL_SELECT)] = AEAudioLoadSound("Assets/Audio/SFX_JOURNAL_SELECT.ogg");
    soundBank[static_cast<int>(SoundID::SFX_JOURNAL_OPEN)] = AEAudioLoadSound("Assets/Audio/SFX_JOURNAL_OPEN.ogg");
    soundBank[static_cast<int>(SoundID::SFX_AA)] = AEAudioLoadSound("Assets/Audio/SFX_AA.ogg");
    soundBank[static_cast<int>(SoundID::SFX_FAH)] = AEAudioLoadSound("Assets/Audio/SFX_FAH.ogg");
    soundBank[static_cast<int>(SoundID::SFX_ELEVATOR_PRESS)] = AEAudioLoadSound("Assets/Audio/SFX_ELEVATOR_PRESS.ogg");
    soundBank[static_cast<int>(SoundID::SFX_ELEVATOR_CLOSE)] = AEAudioLoadSound("Assets/Audio/SFX_ELEVATOR_CLOSE.ogg");
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

    for (int i = 0; i < static_cast<int>(SoundID::SND_COUNT); ++i) {
        AEAudioUnloadAudio(soundBank[i]);
    }

    AEAudioUnloadAudioGroup(groupSFX);
    AEAudioUnloadAudioGroup(groupBGM);
    AEAudioUnloadAudioGroup(groupWalking);

    AEAudioExit(); // release audio engine internal state (matches AEAudioInit in AudioManager_Init)
}

/********************************* PLAYBACK FUNCTIONS ************************************/

void AudioManager_PlayBGM(SoundID id, float volume) {
    AEAudioPlay(soundBank[static_cast<int>(id)], groupBGM, volume, 1.0f, -1);
}

void AudioManager_PlaySFX(SoundID id, float volume, float pitch, s32 loops) {
    AEAudioPlay(soundBank[static_cast<int>(id)], groupSFX, volume, pitch, loops);
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
    AEAudioPlay(soundBank[static_cast<int>(id)], groupWalking, volume, pitch, loops);
}

void AudioManager_StopWalkingSFX()
{
    AEAudioStopGroup(groupWalking);
}

void AudioManager_SetWalkingVolume(float volume)
{
    AEAudioSetGroupVolume(groupWalking, volume);
}
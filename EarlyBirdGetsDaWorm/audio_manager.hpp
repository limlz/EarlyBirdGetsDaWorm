#pragma once

// ============================================================================
// ENUMS & DATA TYPES
// ============================================================================

enum class SoundID {

    // ------------- 1. BACKGROUND MUSIC (BGM) ------------- //

    BGM_GAME_AMBIENCE,              // BGM_GAME_AMBIENCE.wav
    BGM_MAIN_MENU,                  // BGM_MAIN_MENU.wav

    // ---------------- 2. ENVIRONMENTAL SFX --------------- //

    SFX_DOOR_KNOCKING,              // SFX_DOOR_KNOCKING.wav
    SFX_LIGHT_FLICKER,              // SFX_LIGHT_BUZZING.wav
    SFX_PLANT_THUD,                 // SFX_PLANT_DROP_THONK.wav
    SFX_DOOR_KNOCK,                 // SFX_DOOR_KNOCK.wav

    // ------------- 3. PLAYER & CHARACTER SFX ------------- //

    SFX_BOSS_FIGHT,                 // SFX_BOSS_FIGHT.wav
    SFX_PLAYER_WALKING,             // SFX_PLAYER_WALKING.wav
    SFX_SCREAMING,                  // SFX_SCREAMING.wav
    SFX_VOICE_WHISPER,              // SFX_VOICE_WHISPER.wav
    SFX_WHEELCHAIR_SQUEAK,          // SFX_WHEELCHAIR_SQUEAK.wav
    SFX_HEARTBEAT,                  // SFX_HEARTBEAT.wav
    SFX_EXHALE,                     // SFX_EXHALE.wav
    SFX_CRYING,                     // SFX_CRYING.wav
    SFX_CLOCK_TICK,                 // SFX_CLOCK_TICK.wav

    // ----------------- 4. BOSS FIGHT SFX ----------------- //

    SFX_JUMP_GRUNT,                 // SFX_JUMP_GRUNT.wav
    SFX_WEOW,                       // SFX_WEOW.wav  (horror-less jump replacement)
    SFX_WATER_GUN_SQUIRT,           // SFX_WATER_GUN_SQUIRT.wav
    SFX_WATER_IMPACT,               // SFX_WATER_IMPACT.wav
    SFX_SHIELD_BREAK,               // SFX_SHIELD_BREAK.wav
    SFX_DODGE,                      // SFX_DODGE.wav

    // ----------------- 5. UI & SYSTEM SFX ---------------- //

    SFX_MAIN_MENU_WRITING_SCRATCH,  // SFX_MAIN_MENU_WRITING_SCRATCH.wav
    SFX_NEW_DAY,                    // SFX_NEW_DAY.wav
    SFX_PAGER_NOTIFICATIONS,        // SFX_PAGER_NOTIFICATIONS.wav
    SFX_JOURNAL_SELECT,             // SFX_JOURNAL_SELECT.wav
    SFX_JOURNAL_OPEN,               // SFX_JOURNAL_OPEN.wav
    SFX_TUTORIAL_TYPING,            // SFX_TUTORIAL_TYPING.wav
    SFX_AA,                         // SFX_AA.wav
    SFX_FAH,                        // SFX_FAH.wav
    SFX_ELEVATOR_PRESS,             // SFX_ELEVATOR_PRESS.wav
    SFX_ELEVATOR_CLOSE,             // SFX_ELEVATOR_CLOSE.wav

    SND_COUNT
};

// ============================================================================
// SYSTEM LIFECYCLE (INIT / LOAD / UNLOAD)
// ============================================================================

/******************************************************************************************
    @brief  Initializes the audio engine and creates the isolated audio channels.
******************************************************************************************/
void AudioManager_Init();

/******************************************************************************************
    @brief  Pre-loads all sound files into RAM to prevent runtime lag spikes and memory fragmentation.
******************************************************************************************/
void AudioManager_LoadAll();

/******************************************************************************************
    @brief  Strictly stops all playing audio and unloads every single pointer to prevent
            catastrophic audio buffer memory leaks on game exit.
******************************************************************************************/
void AudioManager_Unload();


// ============================================================================
// PLAYBACK
// ============================================================================

/******************************************************************************************
    @brief  Plays background music on the isolated BGM channel.

    @param  id
            The ID of the music track to play.
    @param  volume
            The volume level from 0.0f (mute) to 1.0f (max).
******************************************************************************************/
void AudioManager_PlayBGM(SoundID id, float volume);

/******************************************************************************************
    @brief  Plays a sound effect on the primary SFX channel.

    @param  id
            The ID of the sound effect to play.
    @param  volume
            The volume level from 0.0f (mute) to 1.0f (max).
    @param  pitch
            The playback speed/pitch. Defaults to 1.0f.
    @param  loops
            How many times to loop the sound. Defaults to 0 (play once). -1 for infinite loop.
******************************************************************************************/
void AudioManager_PlaySFX(SoundID id, float volume, float pitch = 1.f, s32 loops = 0);


// ============================================================================
// VOLUME CONTROLS
// ============================================================================

/******************************************************************************************
    @brief  Sets the master volume for the Background Music channel.

    @param  volume
            The volume level from 0.0f (mute) to 1.0f (max).
******************************************************************************************/
void AudioManager_SetBGMVolume(float volume);

/******************************************************************************************
    @brief  Sets the master volume for the primary SFX channel.

    @param  volume
            The volume level from 0.0f (mute) to 1.0f (max).
******************************************************************************************/
void AudioManager_SetSFXVolume(float volume);

/******************************************************************************************
    @brief  Sets the master volume for the isolated Walking channel.

    @param  volume
            The volume level from 0.0f (mute) to 1.0f (max).
******************************************************************************************/
void AudioManager_SetWalkingVolume(float volume);


// ============================================================================
// HALT CONTROLS
// ============================================================================

/******************************************************************************************
    @brief  Instantly halts all audio currently playing on the BGM channel.
******************************************************************************************/
void AudioManager_StopBGM();

/******************************************************************************************
    @brief  Instantly halts all audio currently playing on the primary SFX channel.
******************************************************************************************/
void AudioManager_StopSFX();


// ============================================================================
// WALKING/MOVEMENT OVERRIDES
// ============================================================================

/******************************************************************************************
    @brief  Plays a sound effect on the dedicated Walking channel. This isolation ensures
            stopping footstep sounds never accidentally kills ambient horror SFX.

    @param  id
            The ID of the movement sound effect to play.
    @param  volume
            The volume level from 0.0f (mute) to 1.0f (max).
    @param  pitch
            The playback speed/pitch. Defaults to 1.0f.
    @param  loops
            How many times to loop the sound. Defaults to 0 (play once). -1 for infinite loop.
******************************************************************************************/
void AudioManager_PlayWalkingSFX(SoundID id, float volume, float pitch = 1.f, s32 loops = 0);

/******************************************************************************************
    @brief  Instantly halts all audio currently playing on the dedicated Walking channel.
******************************************************************************************/
void AudioManager_StopWalkingSFX();
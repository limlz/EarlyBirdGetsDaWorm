#include "pch.hpp"

namespace AudioManager
{
    // The individual sound files
    static AEAudio scratchSounds[3];

    // The channel/group the sounds will play on
    static AEAudioGroup sfxGroup;

    void LoadMainMenuSounds()
    {
        // Create an audio group for audio to be played on
        sfxGroup = AEAudioCreateGroup();

        scratchSounds[0] = AEAudioLoadSound("Assets/Audio/scratch.mp3");
        scratchSounds[1] = AEAudioLoadSound("Assets/Audio/scratch.mp3");
        scratchSounds[2] = AEAudioLoadSound("Assets/Audio/scratch.mp3");
    }

    void PlayRandomScratch()
    {
        // Pick a random index: 0, 1, or 2
        int randomIndex = rand() % 3;

        // Check if both the AEAudio and the AEAudioGroup are valid
        if (AEAudioIsValidAudio(scratchSounds[randomIndex]) && AEAudioIsValidGroup(sfxGroup))
        {
            float randomPitch = 0.9f + (float)(rand() % 21) / 100.0f;

            AEAudioPlay(scratchSounds[randomIndex], sfxGroup, 1.0f, randomPitch, 0);
        }
    }

    void UnloadMainMenuSounds()
    {
        // Loop through the array and free all 3 sounds
        for (int i = 0; i < 3; i++)
        {
            if (AEAudioIsValidAudio(scratchSounds[i]))
            {
                AEAudioUnloadAudio(scratchSounds[i]);
            }
        }

        // Unload resources allocated for the audio group
        if (AEAudioIsValidGroup(sfxGroup))
        {
            AEAudioUnloadAudioGroup(sfxGroup);
        }
    }
}
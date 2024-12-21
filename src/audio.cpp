#include "audio.h"
#include <stdio.h>

// Set master volume
real32 master_volume__percent = 20.0f;
int32 master_volume = (int32)((master_volume__percent / 100) * MIX_MAX_VOLUME);

bool32 audio_init(Audio_Context* ctx)
{
    if (!Mix_OpenAudio(0, 0))
    {
        fprintf(stderr, "SDL_mixer could not initialize! Mix_Error: %s\n", SDL_GetError());
        return 0;
    }

    Mix_Volume(-1, master_volume);
    Mix_VolumeMusic(master_volume);

    ctx->start_screen_background_music = Mix_LoadMUS("assets/music/pretty-love-crystalline-synth-melody_142bpm_G_major.wav");
    if (!ctx->start_screen_background_music)
    {
        fprintf(stderr, "Failed to load start screen background music! Mix_Error: %s\n", SDL_GetError());
    }

    ctx->gameplay_background_music = Mix_LoadMUS("assets/music/mixkit-feast-from-the-east.mp3");
    if (!ctx->gameplay_background_music)
    {
        fprintf(stderr, "Failed to load gameplay background music! Mix_Error: %s\n", SDL_GetError());
    }

    ctx->effect_beep = Mix_LoadWAV("assets/sounds/beep.wav");
    if (!ctx->effect_beep)
    {
        fprintf(stderr, "Failed to load beep sound effect! Mix_Error: %s\n", SDL_GetError());
    }

    ctx->effect_beep_2 = Mix_LoadWAV("assets/sounds/beep-2.wav");

    if (!ctx->effect_beep_2)
    {
        fprintf(stderr, "Failed to load beep 2 sound effect! Mix_Error: %s\n", SDL_GetError());
    }

    ctx->effect_boom = Mix_LoadWAV("assets/sounds/boom.wav");

    if (!ctx->effect_boom)
    {
        fprintf(stderr, "Failed to load boom sound effect! Mix_Error: %s\n", SDL_GetError());
    }

    return 1;
}

void audio_cleanup(Audio_Context* ctx)
{
    if (ctx->start_screen_background_music)
    {
        Mix_FreeMusic(ctx->start_screen_background_music);
    }
    if (ctx->gameplay_background_music)
    {
        Mix_FreeMusic(ctx->gameplay_background_music);
    }
    if (ctx->effect_beep)
    {
        Mix_FreeChunk(ctx->effect_beep);
    }
    if (ctx->effect_beep_2)
    {
        Mix_FreeChunk(ctx->effect_beep_2);
    }
    Mix_CloseAudio();
}

void play_music(Mix_Music* music)
{
    if (music)
    {
        Mix_PlayMusic(music, -1);
    }
}

void stop_music(void)
{
    Mix_HaltMusic();
}

// Volume given as percent
void set_music_volume(real32 volume)
{
    real32 percent = volume / 100;
    Mix_VolumeMusic((int32)(master_volume * percent));
}

void play_sound_effect(Mix_Chunk* effect)
{
    if (effect)
    {
        Mix_PlayChannel(-1, effect, 0);
    }
}

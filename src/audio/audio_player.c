#include "audio_player.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Desktop MP3 player
 *
 * MP3
 *   ↓
 * dr_mp3
 *   ↓
 * decoded float PCM
 *   ↓
 * SDL audio conversion
 *   ↓
 * SDL2 audio device
 *   ↓
 * Mac speakers / headphones
 */


static SDL_AudioDeviceID audio_device = 0;

static float *audio_buffer = NULL;

static Uint64 audio_frame_count = 0;

static int audio_channels = 0;

static int audio_sample_rate = 0;

static Uint64 audio_position_frames = 0;

static int audio_playing = 0;

static int audio_initialized = 0;


/*
 * SDL output format.
 *
 * We use stereo float at 48 kHz because the MP3 files
 * we're testing with are already 48 kHz.
 *
 * SDL will tell us the actual obtained format.
 */
static SDL_AudioSpec obtained_spec;


/*
 * SDL audio callback.
 */
static void audio_callback(
    void *userdata,
    Uint8 *stream,
    int len
)
{
    (void)userdata;


    if(!audio_playing ||
       !audio_buffer ||
       audio_frame_count == 0)
    {
        SDL_memset(
            stream,
            0,
            (size_t)len
        );

        return;
    }


    int bytes_per_frame =
        audio_channels *
        (int)sizeof(float);


    if(bytes_per_frame <= 0)
    {
        SDL_memset(
            stream,
            0,
            (size_t)len
        );

        return;
    }


    Uint64 requested_frames =
        (Uint64)len /
        (Uint64)bytes_per_frame;


    Uint64 remaining_frames =
        audio_frame_count -
        audio_position_frames;


    Uint64 frames_to_copy =
        requested_frames;


    if(frames_to_copy > remaining_frames)
    {
        frames_to_copy =
            remaining_frames;
    }


    size_t bytes_to_copy =
        (size_t)(
            frames_to_copy *
            (Uint64)bytes_per_frame
        );


    if(bytes_to_copy > 0)
    {
        memcpy(
            stream,
            audio_buffer +
                (
                    audio_position_frames *
                    (Uint64)audio_channels
                ),
            bytes_to_copy
        );


        audio_position_frames +=
            frames_to_copy;
    }


    if(bytes_to_copy < (size_t)len)
    {
        memset(
            stream + bytes_to_copy,
            0,
            (size_t)len - bytes_to_copy
        );


        if(audio_position_frames >=
           audio_frame_count)
        {
            audio_playing = 0;
        }
    }
}


int audio_player_init(void)
{
    if(audio_initialized)
    {
        return 0;
    }


    if(SDL_WasInit(SDL_INIT_AUDIO) == 0)
    {
        if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        {
            printf(
                "Audio player: SDL_InitSubSystem failed: %s\n",
                SDL_GetError()
            );

            return -1;
        }
    }


    SDL_AudioSpec desired;

    SDL_zero(desired);


    /*
     * Match the MP3 files we're currently testing.
     */
    desired.freq = 48000;

    desired.format = AUDIO_F32SYS;

    desired.channels = 2;

    desired.samples = 2048;

    desired.callback =
        audio_callback;

    desired.userdata = NULL;


    SDL_zero(obtained_spec);


    audio_device =
        SDL_OpenAudioDevice(
            NULL,
            0,
            &desired,
            &obtained_spec,
            0
        );


    if(audio_device == 0)
    {
        printf(
            "Audio player: SDL_OpenAudioDevice failed: %s\n",
            SDL_GetError()
        );

        return -1;
    }


    printf(
        "SDL audio device:\n"
        "  Frequency: %d Hz\n"
        "  Channels: %d\n"
        "  Format: 0x%04x\n",
        obtained_spec.freq,
        obtained_spec.channels,
        obtained_spec.format
    );


    /*
     * This first version expects SDL to give us
     * stereo floating-point PCM.
     */
    if(obtained_spec.freq != 48000 ||
       obtained_spec.channels != 2 ||
       obtained_spec.format != AUDIO_F32SYS)
    {
        printf(
            "Audio player: unexpected SDL output format.\n"
        );

        SDL_CloseAudioDevice(
            audio_device
        );

        audio_device = 0;

        return -1;
    }


    audio_initialized = 1;


    printf(
        "Audio player initialized.\n"
    );


    return 0;
}


void audio_player_shutdown(void)
{
    if(!audio_initialized)
    {
        return;
    }


    audio_playing = 0;


    if(audio_device != 0)
    {
        SDL_CloseAudioDevice(
            audio_device
        );

        audio_device = 0;
    }


    if(audio_buffer)
    {
        free(audio_buffer);

        audio_buffer = NULL;
    }


    audio_frame_count = 0;

    audio_position_frames = 0;

    audio_channels = 0;

    audio_sample_rate = 0;


    audio_initialized = 0;
}


int audio_player_play(
    const char *path
)
{
    if(!audio_initialized)
    {
        printf(
            "Audio player is not initialized.\n"
        );

        return -1;
    }


    if(!path)
    {
        printf(
            "Audio player: NULL path.\n"
        );

        return -1;
    }


    printf(
        "Audio player loading:\n%s\n",
        path
    );


    /*
     * Stop current playback.
     */
    SDL_LockAudioDevice(
        audio_device
    );

    audio_playing = 0;

    SDL_UnlockAudioDevice(
        audio_device
    );


    /*
     * Free previous decoded song.
     */
    if(audio_buffer)
    {
        free(audio_buffer);

        audio_buffer = NULL;
    }


    audio_frame_count = 0;

    audio_position_frames = 0;

    audio_channels = 0;

    audio_sample_rate = 0;


    /*
     * Decode MP3.
     */
    drmp3_config config;

    memset(
        &config,
        0,
        sizeof(config)
    );


    drmp3_uint64 total_frames = 0;


    float *decoded =
        drmp3_open_file_and_read_pcm_frames_f32(
            path,
            &config,
            &total_frames,
            NULL
        );


    if(!decoded ||
       total_frames == 0)
    {
        printf(
            "Audio player: failed to decode MP3.\n"
        );


        if(decoded)
        {
            drmp3_free(
                decoded,
                NULL
            );
        }


        return -1;
    }


    audio_buffer = decoded;

    audio_frame_count = total_frames;

    audio_channels = config.channels;

    audio_sample_rate = config.sampleRate;


    printf(
        "Decoded MP3:\n"
        "  Channels: %d\n"
        "  Sample rate: %d Hz\n"
        "  Frames: %llu\n"
        "  Duration: %.2f seconds\n",
        audio_channels,
        audio_sample_rate,
        (unsigned long long)audio_frame_count,
        audio_player_get_duration()
    );


    /*
     * For this first implementation, require the MP3
     * sample rate and channel count to match the SDL
     * device.
     *
     * This is NOT a permanent limitation.
     *
     * Once basic playback is confirmed, we'll add
     * SDL_AudioCVT conversion so 44.1 kHz, mono,
     * 48 kHz, etc. all work automatically.
     */
    if(audio_channels != obtained_spec.channels ||
       audio_sample_rate != obtained_spec.freq)
    {
        printf(
            "Audio player: MP3 format does not match "
            "SDL output format.\n"
        );

        printf(
            "  MP3: %d channels / %d Hz\n",
            audio_channels,
            audio_sample_rate
        );

        printf(
            "  SDL: %d channels / %d Hz\n",
            obtained_spec.channels,
            obtained_spec.freq
        );


        free(audio_buffer);

        audio_buffer = NULL;

        audio_frame_count = 0;

        audio_channels = 0;

        audio_sample_rate = 0;

        return -1;
    }


    audio_position_frames = 0;


    /*
     * Start playback.
     */
    SDL_LockAudioDevice(
        audio_device
    );

    audio_playing = 1;

    SDL_UnlockAudioDevice(
        audio_device
    );


    SDL_PauseAudioDevice(
        audio_device,
        0
    );


    printf(
        "Audio playback started.\n"
    );


    return 0;
}


void audio_player_pause(void)
{
    if(!audio_initialized)
    {
        return;
    }


    SDL_LockAudioDevice(
        audio_device
    );

    audio_playing = 0;

    SDL_UnlockAudioDevice(
        audio_device
    );
}


void audio_player_resume(void)
{
    if(!audio_initialized)
    {
        return;
    }


    if(!audio_buffer ||
       audio_frame_count == 0)
    {
        return;
    }


    if(audio_position_frames >=
       audio_frame_count)
    {
        return;
    }


    SDL_LockAudioDevice(
        audio_device
    );

    audio_playing = 1;

    SDL_UnlockAudioDevice(
        audio_device
    );


    SDL_PauseAudioDevice(
        audio_device,
        0
    );
}


void audio_player_stop(void)
{
    if(!audio_initialized)
    {
        return;
    }


    SDL_LockAudioDevice(
        audio_device
    );

    audio_playing = 0;

    SDL_UnlockAudioDevice(
        audio_device
    );


    audio_position_frames = 0;
}


int audio_player_is_playing(void)
{
    return audio_playing;
}


double audio_player_get_position(void)
{
    if(audio_sample_rate <= 0)
    {
        return 0.0;
    }


    return (
        (double)audio_position_frames /
        (double)audio_sample_rate
    );
}


double audio_player_get_duration(void)
{
    if(audio_sample_rate <= 0)
    {
        return 0.0;
    }


    return (
        (double)audio_frame_count /
        (double)audio_sample_rate
    );
}

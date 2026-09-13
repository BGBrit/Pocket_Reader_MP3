#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initializes the desktop audio player.
 *
 * Returns 0 on success.
 * Returns -1 on failure.
 */
int audio_player_init(void);

/*
 * Shuts down the desktop audio player.
 */
void audio_player_shutdown(void);

/*
 * Loads and starts playing an MP3 file.
 *
 * Returns 0 on success.
 * Returns -1 on failure.
 */
int audio_player_play(const char *path);

/*
 * Pauses playback.
 */
void audio_player_pause(void);

/*
 * Resumes playback.
 */
void audio_player_resume(void);

/*
 * Stops playback and clears the current song.
 */
void audio_player_stop(void);

/*
 * Returns 1 if audio is currently playing.
 * Returns 0 otherwise.
 */
int audio_player_is_playing(void);

/*
 * Returns the current playback position in seconds.
 */
double audio_player_get_position(void);

/*
 * Returns the current song duration in seconds.
 */
double audio_player_get_duration(void);

#ifdef __cplusplus
}
#endif

#endif

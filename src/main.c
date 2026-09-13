/**
 * @file main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef _MSC_VER
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"

#include <SDL.h>

#include "audio/playlist.h"
#include "audio/audio_library.h"
#include "audio/audio_player.h"
#include "hal/hal.h"


/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/


/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void init_pocket_reader_ui(void);


#if LV_USE_OS != LV_OS_FREERTOS

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;


    /*
     * Initialize LVGL.
     */
    lv_init();


    /*
     * Initialize the playlist system.
     */
    playlist_init();


    /*
     * Initialize SDL/LVGL desktop hardware.
     *
     * This must happen before the SDL audio player
     * is initialized.
     */
    sdl_hal_init(240, 320);


    /*
     * Initialize the audio library.
     *
     * This scans:
     *
     * /Users/beaubritain/Desktop/fakemicroSD/songs
     *
     * and builds the song list.
     */
    audio_library_init();


    /*
     * Initialize the desktop audio player.
     */
    if(audio_player_init() != 0)
    {
        printf(
            "WARNING: Audio player initialization failed.\n"
        );
    }
    else
    {
        /*
         * TEMPORARY TEST:
         *
         * Play the first MP3 in the library.
         *
         * We will remove this test once we know
         * desktop playback works.
         */
        if(audio_library_get_count() > 0)
        {
            const char *path =
                audio_library_get_path(0);


            printf(
                "\n"
                "========================================\n"
                "AUDIO PLAYBACK TEST\n"
                "========================================\n"
                "Song 0:\n"
                "%s\n"
                "========================================\n"
                "\n",
                path
            );


            if(audio_player_play(path) != 0)
            {
                printf(
                    "Audio playback test failed.\n"
                );
            }
        }
        else
        {
            printf(
                "No MP3 files found for playback test.\n"
            );
        }
    }


    /*
     * Start your normal Pocket Reader UI.
     */
    init_pocket_reader_ui();


    /*
     * Main LVGL loop.
     */
    while(1)
    {
        uint32_t sleep_time_ms =
            lv_timer_handler();


        if(sleep_time_ms == LV_NO_TIMER_READY)
        {
            sleep_time_ms =
                LV_DEF_REFR_PERIOD;
        }


#ifdef _MSC_VER

        Sleep(sleep_time_ms);

#else

        usleep(
            sleep_time_ms * 1000
        );

#endif
    }


    return 0;
}


#endif


/**********************
 *   STATIC FUNCTIONS
 **********************/

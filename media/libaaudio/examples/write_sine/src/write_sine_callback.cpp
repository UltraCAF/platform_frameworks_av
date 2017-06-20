/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Play sine waves using an AAudio callback.

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <aaudio/AAudio.h>
#include "AAudioExampleUtils.h"
#include "AAudioSimplePlayer.h"

#define NUM_SECONDS              5

// Application data that gets passed to the callback.
#define MAX_FRAME_COUNT_RECORDS    256

int main(int argc, char **argv)
{
    (void)argc; // unused
    AAudioSimplePlayer player;
    SineThreadedData_t myData;
    aaudio_result_t result;

    // Make printf print immediately so that debug info is not stuck
    // in a buffer if we hang or crash.
    setvbuf(stdout, nullptr, _IONBF, (size_t) 0);
    printf("%s - Play a sine sweep using an AAudio callback\n", argv[0]);

    myData.schedulerChecked = false;

    result = player.open(2, 44100, AAUDIO_FORMAT_PCM_FLOAT,
                         SimplePlayerDataCallbackProc, SimplePlayerErrorCallbackProc, &myData);
    if (result != AAUDIO_OK) {
        fprintf(stderr, "ERROR -  player.open() returned %d\n", result);
        goto error;
    }
    printf("player.getFramesPerSecond() = %d\n", player.getFramesPerSecond());
    printf("player.getChannelCount() = %d\n", player.getChannelCount());
    myData.sineOsc1.setup(440.0, 48000);
    myData.sineOsc1.setSweep(300.0, 600.0, 5.0);
    myData.sineOsc2.setup(660.0, 48000);
    myData.sineOsc2.setSweep(350.0, 900.0, 7.0);

#if 0
    result = player.prime(); // FIXME crashes AudioTrack.cpp
    if (result != AAUDIO_OK) {
        fprintf(stderr, "ERROR - player.prime() returned %d\n", result);
        goto error;
    }
#endif

    result = player.start();
    if (result != AAUDIO_OK) {
        fprintf(stderr, "ERROR - player.start() returned %d\n", result);
        goto error;
    }

    printf("Sleep for %d seconds while audio plays in a callback thread.\n", NUM_SECONDS);
    for (int second = 0; second < NUM_SECONDS; second++)
    {
        const struct timespec request = { .tv_sec = 1, .tv_nsec = 0 };
        (void) clock_nanosleep(CLOCK_MONOTONIC, 0 /*flags*/, &request, NULL /*remain*/);

        aaudio_stream_state_t state;
        result = AAudioStream_waitForStateChange(player.getStream(),
                                                 AAUDIO_STREAM_STATE_CLOSED,
                                                 &state,
                                                 0);
        if (result != AAUDIO_OK) {
            fprintf(stderr, "ERROR - AAudioStream_waitForStateChange() returned %d\n", result);
            goto error;
        }
        if (state != AAUDIO_STREAM_STATE_STARTING && state != AAUDIO_STREAM_STATE_STARTED) {
            printf("Stream state is %d %s!\n", state, AAudio_convertStreamStateToText(state));
            break;
        }
        printf("framesWritten = %d\n", (int) AAudioStream_getFramesWritten(player.getStream()));
    }
    printf("Woke up now.\n");

    printf("call stop()\n");
    result = player.stop();
    if (result != AAUDIO_OK) {
        goto error;
    }
    printf("call close()\n");
    result = player.close();
    if (result != AAUDIO_OK) {
        goto error;
    }

    if (myData.schedulerChecked) {
        printf("scheduler = 0x%08x, SCHED_FIFO = 0x%08X\n",
               myData.scheduler,
               SCHED_FIFO);
    }

    printf("SUCCESS\n");
    return EXIT_SUCCESS;
error:
    player.close();
    printf("exiting - AAudio result = %d = %s\n", result, AAudio_convertResultToText(result));
    return EXIT_FAILURE;
}


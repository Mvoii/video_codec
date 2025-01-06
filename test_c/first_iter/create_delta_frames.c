// create_delta_frames.c
#include "codec.h"

/**
* create_delta_frames - Create delta frames from frame sequence
* @frames: array of frames
* @frame_count: Number of frames
*
* Create delta frames by subtracting consecutive frames
*/
void create_delta_frames(video_frame *frames, int frame_count)
{
    for (int i = frame_count - 1; i > 0; i--) {
        printf("\ndelta in %d frame\n", i);
        for (int j = 0; j < frames[i].size; j++) {
            // printf("delta in %d frame  ", j);
            frames[i].data[j] -= frames[i-1].data[j];
        }
        // printf("\n");
    }
}


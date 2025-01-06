// read_frames.c
#include "codec.h"
#include <stddef.h>
#include <stdio.h>

/**
* read_frames - Read raw video frames from file
* @ctx: Encoder context
* @filename: Input filename
* @frames: pointer to array of frames
* @frame_count: Pointer to store number of frames read
*
* Return: 0 on success, -1 on failure
*/
int read_frames(encoder_context *ctx, const char *filename, video_frame **frames, int *frame_count)
{
    FILE *fp;
    unsigned char *buffer;
    size_t bytes_read;
    int count = 0;
    video_frame *frame_array;

    fp = fopen(filename, "rb");
     if (!fp) {
        fprintf(stderr, "Error opening input file\n");
        return -1;
    }

    /* count frames and allocate memory */
    buffer = malloc(ctx->frame_size);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    while ((bytes_read = fread(buffer, 1, ctx->frame_size, fp)))
        count++;

    *frame_count = count;
    frame_array = malloc(count * sizeof(video_frame));
    if (!frame_array) {
        free(buffer);
        fclose(fp);
        return -1;
    }

    /* read rfames into array*/
    fseek(fp, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        frame_array[i].data = malloc(ctx->frame_size);
        if (!frame_array[i].data) {
            for (int j = 0; j < i; j++)
                 free(frame_array);
            free(frame_array);
            free(buffer);
            fclose(fp);
            return -1;
        }
        frame_array[i].size = ctx->frame_size;
        fread(frame_array[i].data, 1, ctx->frame_size, fp);
    }
    *frames = frame_array;
    free(buffer);
    fclose(fp);
    printf("Done reading file...\n");
    return 0;
}


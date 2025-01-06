// vid_codec.c
#include "codec.h"

/**
 * main - Entry point
 * @argc: Argument count
 * @argv: Argument array
 * 
 * Return: 0 on success, 1 on failure
 */
int main(int argc, char *argv)
{
    encoder_context ctx;
    video_frame *frames;
    int frame_count;
    unsigned char *compressed;
    size_t compressed_size;

    /* init the encoder */
    init_encoder(&ctx, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    /* read input frames */
    if (read_frames(&ctx, "video.rgb24", &frames, &frame_count) != 0)
    {
        fprintf(stderr, "Failed to read input from video\n");
        return 1;
    }

    printf("Reading %d frames\n", frame_count);

    /* convert frames to yuv420 */
    printf("converting to yuv420 ...\n");
    for (int i = 0; i < frame_count; i++)
        convert_to_yuv420(&ctx, &frames[i]);
    
    printf("Creating delta frames...\n");
    create_delta_frames(frames, frame_count);

    printf("compressing the frames ....\n");
    compressed = compressed_frames(frames, frame_count, &compressed_size);
    if (!compressed)
    {
        fprintf(stderr, "Compression failed\n");
        return 1;
    }

    printf("Compressed size: %zu bytes (%.2f%% of original size)\n", compressed_size, 100.0f * compressed_size / (ctx.frame_size * frame_count));

    FILE *fp = fopen("encoded.bin", "wb");
    if (fp)
    {
        fwrite(compressed, 1, compressed_size, fp);
        fclose(fp);
    }

    for (int i = 0; i < frame_count; i++)
        free(frames[i].data);
    free(frames);
    free(compressed);

    return 0;
}

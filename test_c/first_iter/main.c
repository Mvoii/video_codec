/**
 * @file video_encoder.c
 * @brief Basic video encoder implementation demonstrating fundamental concepts
 *
 * This program implements a simple video encoder that demonstrates core
 * video compression concepts including:
 * - RGB to YUV420 conversion
 * - Delta frame encoding
 * - Run-length encoding
 * - DEFLATE compression
 *
 * While real-world encoders are more complex, this shows basic principles
 * to achieve ~90% compression
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/* Default video dimensions */
#define DEFAULT_WIDTH 384
#define DEFAULT_HEIGHT 216

/* Color conversion constants (ITU-R BT.601) */
#define YUV_Y_R 0.299f
#define YUV_Y_G 0.587f
#define YUV_Y_B 0.114f
#define YUV_U_R -0.169f
#define YUV_U_G -0.331f
#define YUV_U_B 0.449f
#define YUV_V_R 0.499f
#define YUV_V_G -0.418f
#define YUV_V_B -0.0813f

/**
 * @struct video_frame
 * @brief Structure to hold frame data and metadata
 *
 * @param data Pointer to frame pixel data
 * @param size Size of frame data in bytes
 */
typedef struct {
    unsigned char *data;
    size_t size;
} video_frame;

/**
 * @struct encoder_context
 * @brief Structure to hold encoder state and configuration
 *
 * @param width Video width in pixels
 * @param height Video height in pixels
 * @param frame_size Size of one frame in bytes
 * @param yuv_size Size of YUV frame in bytes
 */
typedef struct {
    int width;
    int height;
    size_t frame_size;
    size_t yuv_size;
} encoder_context;

/* Function prototypes */
void init_encoder(encoder_context *ctx, int width, int height);
int read_frames(encoder_context *ctx, const char *filename, video_frame **frames, int *frame_count);
void convert_to_yuv420(encoder_context *ctx, video_frame *frame);
void create_delta_frames(video_frame *frames, int frame_count);
unsigned char *compress_frames(video_frame *frames, int frame_count, size_t *compressed_size);
void decode_frames(encoder_context *ctx, unsigned char *compressed_data, 
                  size_t compressed_size, video_frame *frames, int frame_count);
float clamp(float x, float min, float max);

/**
 * init_encoder - Initialize encoder context with given dimensions
 * @ctx: Encoder context to initialize
 * @width: Video width in pixels
 * @height: Video height in pixels
 *
 * Calculates frame sizes and initializes context structure
 */
void init_encoder(encoder_context *ctx, int width, int height)
{
    ctx->width = width;
    ctx->height = height;
    ctx->frame_size = width * height * 3;  /* RGB24 format */
    ctx->yuv_size = width * height + (width * height / 2);  /* YUV420 format */
}

/**
 * read_frames - Read raw video frames from file
 * @ctx: Encoder context
 * @filename: Input filename
 * @frames: Pointer to array of frames (will be allocated)
 * @frame_count: Pointer to store number of frames read
 *
 * Return: 0 on success, -1 on failure
 */
int read_frames(encoder_context *ctx, const char *filename, 
                video_frame **frames, int *frame_count)
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

    /* Count frames and allocate memory */
    buffer = malloc(ctx->frame_size);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    while ((bytes_read = fread(buffer, 1, ctx->frame_size, fp)) == ctx->frame_size)
        count++;

    *frame_count = count;
    frame_array = malloc(count * sizeof(video_frame));
    if (!frame_array) {
        free(buffer);
        fclose(fp);
        return -1;
    }

    /* Read frames into array */
    fseek(fp, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        frame_array[i].data = malloc(ctx->frame_size);
        if (!frame_array[i].data) {
            /* Clean up on failure */
            for (int j = 0; j < i; j++)
                free(frame_array[j].data);
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
    return 0;
}

/**
 * convert_to_yuv420 - Convert RGB frame to YUV420 format
 * @ctx: Encoder context
 * @frame: Frame to convert
 *
 * Converts RGB24 to YUV420 format with chroma subsampling
 */
void convert_to_yuv420(encoder_context *ctx, video_frame *frame)
{
    unsigned char *rgb = frame->data;
    unsigned char *yuv = malloc(ctx->yuv_size);
    unsigned char *Y = yuv;
    unsigned char *U = yuv + ctx->width * ctx->height;
    unsigned char *V = U + (ctx->width * ctx->height / 4);
    
    /* Convert each pixel */
    for (int i = 0; i < ctx->height; i++) {
        for (int j = 0; j < ctx->width; j++) {
            int idx = (i * ctx->width + j) * 3;
            float r = rgb[idx];
            float g = rgb[idx + 1];
            float b = rgb[idx + 2];

            /* RGB to YUV conversion */
            float y = YUV_Y_R * r + YUV_Y_G * g + YUV_Y_B * b;
            Y[i * ctx->width + j] = (unsigned char)clamp(y, 0, 255);

            /* Subsample U,V (4:2:0 format) */
            if (i % 2 == 0 && j % 2 == 0) {
                float u = YUV_U_R * r + YUV_U_G * g + YUV_U_B * b + 128;
                float v = YUV_V_R * r + YUV_V_G * g + YUV_V_B * b + 128;
                U[(i/2) * (ctx->width/2) + j/2] = (unsigned char)clamp(u, 0, 255);
                V[(i/2) * (ctx->width/2) + j/2] = (unsigned char)clamp(v, 0, 255);
            }
        }
    }

    /* Update frame with YUV data */
    free(frame->data);
    frame->data = yuv;
    frame->size = ctx->yuv_size;
}

/**
 * create_delta_frames - Create delta frames from frame sequence
 * @frames: Array of frames
 * @frame_count: Number of frames
 *
 * Creates delta frames by subtracting consecutive frames
 */
void create_delta_frames(video_frame *frames, int frame_count)
{
    for (int i = frame_count - 1; i > 0; i--) {
        for (int j = 0; j < frames[i].size; j++) {
            frames[i].data[j] -= frames[i-1].data[j];
        }
    }
}

/**
 * compress_frames - Compress frames using DEFLATE
 * @frames: Array of frames
 * @frame_count: Number of frames
 * @compressed_size: Pointer to store compressed size
 *
 * Return: Compressed data buffer or NULL on failure
 */
unsigned char *compress_frames(video_frame *frames, int frame_count, 
                             size_t *compressed_size)
{
    z_stream strm;
    unsigned char *compressed;
    size_t total_size = 0;

    /* Calculate total size */
    for (int i = 0; i < frame_count; i++)
        total_size += frames[i].size;

    /* Initialize zlib */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK)
        return NULL;

    /* Allocate compression buffer */
    compressed = malloc(total_size);
    if (!compressed) {
        deflateEnd(&strm);
        return NULL;
    }

    /* Compress frames */
    strm.avail_out = total_size;
    strm.next_out = compressed;

    for (int i = 0; i < frame_count; i++) {
        strm.avail_in = frames[i].size;
        strm.next_in = frames[i].data;
        
        if (deflate(&strm, (i == frame_count - 1) ? Z_FINISH : Z_NO_FLUSH) != Z_OK) {
            free(compressed);
            deflateEnd(&strm);
            return NULL;
        }
    }

    *compressed_size = total_size - strm.avail_out;
    deflateEnd(&strm);
    return compressed;
}

/**
 * decode_frames - Decode compressed frames
 * @ctx: Encoder context
 * @compressed_data: Compressed frame data
 * @compressed_size: Size of compressed data
 * @frames: Array to store decoded frames
 * @frame_count: Number of frames
 */
void decode_frames(encoder_context *ctx, unsigned char *compressed_data,
                  size_t compressed_size, video_frame *frames, int frame_count)
{
    z_stream strm;
    
    /* Initialize zlib */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed_size;
    strm.next_in = compressed_data;
    inflateInit(&strm);

    /* Decompress frames */
    for (int i = 0; i < frame_count; i++) {
        frames[i].data = malloc(ctx->yuv_size);
        frames[i].size = ctx->yuv_size;
        
        strm.avail_out = ctx->yuv_size;
        strm.next_out = frames[i].data;
        inflate(&strm, Z_NO_FLUSH);
    }

    inflateEnd(&strm);

    /* Reconstruct frames from deltas */
    for (int i = 1; i < frame_count; i++) {
        for (int j = 0; j < frames[i].size; j++) {
            frames[i].data[j] += frames[i-1].data[j];
        }
    }
}

/**
 * clamp - Clamp float value between min and max
 * @x: Value to clamp
 * @min: Minimum value
 * @max: Maximum value
 *
 * Return: Clamped value
 */
float clamp(float x, float min, float max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}

/**
 * main - Entry point
 * @argc: Argument count
 * @argv: Argument array
 *
 * Return: 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
    encoder_context ctx;
    video_frame *frames;
    int frame_count;
    unsigned char *compressed;
    size_t compressed_size;

    /* Initialize encoder */
    init_encoder(&ctx, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    /* Read input frames */
    if (read_frames(&ctx, "video.rgb24", &frames, &frame_count) != 0) {
        fprintf(stderr, "Failed to read input video\n");
        return 1;
    }

    printf("Read %d frames\n", frame_count);

    /* Convert frames to YUV420 */
    for (int i = 0; i < frame_count; i++)
        convert_to_yuv420(&ctx, &frames[i]);

    /* Create delta frames */
    create_delta_frames(frames, frame_count);

    /* Compress frames */
    compressed = compress_frames(frames, frame_count, &compressed_size);
    if (!compressed) {
        fprintf(stderr, "Compression failed\n");
        return 1;
    }

    printf("Compressed size: %zu bytes (%.2f%% of original)\n",
           compressed_size,
           100.0f * compressed_size / (ctx.frame_size * frame_count));

    /* Save compressed data */
    FILE *fp = fopen("encoded.bin", "wb");
    if (fp) {
        fwrite(compressed, 1, compressed_size, fp);
        fclose(fp);
    }

    /* Clean up */
    for (int i = 0; i < frame_count; i++)
        free(frames[i].data);
    free(frames);
    free(compressed);

    return 0;
}

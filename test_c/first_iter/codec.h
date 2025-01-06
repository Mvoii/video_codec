#ifndef CODEC_H /*CODEC_H*/
#define CODEC_H

// #include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/* Default video conditions */
#define DEFAULT_WIDTH 384
#define DEFAULT_HEIGHT 216

/* colour conversion constants (ITU-R BT.601) */
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
 * @brief Structure to old frame data and metadata
 *
 * @param data: POinter to next frame pixel data
 * @param size: Size of teh frame data in bytes
 */
typedef struct {
	unsigned char *data;
	size_t size;
} video_frame;

/**
 * @struct encoder_context
 * @brief: Structure to hold encoder state and configs
 *
 * @param width: Vide width in pixels
 * @param height: Video height in pixels
 * @param frame_size: Size of one frame in bytes
 * @param yuv_size: SIze of YUV frame in bytes
 */
typedef struct {
	int width;
	int height;
	size_t frame_size;
	size_t yuv_size;
} encoder_context;

void init_encoder(encoder_context *ctx, int width, int height);
int read_frames(encoder_context *ctx, const char *filename, video_frame **frames, int *frame_count);
void convert_to_yuv420(encoder_context *ctx, video_frame *frame);
void create_delta_frames(video_frame *frames, int frame_count);
unsigned char *compressed_frames(video_frame *frames, int frame_count, size_t *compressed_size);
void decode_frames(encoder_context *ctx, unsigned char *compressed_data, size_t compressed_size, video_frame *frames, int frame_count);
float clamp(float x, float min, float max);

#endif /* CODEC_H */

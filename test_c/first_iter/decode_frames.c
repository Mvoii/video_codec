// decode_frames.c
#include "codec.h"

/**
 * decode_frames - Decode the compressed frames
 * @ctx: Encoder context
 * @compressed_data: Compressed frame data
 * @compressed_size: size of compressed data
 * @frames: Array to store decoded frames
 * @frame_count: Number of frames
 */
void decode_frames(encoder_context *ctx, unsigned char *compressed_data, size_t compressed_size, video_frame *frames, int frame_count)
{
    z_stream strm;

    /* init zlib */
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

    /* reconstruct frames from deltas */
    for (int i = 1; i < frame_count; i++) {
        for (int j = 0; j < frames[i].size; j++)
            frames[i].data[j] += frames[i-1].data[j];
    }
}

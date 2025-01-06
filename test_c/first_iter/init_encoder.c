// init_encoder.c

#include "codec.h"

/**
* init_encoder - Init the encoder context with given dimensions
* @ctx: Encoder context to init
* @width: Video width in pixels
* @height: video height in pixels
*
* Calculates the frame sizes and inits the context struct
*/
void init_encoder(encoder_context *ctx, int width, int height)
{
    ctx->width = width;
    ctx->height = height;
    ctx->frame_size = width * height * 3; // RGB24 format
    ctx->yuv_size = width * height + (width * height / 2);  // YUV420 format
}


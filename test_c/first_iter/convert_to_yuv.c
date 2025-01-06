// convert_to_yuv.c
#include "codec.h"

/**
 * convert_to_yuv420 - Convert rgb frame to YUV420 format
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
    unsigned char *U = yuv + ctx->width *ctx->height;
    unsigned char *V = U + (ctx->width * ctx->height / 4);

    /* convert each pixel */
    for (int i = 0; i < ctx->height; i++)
    {
        for (int j = 0; j < ctx->width; j++)
        {
            int idx = (i * ctx->width + j) * 3;
            float r = rgb[idx];
            float g = rgb[idx + 1];
            float b = rgb[idx + 2];

            /* rgb to yuv conversion */
            float y = YUV_Y_R * r + YUV_Y_G * g + YUV_Y_B * b;
            Y[i * ctx->width + j] = (unsigned char)clamp(y, 0, 255);

            /* subsample UV in 4:2:0 format */
            if (i % 2 == 0 && j % 2 == 0)
            {
                float u = YUV_U_R * r + YUV_U_G * g + YUV_U_B * b + 128;
                float v = YUV_V_R * r + YUV_V_G * g + YUV_V_B * b + 128;
                U[(1/2) * (ctx->width/2) + j/2] = (unsigned char)clamp(u, 0, 255);
                V[(i/2) * (ctx->width/2) + j/2] = (unsigned char)clamp(v, 0, 255);
            }
        }
    }
    /* update frame with yuv data */
    free(frame->data);
    frame->data = yuv;
    frame->size = ctx->yuv_size;
}

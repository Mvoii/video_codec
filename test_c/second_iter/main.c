// incompleted

/**
 * @file video_encoder.c
 * @brief Enhanced video encoder with FFmpeg integration
 *
 * This program implements a video encoder that can handle any video format
 * using FFmpeg for initial decoding. Features include:
 * - Command line argument parsing
 * - Automatic video format detection
 * - RGB to YUV420 conversion
 * - Delta frame encoding
 * - DEFLATE compression
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <unistd.h>
#include <getopt.h>

/* Default video dimensions */
#define DEFAULT_WIDTH 384
#define DEFAULT_HEIGHT 216
#define MAX_CMD_LENGTH 1024

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

/* Previous struct definitions remain the same */
typedef struct {
    unsigned char *data;
    size_t size;
} video_frame;

typedef struct {
    int width;
    int height;
    size_t frame_size;
    size_t yuv_size;
    char input_file[256];
    char output_file[256];
    int target_width;
    int target_height;
    float fps;
} encoder_context;

/* Function prototypes (including new ones) */
void print_usage(const char *program_name);
int parse_arguments(int argc, char *argv[], encoder_context *ctx);
int get_video_info(encoder_context *ctx);
int convert_to_raw(encoder_context *ctx, const char *temp_file);
/* ... (previous function prototypes remain the same) ... */

/**
 * print_usage - Print program usage information
 * @program_name: Name of the program
 */
void print_usage(const char *program_name)
{
    printf("Usage: %s [options] input_file\n", program_name);
    printf("Options:\n");
    printf("  -w, --width WIDTH      Target width (default: %d)\n", DEFAULT_WIDTH);
    printf("  -h, --height HEIGHT    Target height (default: %d)\n", DEFAULT_HEIGHT);
    printf("  -o, --output FILE      Output file (default: encoded.bin)\n");
    printf("  -f, --fps FPS          Target framerate (default: source fps)\n");
    printf("  --help                 Display this help message\n");
}

/**
 * parse_arguments - Parse command line arguments
 * @argc: Argument count
 * @argv: Argument array
 * @ctx: Encoder context to store settings
 *
 * Return: 0 on success, -1 on failure
 */
int parse_arguments(int argc, char *argv[], encoder_context *ctx)
{
    static struct option long_options[] = {
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {"fps", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'H'},
        {0, 0, 0, 0}
    };

    /* Set defaults */
    ctx->target_width = DEFAULT_WIDTH;
    ctx->target_height = DEFAULT_HEIGHT;
    strcpy(ctx->output_file, "encoded.bin");
    ctx->fps = 0;  /* 0 means use source fps */

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "w:h:o:f:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'w':
                ctx->target_width = atoi(optarg);
                break;
            case 'h':
                ctx->target_height = atoi(optarg);
                break;
            case 'o':
                strncpy(ctx->output_file, optarg, sizeof(ctx->output_file) - 1);
                break;
            case 'f':
                ctx->fps = atof(optarg);
                break;
            case 'H':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    /* Get input file */
    if (optind >= argc) {
        fprintf(stderr, "Error: Input file required\n");
        print_usage(argv[0]);
        return -1;
    }

    strncpy(ctx->input_file, argv[optind], sizeof(ctx->input_file) - 1);
    return 0;
}

/**
 * get_video_info - Get video information using FFmpeg
 * @ctx: Encoder context
 *
 * Return: 0 on success, -1 on failure
 */
int get_video_info(encoder_context *ctx)
{
    char cmd[MAX_CMD_LENGTH];
    FILE *pipe;
    char line[256];

    /* Create FFprobe command to get video information */
    snprintf(cmd, sizeof(cmd),
             "ffprobe -v error -select_streams v:0 -show_entries "
             "stream=width,height,r_frame_rate -of default=noprint_wrappers=1 "
             "\"%s\" 2>&1", ctx->input_file);

    pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "Error executing FFprobe\n");
        return -1;
    }

    /* Parse FFprobe output */
    while (fgets(line, sizeof(line), pipe)) {
        if (strncmp(line, "width=", 6) == 0)
            ctx->width = atoi(line + 6);
        else if (strncmp(line, "height=", 7) == 0)
            ctx->height = atoi(line + 7);
        else if (strncmp(line, "r_frame_rate=", 13) == 0) {
            int num, den;
            if (sscanf(line + 13, "%d/%d", &num, &den) == 2 && den != 0)
                ctx->fps = ctx->fps == 0 ? (float)num / den : ctx->fps;
        }
    }

    pclose(pipe);

    /* Check if we got valid information */
    if (ctx->width == 0 || ctx->height == 0) {
        fprintf(stderr, "Error: Could not determine video dimensions\n");
        return -1;
    }

    return 0;
}

/**
 * convert_to_raw - Convert input video to raw RGB24 format
 * @ctx: Encoder context
 * @temp_file: Temporary file to store raw video
 *
 * Return: 0 on success, -1 on failure
 */
int convert_to_raw(encoder_context *ctx, const char *temp_file)
{
    char cmd[MAX_CMD_LENGTH];
    
    /* Create FFmpeg command for video conversion */
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -i \"%s\" -v error "
             "-vf scale=%d:%d "
             "-r %.2f "
             "-f rawvideo -pix_fmt rgb24 "
             "\"%s\" ",
             ctx->input_file,
             ctx->target_width,
             ctx->target_height,
             ctx->fps,
             temp_file);

    printf("Converting video...\n");
    if (system(cmd) != 0) {
        fprintf(stderr, "Error converting video\n");
        return -1;
    }

    return 0;
}

/**
 * main - Entry point with enhanced argument handling
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
    const char *temp_file = "temp_raw_video.rgb24";

    /* Parse command line arguments */
    if (parse_arguments(argc, argv, &ctx) != 0)
        return 1;

    /* Get video information */
    if (get_video_info(&ctx) != 0)
        return 1;

    printf("Input video: %s\n", ctx->input_file);
    printf("Original dimensions: %dx%d\n", ctx->width, ctx->height);
    printf("Target dimensions: %dx%d\n", ctx->target_width, ctx->target_height);
    printf("Target FPS: %.2f\n", ctx->fps);

    /* Convert input video to raw format */
    if (convert_to_raw(&ctx, temp_file) != 0)
        return 1;

    /* Initialize encoder with target dimensions */
    init_encoder(&ctx, ctx->target_width, ctx->target_height);

    /* Read converted raw video */
    if (read_frames(&ctx, temp_file, &frames, &frame_count) != 0) {
        fprintf(stderr, "Failed to read converted video\n");
        unlink(temp_file);  /* Clean up temp file */
        return 1;
    }

    /* Remove temporary raw video file */
    unlink(temp_file);

    printf("Processing %d frames\n", frame_count);

    /* Convert frames to YUV420 */
    for (int i = 0; i < frame_count; i++) {
        printf("\rConverting to YUV: %d%%", (i + 1) * 100 / frame_count);
        fflush(stdout);
        convert_to_yuv420(&ctx, &frames[i]);
    }
    printf("\n");

    /* Create delta frames */
    create_delta_frames(frames, frame_count);

    /* Compress frames */
    compressed = compress_frames(frames, frame_count, &compressed_size);
    if (!compressed) {
        fprintf(stderr, "Compression failed\n");
        return 1;
    }

    printf("Compression results:\n");
    printf("Original size: %zu bytes\n", ctx.frame_size * frame_count);
    printf("Compressed size: %zu bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", 
           100.0f * compressed_size / (ctx.frame_size * frame_count));

    /* Save compressed data */
    FILE *fp = fopen(ctx.output_file, "wb");
    if (fp) {
        fwrite(&ctx.target_width, sizeof(int), 1, fp);  /* Save metadata */
        fwrite(&ctx.target_height, sizeof(int), 1, fp);
        fwrite(&frame_count, sizeof(int), 1, fp);
        fwrite(&ctx.fps, sizeof(float), 1, fp);
        fwrite(compressed, 1, compressed_size, fp);
        fclose(fp);
        printf("Encoded video saved to: %s\n", ctx.output_file);
    }

    /* Clean up */
    for (int i = 0; i < frame_count; i++)
        free(frames[i].data);
    free(frames);
    free(compressed);

    return 0;
}

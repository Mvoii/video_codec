// compress_frames.c
#include "codec.h"

/**
* compress_frames - Compress frames using DEFLATE
* @frames: Array of frames
* @frame_count: Number of frames
* @compressed_size: Pointer to store compressed size
*
* Return: Compressed data buffer or NULL on failure
*/
unsigned char *compressed_frames(video_frame *frames, int frame_count, size_t *compressed_size)
{
   z_stream strm;
   unsigned char *compressed;
   size_t total_size = 0;

   /* Calculate total size */
   for (int i = 0; i < frame_count; i++)
        total_size += frames[i].size;

   printf("\ntotal size: %ld\n", total_size);

   /* init zlib */
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK)
      return NULL;

   /* Allocate copression buffer */
   compressed = malloc(total_size);
   if (!compressed) {
      deflateEnd(&strm);

      return NULL;
   }

   printf("malloced compressed size...\n");

   /* compress frames */
   strm.avail_out = total_size;
   strm.next_out = compressed;

   printf("\ndeflating stream..\n");
   for (int i = 0; i < frame_count; i++) {
      strm.avail_in = frames[i].size;
      strm.next_in = frames[i].data;

      // printf("\n deflating stream\n");

      if (deflate(&strm, (i == frame_count) ? Z_FINISH : Z_NO_FLUSH) != Z_OK) {
         free(compressed);
         deflateEnd(&strm);
         printf("failed deflate on %d frame\n", i);

         return NULL;
      }
   }

   *compressed_size = total_size - strm.avail_out;
   printf("\ncompressed size: %ld\n", *compressed_size);
   deflateEnd(&strm);

   return compressed;
}

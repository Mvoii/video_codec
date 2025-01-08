## Codec translated into python
 <p>the codec compresses a video in a encoded.bin file andn then decodes it inro teh decoded.rgb24 file.</p>

Here are the key changes and improvements

1. Used NumPy arrays instead of raw bytes for better performance and cleaner code.
2. Structured code into class for better organization.
3. Added more comprehensive documentation and comments.

The main differences from the go version are:

1. Memory management is handled by NumPy instead of maula slice manipulation
2. Used NumPy vectorise operations for better performance
3. Used RGB to YUV conversion using matrix operations
4.  Used Pyton zlib in place of the Go's Deflate.
5. Added more robust error handling and logging.


For this encoder you need:

1. Python 3.7+
2. NumPy
3. zlib
4. ffmpeg


```
# convert the video to rgb24 
format:
ffmpeg -i input.mp4 -f rawvideo -pix_fmt rgb24 video.rgb24

# Run the encoder
python video_encoder.py

# play the decoded video
ffplay -f rawvideo -pixel_format rgb24 -video_size 384x216 -framerate 25 decoded.rgb24
```
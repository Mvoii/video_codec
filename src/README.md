## Codec in python
 <p>The codec compresses a video in a encoded.bin file andn then decodes it into teh decoded.rgb24 file.</p>

Here are the key features:

1. Used NumPy arrays instead of raw bytes for better performance and cleaner code.
2. Structured code into class for better organization.
3. Added more comprehensive documentation and comments.


The main differences from the go version are:

1. Memory management is handled by NumPy
2. Used NumPy vectorise operations for better performance
3. Used RGB to YUV conversion using matrix operations
4. Used Python zlib in place of the Deflate.
5. Added more robust error handling and logging.


For this encoder you need:

1. Python 3.7+
2. NumPy
3. zlib
4. ffmpeg




<h3>convert the video to rgb24 </h3>

---

format:
  ``ffmpeg -i input.mp4 -f rawvideo -pix_fmt rgb24 video.rgb24``

<h3>Run the encoder</h3>

---

``python3 src/video_encoder.py``


<h3>play the decoded video</h3>

---

``ffplay -f rawvideo -pixel_format rgb24 -video_size 384x216 -framerate 25 src/decoded.rgb24
``

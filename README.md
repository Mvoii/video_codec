# A rudimentary video codec

<a>https://video-codec-v0.onrender.com/</a>

<h3>Constraints</h3>

1. The video size should end up smaller after the encoding
2. The encoding is reversible
3. The compressed video should contain enough information to be recongisable
---

<h3>technique</h3>

---

<p>
Consecutive frames have spatial redundancy, pixels in consecutive frames are likely to have similar information.
This allows us to compress like how png does.</p>

<p>Each frame contains pixels -which contain three attribuutes: r, g, and b. This gives a total size of (width * height * 3) per frame.</p>

<p>We convert each frame to YUV420 format. Then for each 4 adjacent pixels, we average the chominance this reducing information stored by each pixels to 0.25</p>

---
<h3>on the webpage</h3>

---

<a>https://video-codec-v0.onrender.com/</a>

1. Use the `video.rgb24` file available in this repository
2. upload the video, add its width `384` and height `216`
3. the video should be compressed and downloaded to your computer
4. upload the compressed video, width `384`, height `216` and number of frames `217`
5. the video should be decompressed and downloaded o your computer
---

<h3>play the videos using ffmpeg</h3>

---

type the command onto the terminal:
    ``ffplay -f rawvideo -pixel_format rgb24 -video_size 384x216 -framerate 25 <filename>.rgb24
    ``


references 
--
1. <a> https://github.com/kevmo314/codec-from-scratch </a>
2. ffmpeg blogs

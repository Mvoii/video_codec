### A rudimentary video codec
---
<h3>Constraints</h3>

1. The video size should end up smaller after the encoding
2. The encoding is reversible
3. The compressed video should contain enogh information to e recongisable

<h3>technique</h3>
<p>
Consecutive frames have spatial redundancy, pixels in consecutive frames are likely to have similar information.
This allows us to compress like how png does.</p>

<p>Each frame contains pixels -which contain three attribuutes: r, g, and b. This gives a total size of (width * height * 3) per frame.</p>

<p>We convert each frame to YUV420 format. Then for each 4 adjacent pixels, we average the chominance this reducing information stored by each pixels to 0.25</p>
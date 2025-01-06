import argparse
import logging
from typing import List, Tuple
import zlib
import numpy as np
import io


class VideoEncoder:
    """A basic video encoder that demonstrates fundamental concepys of video compression.
    The encder uses teh following techniques:
    1. RGB to YUV420 conversion
    2. Chroma subsampling
    3. Delta compression
    4. DEFLATE compression
    
    While a real world video encoders are more complex, this is much easier to acheive 90% compression
    """
    
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.frame_size = width * height * 3
        
    def read_frames(self, input_data: bytes) -> List[np.ndarray]:
        """Read reaw video frames from input bytes in RGB24 format
           Each pixel is represented by 3 bytes

        Args:
            input_data (bytes): Raw video dara in RGB24 format

        Returns:
            List[np.ndarray]: List of numpy arrays containning frame data
        """
        frames = []
        data = io.BytesIO(input_data)
        
        while True:
            frame_data = data.read(self.frame_size)
            if not frame_data or len(frame_data) < self.frame_size:
                break
            
            # reshape frame data into height x width x 3 array
            frame = np.frombuffer(frame_data ,dtype=np.uint8)
            frame = frame.reshape((self.height, self.width, 3))
            frames.append(frame)
            
        return frames
    
    def rgb_to_yuv420(self, frame: np.ndarray) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """convert RGB frame to YUV420 format with chroma subsampling
        
           YUV420 separates brightness (Y) from color information (U,V) and takes advantage of human eye's lower sensitivity to color detail by subsampling the U,V components.

        Args:
            frame (np.ndarray): RBG frame data (height x width x 3)

        Returns:
            Tuple[np.ndarray, np.ndarray, np.ndarray]: Y U V components where U V are subsampled
        """
        frame = frame.astype(np.float32)
        
        #rgb to yuv conversion matrix
        Y = 0.299 * frame[:,:,0] + 0.587 * frame[:,:,1] + 0.114 * frame[:,:,2]
        U = -0.169 * frame[:,:,0] - 0.331 * frame[:,:,1] + 0.449 * frame[:,:,2] + 128
        V = 0.499 * frame[:,:,0] - 0.418 * frame[:,:,1] - 0.0813 * frame[:,:,2] + 128
        
        # convert back to uint8
        Y = Y.astype(np.uint8)
        
        # subsample UV (4:2:0 format)
        # take avg of 2x2 blocks
        U = U[::2, ::2].astype(np.uint8)  # Subsample by taking every 2nd pixel
        V = V[::2, ::2].astype(np.uint8)
        
        return Y, U, V
    
    def encode_frames(self, frames: List[np.ndarray]) -> bytes:
        """Encode video frames using YUV420 conversion, delta encoding, and deflate compression
        
        The encoding process:
        1 Convert each frame to YUV420
        2. Convert delta frames (except for keyframes)
        3. Apply DEFLATE compression

        Args:
            frames (List[np.ndarray]): List of RGB frames

        Returns:
            bytes: Compressed video data
        """
        logging.info(f"Raw size: {sum(frame.size * frame.itemsize for frame in frames)} bytes")
        
        # convert frames to YUV420
        yuv_frames = []
        for frame in frames:
            Y, U, V = self.rgb_to_yuv420(frame)
            # store in a planar format (all Y, then all U, then all V)
            yuv_frame = np.concatenate([Y.flatten(), U.flatten(), V.flatten()])
            yuv_frames.append(yuv_frame)

        yuv_size = sum(frame.size *frame.itemsize for frame in yuv_frames)
        logging.info(f"YUV420 size: {yuv_size} bytes({100 * yuv_size / (self.frame_size * len(frames)):.2f}% original siize)")
        
        # create delta frames
        delta_frames = []
        for i, frame in enumerate(yuv_frames):
            if i == 0:
                # first key frame
                delta_frames.append(frame)
            else:
                # delta frame of previous frame
                delta = frame - yuv_frames[i-1]
                delta_frames.append(delta)
            
            # compress using DEFLATE
            compressed = zlib.compress(np.concatenate(delta_frames).tobytes(), level=9)
            logging.info(f"DEFLATE size: {len(compressed)} bytes ({100 * len(compressed) / (self.frame_size * len(frames)):.2f}% original size)")
            
            return compressed
        
    def decode_frames(self, compressed_data: bytes, num_frames: int) -> List[np.ndarray]:
        """Decode compressed video back to RGB frames
    
        The decoding process:
        1. Decompress DEFLATE data
        2. Reconstruct frames from deltas
        3. Convert YUV420 back to RGB

        Args:
            compressed_data (bytes): Compressed video data
            num_frames (int): NUmber of frames ro decode

        Returns:
            List[np.ndarray]: List of RGB frames
        """
        # decompress DEFLATE
        decompressed = np.frombuffer(zlib.decompress(compressed_data), dtype=np.uint8)
        
        # calculate size of each yuv frame
        yuv_frame_size = (self.width * self.height) + (self.width * self.height // 2)
        
        # split into frames
        frames = []
        for i in range(num_frames):
            start = i * yuv_frame_size
            end = start + yuv_frame_size
            frames.append(decompressed[start:end])
            
        # reconstruct from deltas
        for i in range(1, len(frames)):
            frames[i] = frames[i] + frames[i-1]
        
        # convert back to rgb
        rgb_frames = []
        for frame in frames:
            # split into Y,U,V components
            Y = frame[:self.width * self.height].reshape(self.height, self.width)
            U = frame[self.width * self.height:self.width * self.height + self.width * self.height // 4].reshape(self.height // 2, self.width // 2)
            V = frame[self.width * self.height + self.width * self.height // 4:].reshape(self.height // 2, self.width // 2)
            
            # upsampled UV
            U = np.repeat(np.repeat(U, 2, axis=0), 2, axis=1)
            V = np.repeat(np.repeat(V, 2, axis=0), 2, axis=1)
            
            # convert to float for calculations
            Y = Y.astype(np.float32)
            U = U.astype(np.float32) - 128
            V = V.astype(np.float32) - 128
            
            # yuv to rgb conversion
            R = np.clip(Y + 1.402 * V, 0, 255).astype(np.uint8)
            G = np.clip(Y - 0.344 * U - 0.714 * V, 0, 255).astype(np.uint8)
            B = np.clip(Y + 1.772 * U, 0, 255).astype(np.uint8)
            
            # stack rgb channels
            rgb_frame = np.stack([R, G, B], axis=2)
            rgb_frames.append(rgb_frame)
        
        return rgb_frames
        

def main():
    """Main function to demo the codec"""
    parser = argparse.ArgumentParser(description="Basic video codec demo")
    parser.add_argument("--width", type=int, default=384, help="Width of the video")
    parser.add_argument("--height", type=int, default=216, help="Height of the video")
    args = parser.parse_args()
    
    # config the logging
    logging.basicConfig(level=logging.INFO)
    
    # init encoder
    encoder = VideoEncoder(args.width, args.height)
    
    # read input video
    logging.info("Readinng input video...")
    with open('video.rgb24', "rb") as f:
        input_data = f.read()
        
    # read and encode frames
    frames = encoder.read_frames(input_data)
    compressed = encoder.encode_frames(frames)
    
    # save compressed data
    with open('encoded.bin', 'wb') as f:
        f.write(compressed)
    
    # decode and save
    decoded_frames = encoder.decode_frames(compressed, len(frames))
    
    # save decoded video
    with open("decoded.rgb24", "wb") as f:
        for frame in decoded_frames:
            f.write(frame.tobytes())
        
    logging.info("Encoding/decoding complete!")


if __name__ == "__main__":
    main()

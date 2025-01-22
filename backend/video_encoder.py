import argparse
import io
import logging
import numpy as np
import zlib
from typing import List, Tuple

class VideoEncoder:
    """
    A basic video encoder that demonstrates fundamental concepts of video compression.
    The encoder uses the following techniques:
    1. RGB to YUV420 color space conversion
    2. Chroma subsampling
    3. Delta frame encoding
    4. DEFLATE compression
    
    While real-world video encoders are much more sophisticated, this implementation
    shows how to achieve ~90% compression with simple techniques.
    """
    
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.frame_size = width * height * 3  # RGB24 format: 3 bytes per pixel
        
    def read_frames(self, input_data: bytes) -> List[np.ndarray]:
        """
        Read raw video frames from input bytes in RGB24 format.
        Each pixel is represented by 3 bytes (R,G,B).
        
        Args:
            input_data: Raw video data in RGB24 format
            
        Returns:
            List of numpy arrays containing frame data
        """
        frames = []
        data = io.BytesIO(input_data)
        
        while True:
            frame_data = data.read(self.frame_size)
            if not frame_data or len(frame_data) < self.frame_size:
                break
                
            # Reshape frame data into height × width × 3 array
            frame = np.frombuffer(frame_data, dtype=np.uint8)
            frame = frame.reshape((self.height, self.width, 3))
            frames.append(frame)
            
        print(f"frame count: {len(frames)}")
            
        return frames

    def rgb_to_yuv420(self, frame: np.ndarray) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """
        Convert RGB frame to YUV420 format with chroma subsampling.
        
        YUV420 separates brightness (Y) from color information (U,V) and takes advantage
        of human eye's lower sensitivity to color detail by subsampling the U,V components.
        
        Args:
            frame: RGB frame data (height × width × 3)
            
        Returns:
            Tuple of (Y, U, V) components where U,V are subsampled
        """
        # Convert to float for calculations
        frame = frame.astype(np.float32)
        
        # RGB to YUV conversion matrix (ITU-R BT.601 standard)
        Y = 0.299 * frame[:,:,0] + 0.587 * frame[:,:,1] + 0.114 * frame[:,:,2]
        U = -0.169 * frame[:,:,0] - 0.331 * frame[:,:,1] + 0.449 * frame[:,:,2] + 128
        V = 0.499 * frame[:,:,0] - 0.418 * frame[:,:,1] - 0.0813 * frame[:,:,2] + 128
        
        # Convert back to uint8
        Y = Y.astype(np.uint8)
        
        # Subsample U and V components (4:2:0 format)
        # Take average of 2x2 blocks
        U = U[::2, ::2].astype(np.uint8)  # Subsample by taking every 2nd pixel
        V = V[::2, ::2].astype(np.uint8)
        
        return Y, U, V

    def encode_frames(self, frames: List[np.ndarray]) -> bytes:
        """
        Encode video frames using YUV420 conversion, delta encoding, and DEFLATE compression.
        
        The encoding process:
        1. Convert each frame to YUV420
        2. Create delta frames (except for keyframe)
        3. Apply DEFLATE compression
        
        Args:
            frames: List of RGB frames
            
        Returns:
            Compressed video data
        """
        logging.info(f"Raw size: {sum(frame.size * frame.itemsize for frame in frames)} bytes")
        
        # Convert frames to YUV420
        yuv_frames = []
        for frame in frames:
            Y, U, V = self.rgb_to_yuv420(frame)
            # Store in planar format (all Y, then all U, then all V)
            yuv_frame = np.concatenate([Y.flatten(), U.flatten(), V.flatten()])
            yuv_frames.append(yuv_frame)
            
        yuv_size = sum(frame.size * frame.itemsize for frame in yuv_frames)
        logging.info(f"YUV420P size: {yuv_size} bytes ({100 * yuv_size / (self.frame_size * len(frames)):.2f}% original size)")
        
        # Create delta frames
        delta_frames = []
        for i, frame in enumerate(yuv_frames):
            if i == 0:
                # First frame is keyframe
                delta_frames.append(frame)
            else:
                # Delta from previous frame
                delta = frame - yuv_frames[i-1]
                delta_frames.append(delta)
        
        # Compress using DEFLATE
        compressed = zlib.compress(np.concatenate(delta_frames).tobytes(), level=9)
        logging.info(f"DEFLATE size: {len(compressed)} bytes ({100 * len(compressed) / (self.frame_size * len(frames)):.2f}% original size)")
        
        return compressed

    def decode_frames(self, compressed_data: bytes, num_frames: int) -> List[np.ndarray]:
        """
        Decode compressed video data back to RGB frames.
        
        The decoding process:
        1. Decompress DEFLATE data
        2. Reconstruct frames from deltas
        3. Convert YUV420 back to RGB
        
        Args:
            compressed_data: Compressed video data
            num_frames: Number of frames to decode
            
        Returns:
            List of RGB frames
        """
        # Decompress DEFLATE
        decompressed = np.frombuffer(zlib.decompress(compressed_data), dtype=np.uint8)
        
        # Calculate size of each YUV frame
        yuv_frame_size = (self.width * self.height) + (self.width * self.height // 2)
        
        # Split into frames
        frames = []
        for i in range(num_frames):
            start = i * yuv_frame_size
            end = start + yuv_frame_size
            frames.append(decompressed[start:end])
        
        # Reconstruct from deltas
        for i in range(1, len(frames)):
            frames[i] = frames[i] + frames[i-1]
        
        # Convert back to RGB
        rgb_frames = []
        for frame in frames:
            # Split into Y, U, V components
            Y = frame[:self.width * self.height].reshape(self.height, self.width)
            U = frame[self.width * self.height:self.width * self.height + self.width * self.height // 4].reshape(self.height // 2, self.width // 2)
            V = frame[self.width * self.height + self.width * self.height // 4:].reshape(self.height // 2, self.width // 2)
            
            # Upsample U and V
            U = np.repeat(np.repeat(U, 2, axis=0), 2, axis=1)
            V = np.repeat(np.repeat(V, 2, axis=0), 2, axis=1)
            
            # Convert to float for calculations
            Y = Y.astype(np.float32)
            U = U.astype(np.float32) - 128
            V = V.astype(np.float32) - 128
            
            # YUV to RGB conversion
            R = np.clip(Y + 1.402 * V, 0, 255).astype(np.uint8)
            G = np.clip(Y - 0.344 * U - 0.714 * V, 0, 255).astype(np.uint8)
            B = np.clip(Y + 1.772 * U, 0, 255).astype(np.uint8)
            
            # Stack RGB channels
            rgb_frame = np.stack([R, G, B], axis=2)
            rgb_frames.append(rgb_frame)
            
        return rgb_frames

def main():
    """Main function to demonstrate the video encoder/decoder."""
    parser = argparse.ArgumentParser(description='Basic video encoder demonstration')
    parser.add_argument('--width', type=int, default=384, help='Width of the video')
    parser.add_argument('--height', type=int, default=216, help='Height of the video')
    args = parser.parse_args()

    # Configure logging
    logging.basicConfig(level=logging.INFO)
    
    # Initialize encoder
    encoder = VideoEncoder(args.width, args.height)
    
    # Read input video
    logging.info("Reading input video...")
    with open('src/video.rgb24', 'rb') as f:
        input_data = f.read()
    
    # Read and encode frames
    frames = encoder.read_frames(input_data)
    compressed = encoder.encode_frames(frames)
    
    # Save compressed data
    with open('src/encoded.bin', 'wb') as f:
        f.write(compressed)
    
    # Decode and save
    decoded_frames = encoder.decode_frames(compressed, len(frames))
    
    # Save decoded video
    with open('src/decoded.rgb24', 'wb') as f:
        for frame in decoded_frames:
            f.write(frame.tobytes())
    
    logging.info("Encoding/decoding complete!")

""" if __name__ == "__main__":
    main()
 """
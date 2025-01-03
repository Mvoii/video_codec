package main

import (
	"bytes"
	"compress/flate"
	"flag"
	"io"
	"log"
	"os"

	//"golang.org/x/tools/go/analysis/passes/defers"
)

// builds a basic video encoder
// take advantage of human eye's insensitivity to small
// changes in colours.
//
// run with:
//	cat video.rgb24 | go run main.go

func main() {
	var width, height int
	flag.IntVar(&width, "width", 384, "width of the video")
	flag.IntVar(&height, "height", 216, "height of the video")
	flag.Parse()

	frames := make([][]byte, 0)

	for {
		// read raw of video frames from stdin.
		// in rgb24 format, each pixel (r, g, b) is one byte
		// so the total size of a frame is width * height * 3

		frame := make([]byte, width * height * 3)

		// read the frame from stdin
		if _, err := io.ReadFull(os.Stdin, frame); err != nil {
			break
		}

		frames = append(frames, frame)
	}

	// Now we have our new video, using some memory
	rawSize := size(frames)
	log.Printf("Raw size %d bytes", rawSize)

	for i, frame := range frames {
		// convert each frame to YUV420 format.
		// each px in rgb24 format is a vector containg (r, g, b)
		//
		// the YU420; each px has 3 vectors [Y(0, 0), U(0, 0), V(0, 0)]
		// Y - lumminance
		// UV - chrominance
		//
		// we can reduce the chromoinance since human eyes are less sensitive to colour,

		Y := make([]byte, width*height)
		U := make([]float64, width*height)
		V := make([]float64, width*height)
		for j := 0; j < width*height; j++ {
			// convert px to yuv
			r, g, b := float64(frame[3*j]), float64(frame[3*j+1]), float64(frame[3*j+2])

			// These coefficients are from the ITU-R standard,
			// wikipage https://en.wikipedia.org/wiki/YUV#Y%E2%80%B2UV4444_to_RGB888_conversion
			//
			y := +0.299*r + 0.587*g + 0.114*b
			u := -0.169*r - 0.331*g + 0.449*b + 128
			v := 0.499*r - 0.418*g - 0.0813*b + 128

			// store the YUV values in our byte slices, These are separated for next step to be easy
			Y[j] = uint8(y)
			U[j] = u
			V[j] = v
		}

		// we downsample the UV components
		// we take the 4 px that share a UV component and average them together
		//
		// store the downsample UV in these slices
		uDownsampled := make([]byte, width*height/4)
		vDownsampled := make([]byte, width*height/4)
		for x := 0; x < height; x += 2 {
			for y := 0; y < width; y += 2 {
				// we avg the U and V components of the 4 px that share the components
				u := (U[x*width+y] + U[x*width+y+1] + U[(x+1)*width+y] + U[(x+1)*width+y+1]) / 4
				v := (V[x*width+y] + V[x*width+y+1] + V[(x+1)*width+y] + V[(x+1)*width+y+1]) / 4

				// store the downsampled U and V components in our bytes slices
				uDownsampled[x/2*width/2+y/2] = uint8(u)
				vDownsampled[x/2*width/2+y/2] = uint8(v)
			}
		}

		yuvFrame := make([]byte, len(Y)+len(uDownsampled)+len(vDownsampled))

		// we need to store the YUV values in a byte slice, to make the data
		// more compressible, we will store all the Y vales first, the all the U values
		// then all the V values, also called planar format
		//
		// the intuition is that ajacent YUV valies are more likely to be similar than  YUV themselves
		// thus, storing the components in a planar format saves data later

		copy(yuvFrame, Y)
		copy(yuvFrame[len(Y):], uDownsampled)
		copy(yuvFrame[len(Y) + len(uDownsampled):], vDownsampled)

		frames[i] = yuvFrame
	}

	// now we have our yuv-encoded video, taking half the space

	yuvSize := size(frames)
	log.Printf("YUV420P size: %d bytes(%0.2f%% original size)", yuvSize, 100*float32(yuvSize)/float32(rawSize))

	// we can also write this out to a file, which can be played with ffplay:
	//
	// ffplay -f rawvideo -pixel_format yuv420p -video_size 384x216 -framerate 25 encoded.yuv

	if err := os.WriteFile("encoded.yuv", bytes.Join(frames, nil), 0644); err != nil {
		log.Fatal(err)
	}

	encoded := make([][]byte, len(frames))
	for i := range frames {
		// we simplify the daata by computing the delta between each frame
		// observe that in many cases, pixels between frames don't change much
		// therefore many of the deltas will be small
		// we can store the small deltas more efficiently
		//
		// of course, the first frame doesn't have a previous frame so we will store the entire thing
		// This is called a keyframe, in the real world , keyframes are computed periodically
		// and demarcaed in the metadata
		// Keyframes can also be compressed, but we will deal with it later
		// TODO: compress the Keyframes
		// In our encoder, we wil make the frame 0 the keyframe
		//
		// The rest of the frames will delta from the previous frame. this are called
		// predicted frames, P-frames

		if i == 0 {
			// this is the keyframe, store the rawframe
			encoded[i] = frames[i]
			continue
		}

		delta := make([]byte, len(frames[i]))
		for j := 0; j < len(delta); j++ {
			delta[j] = frames[i][j] - frames[i-1][j]
		}

		// now we have our delta frame, which if we print out contains a bunc of zeroes
		// this is a simple algo where we store the number of times a value repeats, then teh value
		//
		// Example: the sequence 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0
		// would be stored as 4, 0, 6, 1, 4, 0
		//
		// Run length encoding; its name

		var rle []byte
		for j := 0; j < len(delta); {
			// count number of times the current values repeat
			var count byte
			for count = 0; count < 255 && j+int(count) < len(delta) && delta[j+int(count)] == delta[j]; count++ {
			}

			// store the count and value
			rle = append(rle, count)
			rle = append(rle, delta[j])

			j += int(count)
		}

		// save the rle frame
		encoded[i] = rle
	}

	rleSize := size(encoded)
	log.Printf("RLE size: %d bytes (%0.f%% original size)", rleSize, 100*float32(rleSize)/float32(rawSize))

	// This is good, we are at 0.25 of the original size, But we could do better.
	//
	// note that the longest runs are runs of zeroes, this is because our delta
	// between frames is really small, we have a little bit of flexibility in
	// choosing an algo here, so we keep the encoder simple
	// we will defer to usin DEFLATE algo which is available in the standard library.
	// The implememntation is beyond the scope of this project

	var deflated bytes.Buffer
	w, err := flate.NewWriter(&deflated, flate.BestCompression)
	if err != nil {
		log.Fatal(err)
	}
	for i :=range frames {
		if i == 0 {
			// this is teh keyframe, write the raw frame
			if _, err := w.Write(frames[i]); err != nil {
				log.Fatal(err)
			}
			continue
		}

		delta := make([]byte, len(frames[i]))
		for j := 0; j < len(delta); j++ {
			delta[j] = frames[i][j] - frames[i-1][j]
		}
		if _, err := w.Write(delta); err != nil {
			log.Fatal(err)
		}
	}
	if err := w.Close(); err != nil {
		log.Fatal(err)
	}

	deflatedSize := deflated.Len()
	log.Printf("DEFLATED size: %d bytes (%0.2f%% original size)", deflatedSize, 100*float32(deflatedSize)/float32(rawSize))

	// note the deflate step takes long. Genenrally encoders are slower than decoders
	//
	// at this point we have achieved 90% compression, JPEG levles compressions
	// we have used temporal locality to compress the data
	//
	// now lets decode our encoded data

	// first: deflate the stream
	var inflated bytes.Buffer
	r := flate.NewReader(&deflated)
	if _, err := io.Copy(&inflated, r); err != nil {
		log.Fatal(err)
	}
	if err := r.Close(); err != nil {
		log.Fatal(err)
	}

	// split the inflated streams into frames
	decodedFrames := make([][]byte, 0)
	for {
		frame := make([]byte, width*height*3/2)
		if _, err := io.ReadFull(&inflated, frame); err != nil {
			if err == io.EOF {
				break
			}
			log.Fatal(err)
		}
		decodedFrames = append(decodedFrames, frame)
	}

	// for every frame except the first one, we need to add teh previous frame to the delta frame.
	// this is the opposite of what we did in the encoder
	for i := range decodedFrames {
		if i == 0 {
			continue
		}

		for j := 0; j < len(decodedFrames[i]); j++ {
			decodedFrames[i][j] += decodedFrames[i-1][j]
		}
	}

	if err := os.WriteFile("decoded.yuv", bytes.Join(decodedFrames, nil), 0644); err != nil {
		log.Fatal(err)
	}

	// then convert each yuv frame into rgb
	for i, frame := range decodedFrames {
		Y := frame[:width*height]
		U := frame[width* height : width*height+width*height/4]
		V := frame[width*height+width+height/4:]

		rgb := make([]byte, 0, width*height*3)
		for j := 0; j < height; j++ {
			for k := 0; k < width; k++ {
				y := float64(Y[j*width+k])
				u := float64(U[(j/2)*(width/2)+(k/2)]) - 128
				v := float64(V[(j/2)*(width/2)+(k/2)]) - 128

				r := clamp(y+1.402*v, 0, 255)
				g := clamp(y-0.344*u-0.714*v, 0, 255)
				b := clamp(y+1.772*u, 0, 255)

				rgb = append(rgb, uint8(r), uint8(g), uint8(b))
			}
		}
		decodedFrames[i] = rgb
	}

	// write the decoded video to a file
	//
	// the video can be played with ffplay:
	//	ffplay -f rawvideo -pixel_format rgb24 -video_size 384x216 -framerate 25 decoded.rgb24
	
	out, err := os.Create("decoded.rgb24")
	if err != nil {
		log.Fatal(err)
	}
	defer out.Close()

	for i := range decodedFrames {
		if _, err := out.Write(decodedFrames[i]); err != nil {
			log.Fatal(err)
		}
	}
}

func size(frames [][]byte) int {
	var size int
	for _, frame := range frames {
		size += len(frame)
	}
	return size
}

func clamp(x, min, max float64) float64 {
	if x < min {
		return min
	}

	if x > max {
		return max
	}
	return x
}

# H.264 Optical Flow [![Build Status](https://travis-ci.org/Kuree/h264flow.svg?branch=master)](https://travis-ci.org/Kuree/h264flow)

## Build
```
mkdir -p build && cd build
cmake .. && make
```

### Optional Visualizer
`example/visualize` uses OpenCV to visualize the motion vectors. If you have OpenCV installed, `cmake` will automatically configure the `Makefile`.

## Prepare your videos
Currently the decoder only works for H264 baseline profile without any partition or reference frames. You can use `x264` to prepare your video:
```
x264 <input> --profile baseline --preset ultrafast --ref 0 -o <output>
```
Notice that `--min-keyint <number>` can used here to make sure there are enough P-frames/P-slices to extract motion vectors without hitting I-frames/slices too quickly. `x264` is smart enough to figure out most of the scenecuts and insert an I-frames/slices.

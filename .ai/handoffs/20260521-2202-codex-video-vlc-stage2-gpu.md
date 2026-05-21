# Goal

Fix the Debug CRT heap-corruption dialog that appeared when closing the video player during playback, then implement the VLC-inspired video stage 2 GPU path with D3D11VA auto hardware decode and software fallback.

# Completed

- Replaced manual `VideoFrame` YUV plane `new[]` / `delete[]` ownership with `QByteArray`-backed RAII storage and deleted copy operations.
- Kept the existing OpenGL renderer path but made decoded frames use stable plane pointers owned by the frame object.
- Added unified video session release in `VideoPlayerWindow`, covering load-new-video, close-event, destructor, and load-failure paths.
- Detached `VideoRendererGL` / fallback renderer from `VideoBuffer` before `MediaSession` and `VideoSession` teardown.
- Added `VideoDecodeOptions` and D3D11VA auto hardware decode in `VideoDecoder`.
- Added hardware-frame transfer back to software frames before the existing YUV420P conversion path.
- Preserved software decode fallback when D3D11VA config, device creation, decoder open, or format selection fails.

# Changed Scope

- `src/video/VideoDecoder.h`
- `src/video/VideoDecoder.cpp`
- `src/video/VideoSession.h`
- `src/video/VideoSession.cpp`
- `src/video/VideoPlayerWindow.h`
- `src/video/VideoPlayerWindow.cpp`
- `src/video/MediaSession.h`
- `src/video/MediaSession.cpp`
- `src/video/VideoRenderer.cpp`
- `src/video/VideoRendererGL.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

# Open Work

- Manually replay the user-reported case: play the same video, close the video window, then exit the app in Debug.
- Check logs for `D3D11VA hardware decode enabled` on supported H.264/H.265 files, or a clear fallback reason.
- Runtime smoke test local file, HTTP MP4, HLS VOD, and HLS live close/seek/stop paths.
- Future stage: custom AVIO/prefetch for video and HLS ABR/variant fallback.

# Risks

- This stage transfers D3D11VA frames back to CPU before OpenGL upload; it is hardware decode, not zero-copy rendering.
- Some codecs/files may not expose D3D11VA through the FFmpeg 4.4 build and will intentionally fall back to software decode.
- The exact user video was not available for automated runtime verification in this session.

# Next Entry Point

Start with `src/video/VideoDecoder.cpp` around `initHardwareDevice()`, `chooseHardwareFormat()`, and `softwareFrameForConvert()` for GPU behavior. For close-time crashes, start with `src/video/VideoPlayerWindow.cpp::releaseMediaSession()` and `src/video/VideoSession.cpp::setVideoRenderer(nullptr)`.

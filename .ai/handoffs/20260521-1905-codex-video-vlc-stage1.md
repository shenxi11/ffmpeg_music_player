# Goal

Implement the first stage of the VLC-inspired video optimization plan for `src/video/`, focused on HTTP/HLS startup stability, bounded network I/O, safe seek/stop behavior, pixel-format compatibility, and renderer frame-queue ownership.

# Completed

- Added `MediaOpenOptions`, `VideoSourceInfo`, and `NetworkStats` to `src/video/MediaSession.*`.
- Classified local, HTTP, HLS VOD, and HLS live sources, and applied FFmpeg network options for timeout, read timeout, reconnect, keep-alive HTTP, probe size, analyze duration, user agent, optional headers, and seekable HTTP.
- Added FFmpeg interrupt callbacks with monotonic deadlines for open, stream info, and `av_read_frame()`.
- Hardened `seekTo()`, `stopDemux()`, and `cleanupDemuxer()` so blocking demux reads are interrupted and the demux thread is waited before seeking or closing the shared `AVFormatContext`.
- Updated `VideoDecoder` to normalize decoded frames to `AV_PIX_FMT_YUV420P` through swscale instead of assuming incoming frames are planar YUV420P.
- Added `VideoBuffer::peekInfo()` and `VideoBuffer::popIfPts()` and updated both renderers to avoid using queue-owned raw pointers after unlocking.
- Fixed chroma texture dimensions in `VideoRendererGL` for odd video dimensions.

# Changed Scope

- `src/video/MediaSession.h`
- `src/video/MediaSession.cpp`
- `src/video/VideoDecoder.h`
- `src/video/VideoDecoder.cpp`
- `src/video/VideoBuffer.h`
- `src/video/VideoBuffer.cpp`
- `src/video/VideoRenderer.cpp`
- `src/video/VideoRendererGL.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

# Open Work

- Runtime smoke test with real HTTP MP4 and HLS VOD/live URLs.
- Media-neutral custom AVIO/prefetch layer for video, similar to the audio VLC parity work.
- Independent video decode thread and tighter A/V clock ownership.
- Subtitle and audio-track selection.
- Hardware decode path and renderer upload optimization.
- HLS ABR, playlist refresh, variant fallback, and segment blacklist/retry policy.

# Risks

- FFmpeg option names depend on the linked FFmpeg protocol build; unsupported options are generally ignored, but runtime logs should confirm behavior.
- Demux thread wait timeout is bounded at 1500 ms. If a platform/protocol ignores interrupt callbacks, seek/close will now surface an error instead of racing the shared format context.
- Frame conversion improves compatibility but adds CPU cost until hardware decode or zero-copy paths are added.
- No runtime HTTP/HLS smoke test was performed in this session.

# Next Entry Point

Start with `src/video/MediaSession.cpp` around `initDemuxer()`, `seekTo()`, `stopDemux()`, `waitForDemuxThreadStopped()`, and `demuxLoop()`. Then test one local file, one HTTP MP4, and one HLS VOD/live stream before adding the next stage custom AVIO/prefetch layer.

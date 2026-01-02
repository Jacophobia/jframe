# miniaudio

This directory contains [miniaudio](https://github.com/mackron/miniaudio), a single-file audio playback and capture library.

## Version

v0.11.23 (2025-09-11)

## License

miniaudio is released under a choice of **Public Domain** or **MIT-0** (No Attribution). See the end of `miniaudio.h` for full license text.

## Usage in Bestow

miniaudio serves as the **fallback audio backend** when FMOD is not available. The system automatically selects:

1. **FMOD** (primary) - When `fmod` CMake target is available
2. **miniaudio** (fallback) - When FMOD is unavailable but miniaudio.h exists
3. **Stub mode** - When neither is available (no actual audio playback)

## Updating

To update miniaudio, download the latest version from:
https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h

```bash
curl -o miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
```

## Features Used

- `ma_engine` - High-level audio engine with 3D spatialization
- `ma_sound` - Sound playback with volume, pitch, and looping control
- 3D audio positioning with distance attenuation
- Multiple audio format decoding (WAV, FLAC, MP3, Vorbis)

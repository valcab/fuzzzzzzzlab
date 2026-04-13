# FuzzVST (JUCE)

Cross-platform JUCE guitar fuzz plugin targeting VST3 + AU (plus Standalone target for testing).

## Features

- JUCE `AudioProcessor` / `AudioProcessorEditor`
- Stereo in/out only
- APVTS parameter system (`AudioProcessorValueTreeState`)
- Real-time-safe audio path (no allocations in `processBlock`)
- 6 fuzz algorithms:
  - Hard clipping fuzz (Fuzz Face style approximation)
  - Octave fuzz (full-wave rectification + distortion)
  - Gated/velcro fuzz (bias-controlled asymmetric clipping + optional gate)
  - Muff-style fuzz (soft clipping + tone shaping)
  - Wavefolding fuzz
  - Bitcrushed/digital fuzz
- Switchable oversampling: 1x, 2x, 4x
- DC blocking high-pass stage
- Input gain, output gain, and dry/wet mix
- Optional cabinet simulation low-pass (~5 kHz)

## Build

### Prerequisites

- CMake 3.22+
- C++17 toolchain (Xcode on macOS)

### Configure + Build (macOS)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### One-command build (recommended)

```bash
make build
```

Other useful commands:

```bash
make debug
make release
make run-standalone
make clean
```

Or run the script directly:

```bash
./scripts/build.sh Release
./scripts/build.sh Debug
```

JUCE is pulled automatically with CMake `FetchContent`.

## Parameters

- `Input` (-24 dB to +24 dB)
- `Drive` (0 to 1)
- `Tone` (0 to 1)
- `Bias` (-1 to +1)
- `Octave Blend` (0 to 1)
- `Mix` (0 to 1)
- `Output` (-24 dB to +24 dB)
- `Fuzz Type` (Hard Clip, Octave, Gated, Muff, Wavefold, Bitcrush)
- `Oversampling` (1x, 2x, 4x)
- `Noise Gate` (on/off)
- `Cab Sim` (on/off)

## Project Structure

- `src/PluginProcessor.*`: parameter + DSP graph + algorithm routing
- `src/PluginEditor.*`: minimal control UI
- `src/dsp/FuzzAlgorithms.h`: modular fuzz algorithm classes

## Notes

- The algorithm comments in `FuzzAlgorithms.h` describe analog inspirations.
- The UI is intentionally minimal and can be extended with waveform preview later.

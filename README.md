# CPTR480 Trombone Theremin

An embedded real-time trombone theremin on the Raspberry Pi Pico that mimics trombone slide movements using contactless ultrasonic gesture control and harmonic series synthesis.

## What It Does

This project creates a playable electronic trombone using an **HC-SR04 ultrasonic sensor** to measure hand distance (the "slide") and a **rotary encoder** to select harmonic partials (1–12). Move the hand closer/farther to bend pitch; rotate the encoder to jump between harmonics. Press the button to activate/deactivate the note. All audio synthesis runs in real-time on dual CPU cores.

## How It Works

### System Overview

The system is split across two cores:
- **Core 0 (Control)**: Polls the distance sensor and rotary encoder, calculates target frequency, sends commands to Core 1 via FIFO
- **Core 1 (Audio)**: Generates 24-bit audio samples at 48 kHz via I2S→DAC output, driven by frequency commands from Core 0

### Sensor Input: Distance → Frequency

The **HC-SR04 ultrasonic sensor** measures the time-of-flight for a sound pulse bouncing off your hand:

1. **Distance Measurement**: Trigger pin sends an ultrasonic pulse; echo pin responds with a duration proportional to hand distance
2. **Wavelength Calibration**: A linear interpolation formula maps measured distance (mm) to acoustic wavelength (cm):
   ```
   wavelength = 4.2831e-3 × distance_mm + 5.5438
   ```
3. **Frequency Calculation**: Using the speed of sound (343 m/s):
   ```
   frequency = 343 (m/s) / wavelength (m)
   ```
   Sensor range: ~0–4 meters (sound limited to about 1 meter), real-time polling on Core 0

### Audio Synthesis: Real-Time DSP on Core 1

Core 1 generates audio samples in real-time:

- **I2S Protocol**: Uses the Pico's Programmable I/O (PIO) state machine to implement I2S (24-bit audio, 48 kHz sample rate)
- **DAC Output**: Samples stream to a digital-to-analog converter, then to a speaker
- **Fixed-Point Math**: Uses 16-bit fractional precision (`FREQ_FRAC_BITS`) to ensure accurate frequency synthesis without floating-point overhead
- **Inter-Core FIFO**: Core 0 sends frequency commands (as fixed-point integers) via the Pico's FIFO; Core 1 dequeues and synthesizes continuously

---

## System Architecture

### Dual-Core Design

```
┌─────────────────────────────┐
│        Core 0 (Control)     │
├─────────────────────────────┤
│  • Poll HC-SR04 sensor      │
│  • Read rotary encoder      │
│  • Check button state       │
│  • Calculate: freq = 343 /  │
│    (4.2831e-3×distance+...) │
│  • Multiply by partial      │
│  • Push to FIFO (fixed-pt)  │
└────────────┬────────────────┘
             │ Inter-Core FIFO
             │ (frequency cmds)
             ▼
┌─────────────────────────────┐
│      Core 1 (Audio Synth)   │
├─────────────────────────────┤
│  • Dequeue frequency from   │
│    FIFO                     │
│  • Generate waveform        │
│    samples (scaled)         │
│  • Output via I2S→DAC at    │
│    48 kHz, 24-bit           │
└─────────────────────────────┘
             │
             ▼ I2S Protocol
       ┌──────────────┐
       │   DAC Chip   │
       └──────────────┘
             │
             ▼ Analog
         [Speaker]
```

### Pin Assignments

- **Encoder**: GPIO 18 (A), GPIO 17 (B), GPIO 5 (Button)
- **HC-SR04**: GPIO 19 (Trigger), GPIO 20 (Echo)
- **I2S/DAC**: GPIO 9 (DOUT), GPIO 6 (BCLK), GPIO 7 (LRCLK)

---

## Key Features

- ✅ **Contactless Control**: Hand gestures replace physical sliding (like a theremin meets trombone)
- ✅ **Harmonic Series (1–12)**: Authentic trombone timbre; each partial covers a frequency band
- ✅ **Real-Time Synthesis**: Dual-core Pico architecture ensures low-latency, jitter-free audio
- ✅ **Calibrated Pitch**: Linear interpolation from distance sensor to frequency ensures smooth, musical control
- ✅ **Embedded Audio Output**: I2S→DAC chain at 48 kHz, 24-bit depth via Programmable I/O
- ✅ **Low Power**: Runs entirely on Raspberry Pi Pico (arm Cortex-M0+) with minimal external hardware

---

## Project Structure

| File | Role |
|------|------|
| `src/main.cpp` | Core 0 (sensor polling, frequency calculation, FIFO messaging) |
| `src/encoder.hpp` / `src/encoder.cpp` | Rotary encoder debouncing and partial selection logic |
| `src/HC-SR04.hpp` / `src/HC-SR04.cpp` | Ultrasonic distance sensor interface and time-of-flight measurement |
| `src/dac.hpp` / `src/dac.cpp` | Core 1 (I2S audio synthesis and DAC output via PIO) |
| `src/i2s.pio` | PIO program for I2S protocol state machine |
| `CMakeLists.txt` | Build configuration (top-level) |
| `src/CMakeLists.txt` | Pico application target definition |

---

## Requirements

- **CMake** 3.13+
- **Ninja** build system
- **Raspberry Pi Pico SDK** (local checkout with `PICO_SDK_PATH` environment variable set)
- **Hardware**:
  - Raspberry Pi Pico board (WWU CPTR480 2026 board or equivalent)
  - HC-SR04 ultrasonic distance sensor
  - Rotary encoder (with integrated pushbutton)
  - I2S compatible DAC (or equivalent audio output)
  - Speaker

---

## Build

```bash
# Configure (uses CMakePresets)
cmake --preset default

# Build
cmake --build --preset default
```

Output binary: `build/src/trombone.uf2` (ready to flash to Pico)

The Pico application target is defined in `src/CMakeLists.txt`.

---

## Usage

1. **Flash** `trombone.uf2` to your Pico board
2. **Connect hardware**:
   - HC-SR04 sensor to trigger (GPIO 19) and echo (GPIO 20)
   - Rotary encoder pins: GPIO 18 (A), GPIO 17 (B), GPIO 5 (button)
   - I2S DAC pins: GPIO 9 (DOUT), GPIO 6 (BCLK), GPIO 7 (LRCLK)
   - Speaker to DAC output
3. **Power on** and press the encoder button to activate
4. **Move your hand** ~0–100 cm from the sensor to control pitch
5. **Rotate the encoder** to change harmonics (1–12)

---

## Notes & Calibration

- **Frequency Range**: Determined by HC-SR04 measurement range (~0–4 m) and the calibration formula. Practical playing range is ~0–100 cm for musical frequencies.
- **Harmonic Boundaries**: Each partial `N` naturally constrains frequency to a useful range (e.g., partial 2 at fundamental F produces 2F; partial 12 produces 12F). Manual calibration may be needed for your sensor setup.
- **Calibration Formula**: The linear fit `wavelength = 4.2831e-3 × distance_mm + 5.5438` was derived from experimental measurements. If your sensor is significantly different, you may need to re-calibrate.

---

## Future Enhancements

- 🎼 **Waveform variety**: Add sine, square, sawtooth for different trombone colors
- 🎛️ **Effects**: Vibrato (hand tremolo), portamento (sliding between partials)
- 📊 **Visualization**: On-board display showing current partial and frequency
- 🔊 **Recording**: Store played notes to flash for later playback
- 🎯 **Tuning modes**: Fixed frequency ranges for specific musical temperaments

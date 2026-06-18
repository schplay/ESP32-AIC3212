# TLV320AIC3212 ESP32 Audio Control Library

ESP32-compatible I2C control class for the Texas Instruments TLV320AIC3212 audio codec.

## Attribution

This library is derived from the **TLV320AIC3212 control module** originally written by **Eric Yuan** for the [Tympan project](https://github.com/Tympan/Tympan_Library) (MIT License). The Tympan library targets the **Teensy Audio Library** infrastructure; this version is a ground-up rewrite that:

- Removes all Teensy Audio Library dependencies
- Targets the **ESP32-IDF I2S driver** directly
- Adds a generic I2C adapter template (`GenericI2CWrapper<T>`) so any `Wire`-compatible bus — including [SoftWire](https://github.com/nickcoutsos/ESP_SoftWire) — can be used without modifying the class
- Implements the ground-centered headphone driver sequencing required by [SLAU360](https://www.ti.com/lit/an/slau360/slau360.pdf) (TI application note for the GC HP output stage)
- Adds just-in-time (JIT) headset insertion detection designed to survive the codec's PLL-disturbing load-probe mechanism

---

## What It Does

The TLV320AIC3212 is a stereo audio codec with:

- 24-bit DAC / ADC, up to 192 kHz
- **Ground-centered headphone amplifier** (HPL / HPR) — differential, no DC-blocking caps required
- **Class D mono speaker amplifier** (LOR / LOL → SPKR)
- On-chip PLL, programmable biquad EQ, miniDSP processing blocks
- Headset insertion / type detection engine (TRS vs TRRS classification)
- I2C control (page/book register model), I2S audio

This library handles I2C register programming for initialization, routing, volume, and headset detection. It does **not** produce or process audio samples — that is handled by your I2S driver and your application code.

---

## Hardware Requirements

### Codec power supply

| Rail | Voltage | Notes |
|------|---------|-------|
| DVDD | 1.8 V | Digital core |
| IOVDD | 1.8 V or 3.3 V | I/O (I2C, I2S, GPIO) |
| AVDD | 1.8 V | ADC / DAC analog |
| DRVDD | 3.3 V | Headphone driver |
| SPKVDD | 3.3 V–5 V | Class D speaker driver |

### I2C address

The default address is **0x18** (AD0/AD1 = GND). The constructor accepts a custom address as the third argument.

### I2C pull-ups

Place **4.7 kΩ** pull-up resistors on SDA and SCL to IOVDD. The codec's I2C pins are open-drain. 400 kHz (Fast Mode) is the recommended bus speed.

### Reset pin

Connect `AUDIO_RST` (active LOW) to the codec's `RESET` pin. Drive it LOW for ≥ 10 ms before releasing. The codec must be held in reset while the I2S clock (BCLK) is being established; see [Initialization sequence](#initialization-sequence) below.

### I2S connections

The codec uses **I2S port 1** (ASI1) by default. Standard 4-wire I2S:

| ESP32 GPIO | Codec pin | Direction | Description |
|-----------|-----------|-----------|-------------|
| AUDIO_BCLK | BCLK1 | ESP32 → codec | Bit clock (master out) |
| AUDIO_WCLK | WCLK1 | ESP32 → codec | Word/LR clock (master out) |
| AUDIO_DOUT | DIN1 | ESP32 → codec | Audio data to DAC |
| AUDIO_DIN | DOUT1 | codec → ESP32 | Audio data from ADC |

No MCLK connection is needed — the codec derives its master clock from BCLK via the internal PLL.

### Headphone jack wiring

For the ground-centered HP driver, **HPVSS_SENSE must have a DC reference**. If the TRRS jack Ring2 connects to HPVSS_SENSE, ensure the node is anchored to ground on the board or configure the codec for internal-ground common-mode via P1_R34 D[7:6]=`10` (see [Gotchas](#critical-gotchas)).

For headset detection, connect the jack's microphone node to:

```
MICBIAS_EXT → 2.2 kΩ series resistor → MICDET (codec pin)
                                               ↓
                                         jack Sleeve (also → IN1L via 100 nF)
```

This is the exact TI reference arrangement from the SLAU360 application note.

---

## Library Installation

1. Copy `ESP32_AIC3212.h` and `ESP32_AIC3212.cpp` into your sketch folder (or a library folder).
2. Add `#include "ESP32_AIC3212.h"` to your sketch.
3. No additional library dependencies beyond the ESP-IDF I2S driver (included in the Arduino-ESP32 core).

### SoftWire compatibility

This class works with [SoftWire](https://github.com/nickcoutsos/ESP_SoftWire) — a bit-banged I2C implementation for ESP32 — via the `setWire<T>()` template method. SoftWire performs well on this codec at 400 kHz; using it on dedicated GPIO pins avoids bus contention with other I2C devices.

```cpp
SoftWire sw;
sw.begin(SDA_PIN, SCL_PIN, 400000);
audio.setWire(sw);   // must be called before audio.enable()
```

When using hardware `Wire`, simply call `Wire.begin(SDA, SCL, 400000)` before `audio.enable()` — no `setWire()` call is needed (the constructor defaults to `Wire` on bus index 0).

---

## Initialization Sequence

The order matters. Deviating from this sequence can leave the codec's PLL unlocked, the HP driver uncalibrated, or the I2S bus mismatched.

```cpp
// 1. Assert codec reset BEFORE I2S starts — the codec must not see a
//    live BCLK while its registers are in the reset state.
pinMode(AUDIO_RST, OUTPUT);
digitalWrite(AUDIO_RST, LOW);
delay(50);

// 2. Start the I2S driver. This begins driving BCLK and WCLK.
initI2S(AUDIO_BCLK, AUDIO_WCLK, AUDIO_DIN, AUDIO_DOUT);
delay(50);

// 3. Release reset. The codec now sees BCLK and can lock its PLL.
digitalWrite(AUDIO_RST, HIGH);
delay(50);

// 4. Start I2C.
Wire.begin(SDA_PIN, SCL_PIN, 400000);

// 5. Enable the codec. Writes all registers from DefaultConfig.
audio.enable();

// 6. Choose output.
audio.outputSelect(Aic_Output_None, Aic_Output_Spk, true);
audio.setSpeakerVolume_dB(30.0f);
audio.setDACVolume_dB(10.0f);

// 7. Enable headset detection; wait > 64 ms for first debounce cycle.
audio.enableHeadsetDetection();
delay(70);
bool hp = audio.isHeadsetInserted();
if (hp) {
    audio.outputSelect(Aic_Output_Hp, Aic_Output_Hp, true);
    audio.setHeadphoneGain_dB(0.0f, 0.0f);
    audio.setDACVolume_dB(0.0f);
}
```

---

## DefaultConfig — Clock Chain

`DefaultConfig` is the built-in configuration struct used by `enable()` when no custom config is passed. It targets **48 kHz playback** from a **3.072 MHz BCLK** (32-bit frames × 2 channels × 48 kHz = 3.072 MHz).

### PLL

```
BCLK1 (3.072 MHz) → PLL (×12) → PLL_CLK = 36.864 MHz
```

PLL multiplier = 12 (P=1, R=1, J=12, D=0).

### DAC clock dividers

```
PLL_CLK (36.864 MHz)
    ÷ NDAC (3) = 12.288 MHz (DAC clock)
    ÷ MDAC (4) = 3.072 MHz  (DAC modulator clock)
    ÷ DOSR (64) = 48 000 Hz (actual sample rate)
```

### Processing block

`PRB_P1` — processing block P1. Provides **3 biquad filter stages** on the DAC path and is required for the DC-blocking HPF written during `aic_init()`.

### Power tune mode

`PTM_P1` — lowest power consumption for DAC power tuning. Appropriate for portable / battery devices.

### I2S format

32-bit word, standard I2S (left-channel first, MSB-justified after WCLK edge). The codec's DAC ignores the lower 8 bits of each 32-bit word; audio data should be placed in the upper 24 bits (or upper 16 bits for 16-bit sources, left-shifted by 16).

---

## API Reference

All methods are members of `esp32aic3212::ESP32AIC3212`.

### Construction

```cpp
ESP32AIC3212 audio(resetPin, wireIndex, i2cAddress, debug);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `resetPin` | `int` | GPIO connected to codec RESET (active LOW) |
| `wireIndex` | `int` | `0` = Wire, `1` = Wire1 (ignored if `setWire()` is called) |
| `i2cAddress` | `uint8_t` | Default `0x18`; `0x19` if AD0 = VCC |
| `debug` | `bool` | Print register writes to `Serial` |

### Core

| Method | Returns | Description |
|--------|---------|-------------|
| `enable()` | `bool` | Soft-reset and configure using `DefaultConfig` |
| `enable(config)` | `bool` | Configure using a custom `Config` struct |
| `disable()` | `void` | Power down codec |
| `setWire<T>(wire)` | `void` | Attach any `TwoWire`-compatible bus (SoftWire, Wire, Wire1) |

### Output routing

```cpp
audio.outputSelect(AIC_Output left, AIC_Output right, bool powerUp);
```

`AIC_Output` values: `Aic_Output_Hp`, `Aic_Output_Spk`, `Aic_Output_None`.

When `powerUp = true`, the method powers up the selected driver (HP or speaker). After selecting `Aic_Output_Hp`, an **unconditional 300 ms delay** is applied internally before the method returns, to allow the ground-centered HP driver's offset calibration to complete. Do not attempt to play audio until this wait has elapsed.

### Volume

| Method | Description |
|--------|-------------|
| `setDACVolume_dB(dB)` | Digital DAC volume (both channels). Range approximately −63 to +24 dB |
| `setHeadphoneGain_dB(leftdB, rightdB)` | HP analog volume register. 0 dB = unity; negative values attenuate |
| `setSpeakerVolume_dB(dB)` | Class D speaker gain register |
| `volume_dB(dB)` | Shorthand — sets DAC volume on both channels |
| `setDACMute(mute)` | Mute / unmute DAC. Use to bracket `i2s_set_clk()` calls |

### Headset detection

| Method | Returns | Description |
|--------|---------|-------------|
| `enableHeadsetDetection()` | `void` | Enable the codec's continuous headset detection engine |
| `disableHeadsetDetection()` | `void` | Disable the detection engine |
| `isHeadsetInserted()` | `bool` | Read the current insertion flag (engine must be running) |
| `checkHeadsetPresence(currently_hp)` | `bool` | JIT probe — see [JIT detection](#just-in-time-jit-headset-detection) |

### Playback / recording mode

| Method | Description |
|--------|-------------|
| `setPlaybackMode()` | Route ASI1 I2S input → DAC (normal playback) |
| `setRecordingMode()` | Route ADC → ASI1 I2S output |

### Low-level register access

```cpp
audio.aic_writePage(page, reg, value);   // write one register on a given page
audio.aic_readPage(page, reg);           // read one register, returns uint8_t
```

The codec uses a **book / page / register** hierarchy. `aic_writePage` and `aic_readPage` operate within the currently selected book (use `aic_writePage(0, 0, book)` to switch books). All methods in the class use page 0 and page 1 (book 0) except `writeBiquadCoefficients()` which uses book 80.

### Diagnostics

```cpp
audio.diagHP();   // Print HP analog register state to Serial (mute, volume, driver enable)
```

---

## Just-in-Time (JIT) Headset Detection

Headset detection is expensive: enabling the detection engine fires a small current pulse (load probe) onto the HP output pins to classify the load. This is harmless when the HP driver is powered down, but with the driver live, the probe pulse can disturb the ground-centered amplifier's offset servo and cause a brief output glitch.

The JIT pattern eliminates continuous detection and performs a single probe only at the beginning of each playback request, before any audio data flows:

```cpp
// At the START of every playback, before i2s_write:
bool inserted = audio.checkHeadsetPresence(headphones_connected);
if (inserted != headphones_connected) {
    headphones_connected = inserted;
    if (inserted) {
        audio.outputSelect(Aic_Output_Hp, Aic_Output_Hp, true);
        audio.setHeadphoneGain_dB(0.0f, 0.0f);
        audio.setDACVolume_dB(0.0f);
    } else {
        audio.outputSelect(Aic_Output_None, Aic_Output_Spk, true);
        audio.setSpeakerVolume_dB(30.0f);
        audio.setDACVolume_dB(10.0f);
    }
}
// Now start audio.
```

Rules:
- **Only change the output if the state actually changed.** If the state is the same as last time, touch nothing — no I2C writes, no `outputSelect`.
- **Never read codec I2C registers during playback.** The I2C transaction introduces latency that causes DMA underruns (audible clicks). All detection must happen before the first `i2s_write` call.
- The 300 ms HP calibration wait inside `outputSelect` is enforced unconditionally. Budget for it.

`checkHeadsetPresence()` behavior depends on the current output mode:

- **Speaker mode** (`currently_using_hp = false`): performs a passive register read (no probe pulses). The codec's continuous detection engine is running and the R46/R37 flags are valid.
- **HP mode** (`currently_using_hp = true`): enables detection for one 25 ms window (16 ms debounce + margin), reads R46/R37, then disables detection. If the headset is still inserted, immediately re-asserts the three critical HP driver registers (P1_R08, P1_R23, P1_R22) to undo any analog disturbance from the probe — the **probe + heal** pattern.

---

## I2S Sample Rate Changes

`i2s_set_clk()` unlocks the codec's PLL. On a device using the ground-centered HP driver, this tears down the offset servo reference and can leave the HP output silently wedged until the driver is powered down and back up. The wedge is not visible in register state — `diagHP()` will show normal values while the analog stage produces no output.

To avoid this:

1. **Set the I2S rate only once per playback, at the very start**, before `outputSelect` or any audio data.
2. **Never restore the rate after playback ends.** Use a `g_i2s_rate` variable to track the live bus rate; only call `i2s_set_clk` when the new file's rate differs from `g_i2s_rate`.
3. **Mute the DAC around any `i2s_set_clk` call when headphones are connected**:

```cpp
if (g_i2s_rate != new_rate) {
    if (headphones_connected) audio.setDACMute(true);
    i2s_set_clk(I2S_PORT, new_rate, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_STEREO);
    g_i2s_rate = new_rate;
    i2s_zero_dma_buffer(I2S_PORT);
    if (headphones_connected) audio.setDACMute(false);
}
```

The best long-term fix is to resample all audio assets to 48 kHz so that rate changes never occur in normal operation.

---

## Critical Gotchas

### 1. HPVSS_SENSE must have a DC reference

The ground-centered HP driver uses HPVSS_SENSE as the reference for its offset servo. If this node is floating (e.g., connected only to a TRRS jack Ring2 with nothing plugged in), the servo chases the drift and ramps the output toward a rail. The result is silent output and a stuck over-current flag in R2C.

**Fix**: Set P1_R34 D[7:6] = `10` (internal-ground common-mode, SLAU360 §5.3.27). This is written during `aic_init()`:
```cpp
aic_writePage(1, 0x22, 0xBE);  // P1_R34: internal-ground CM
```
Alternatively, tie HPVSS_SENSE to board GND with a resistor on the PCB.

### 2. 300 ms HP calibration wait is unconditional

After every HP driver power-up (`outputSelect` with `Aic_Output_Hp`), there must be a minimum wait for the offset calibration to settle before audio flows. TI does not publish the exact duration; 300 ms has been validated empirically. Starting playback before the cal completes produces a loud click and potentially an over-current trip.

`outputSelect` enforces this wait internally. Do not route around it.

### 3. Operating mode must be set after configuration

After `enable()` configures the codec, it must be explicitly set to **Active Measure Mode** to produce real ADC/DAC values. The codec powers up in **Configuration Mode** where all conversions return zero. This is handled inside `enable()` via `setOperatingMode(OPERATING_MODE_ActiveMeasureMode)`.

If you are using the codec in a standalone / non-library context, always call:
```cpp
aic_writePage(0, 0x04, 0x03);  // P0_R04: set Active Measure Mode
```

### 4. MICBIAS_EXT can corrupt the HP offset servo on mic-capable headsets

The codec's headset detection engine automatically enables MICBIAS_EXT when it classifies the headset as having a microphone. If MICBIAS_EXT connects to the TRRS Sleeve, and the Sleeve also connects to HPVSS_SENSE (directly or via the mic circuit), the bias current introduces a DC offset into the GC servo reference, corrupting the calibration.

If the microphone input is **not used**, disable the automated MICBIAS_EXT behavior permanently:
```cpp
aic_writePage(1, 0x33, 0x80);  // P1_R51: MICBIAS_EXT manual mode, permanently off
```
This is written during `aic_init()` in this library.

### 5. Load probe auto-slowdown produces a whine on TRRS loads

By default, the codec's headset detection engine slows its probe cadence after classifying a high-impedance load, but the slowdown transition itself creates an audible low-frequency whine at approximately 1 kHz on TRRS headsets. Disable the auto-slowdown:
```cpp
aic_writePage(1, 0x77, 0x95);  // P1_R119: disable probe auto-slowdown
```
This is written during `aic_init()` in this library.

### 6. Over-current is configured as limit-and-continue, not latch-off

The HP over-current protection is set to clip-and-continue rather than silencing the driver:
```cpp
aic_writePage(1, 0x09, 0x18);  // P1_R09: OC limit-and-continue (D0=0)
```
With the default latch-off behavior (D0=1), a brief over-current event — such as the one caused by a floating HPVSS_SENSE — would silence the HP output permanently until the driver is powered off and back on. Limit-and-continue allows the driver to recover from transient OC events without losing audio, at the cost of brief clipping instead.

### 7. i2s_zero_dma_buffer before and after playback

Always call `i2s_zero_dma_buffer(I2S_PORT)` after changing the I2S clock rate and after stopping playback. If the DMA buffer still holds non-zero samples when the I2S rate changes, the output produces a short burst of alias noise as the old data is clocked out at the wrong rate.

### 8. APLL is required for accurate 48 kHz

The ESP32 integer clock divider cannot produce an exact 3.072 MHz BCLK from typical crystal frequencies. Set `use_apll = true` in `i2s_config_t`. Without APLL, the codec's PLL will lock to a slightly wrong frequency, causing pitch drift and eventual frame slip.

### 9. HWCDC RX buffer

Unrelated to the codec, but relevant if you are using `Serial` on the ESP32-S3 with HWCDC: the default RX buffer is approximately 256 bytes. For high-throughput serial logging during audio playback, call `Serial.setRxBufferSize(2048)` **before** `Serial.begin()`.

---

## Example

See [`AIC3212-ESP32-example.ino`](AIC3212-ESP32-example.ino) in this directory for a complete working sketch demonstrating:

- I2S and codec initialization
- Speaker and headphone output switching
- Just-in-time headset detection
- WAV file playback from SPIFFS
- Sine tone generation with fade-in/fade-out
- FreeRTOS high-priority playback task

---

## License

MIT — see original [Tympan_Library](https://github.com/Tympan/Tympan_Library) for Eric Yuan's original copyright notice.

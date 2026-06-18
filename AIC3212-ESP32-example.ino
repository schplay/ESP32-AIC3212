/*
 * TLV320AIC3212 — ESP32-S3 Audio Example
 *
 * Demonstrates:
 *   - Codec initialization over I2C (hardware Wire; SoftWire drop-in compatible)
 *   - I2S audio output at 48 kHz, 32-bit frames, using ESP32 APLL for accurate clocking
 *   - Speaker output via the Class D amplifier path (LOR → SPKR)
 *   - Headphone output via the ground-centered HP driver (HPL / HPR)
 *   - Just-in-time (JIT) headset insertion detection with output switching
 *   - WAV file playback from SPIFFS
 *   - Sine tone generation with linear fade-in and fade-out
 *   - FreeRTOS high-priority playback task pinned to core 0
 *
 * Based on the TLV320AIC3212 control library originally written by Eric Yuan for
 * the Tympan project (https://github.com/Tympan/Tympan_Library, MIT License).
 * Refactored for ESP32-S3 / ESP-IDF I2S driver; SoftWire I2C adapter added;
 * headphone detection, ground-centered driver sequencing, and JIT output
 * switching implemented from scratch.
 *
 * License: MIT
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <driver/i2s.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "ESP32_AIC3212.h"

using namespace esp32aic3212;

// =============================================================================
// Pin assignments — adjust for your board
// =============================================================================
#define AUDIO_RST   9    // Codec hardware reset (active LOW)
#define AUDIO_SDA  15    // I2C SDA for codec
#define AUDIO_SCL   7    // I2C SCL for codec
#define AUDIO_BCLK  3    // I2S bit clock  (BCLK1)
#define AUDIO_WCLK  8    // I2S word clock (WCLK1 / LRC)
#define AUDIO_DOUT 16    // I2S data to codec  (DIN1 on codec)
#define AUDIO_DIN   6    // I2S data from codec (DOUT1 on codec, for recording)

// =============================================================================
// Audio constants
// =============================================================================
#define SAMPLE_RATE       48000
#define I2S_PORT          I2S_NUM_0
#define PLAYBACK_BUF_SIZE 128   // samples per I2S write (stereo pairs)
#define FADE_SAMPLES      256   // linear fade-in / fade-out length

// =============================================================================
// Codec instance
//   Constructor args: (resetPin, i2cBusIndex, i2cAddress, debugToSerial)
//   i2cBusIndex 0 = Wire, 1 = Wire1.
//   For SoftWire: call audio.setWire(mySoftWire) after construction instead.
// =============================================================================
ESP32AIC3212 audio(AUDIO_RST, 0, 0x18, false);

// =============================================================================
// Playback queue (FreeRTOS)
// =============================================================================
typedef enum { CMD_PLAY_FILE, CMD_PLAY_SINE, CMD_STOP } CmdType;

typedef struct {
    CmdType  type;
    char     filename[64];
    int      frequency;
    uint32_t duration_ms;
} PlaybackCmd;

static QueueHandle_t    playback_queue      = NULL;
static TaskHandle_t     playback_task_handle = NULL;
static volatile bool    stop_playback       = false;
static volatile bool    headphones_connected = false;
static uint32_t         g_i2s_rate          = SAMPLE_RATE;  // tracks live bus rate

// =============================================================================
// Forward declarations
// =============================================================================
bool     initI2S(void);
void     switchToSpeaker(void);
void     switchToHeadphones(void);
bool     jitDetectAndSwitch(void);  // returns true if headphones currently inserted
void     playSineTone(int frequency, uint32_t duration_ms);
void     playWavFile(const char *path);
void     playback_task(void *arg);

// =============================================================================
// setup()
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("AIC3212 example starting");

    // ---- SPIFFS ----
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
    }

    // ---- I2S driver ----
    if (!initI2S()) {
        Serial.println("I2S init failed — halting");
        while (1) delay(1000);
    }

    // ---- Codec reset ----
    // Assert reset BEFORE I2C begins, release AFTER I2S is running so the
    // codec's PLL can lock onto the BCLK that is already present.
    pinMode(AUDIO_RST, OUTPUT);
    digitalWrite(AUDIO_RST, LOW);
    delay(50);
    digitalWrite(AUDIO_RST, HIGH);
    delay(50);

    // ---- I2C init ----
    // Use hardware Wire on the codec's dedicated SDA/SCL pins at 400 kHz.
    // To use SoftWire instead, replace these two lines with:
    //   SoftWire sw;
    //   sw.begin(AUDIO_SDA, AUDIO_SCL, 400000);
    //   audio.setWire(sw);
    Wire.begin(AUDIO_SDA, AUDIO_SCL, 400000);
    delay(10);

    // ---- Codec init ----
    // enable() performs a soft-reset, then writes all clock, PLL, I2S-format,
    // processing-block, and biquad-coefficient registers from DefaultConfig.
    // DefaultConfig targets 48 kHz from a 3.072 MHz BCLK1 via PLL (×12 = 36.864 MHz).
    if (!audio.enable()) {
        Serial.println("Codec enable() failed — check I2C wiring and reset pin");
        while (1) delay(1000);
    }
    Serial.println("Codec enabled");

    // ---- Default output: speaker ----
    switchToSpeaker();

    // ---- Headset detection ----
    // Enable the codec's built-in headset detection engine and read the initial
    // insertion state. Wait > 64 ms (one detection debounce period) before reading.
    audio.enableHeadsetDetection();
    delay(70);
    headphones_connected = audio.isHeadsetInserted();
    Serial.printf("Initial headset state: %s\n",
                  headphones_connected ? "INSERTED" : "not inserted");

    if (headphones_connected) {
        switchToHeadphones();
    }

    // ---- Playback task ----
    playback_queue = xQueueCreate(4, sizeof(PlaybackCmd));
    xTaskCreatePinnedToCore(
        playback_task,          // function
        "playback",             // name
        8192,                   // stack (bytes)
        NULL,                   // arg
        20,                     // priority (high — prevents preemption during audio)
        &playback_task_handle,
        0                       // core 0
    );

    Serial.println("Setup complete");
}

// =============================================================================
// loop() — enqueue playback commands
// =============================================================================
void loop() {
    // Example: play a 1 kHz tone for 500 ms on button press.
    // Replace this with whatever triggers audio in your application.

    // ---- Play a sine tone ----
    {
        PlaybackCmd cmd = {};
        cmd.type        = CMD_PLAY_SINE;
        cmd.frequency   = 1000;
        cmd.duration_ms = 500;
        xQueueSend(playback_queue, &cmd, 0);
    }
    delay(2000);

    // ---- Play a WAV file ----
    {
        PlaybackCmd cmd = {};
        cmd.type = CMD_PLAY_FILE;
        strncpy(cmd.filename, "/beep.wav", sizeof(cmd.filename) - 1);
        xQueueSend(playback_queue, &cmd, 0);
    }
    delay(3000);
}

// =============================================================================
// I2S initialization
// =============================================================================
bool initI2S(void) {
    i2s_config_t cfg = {};
    cfg.mode                = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
    cfg.sample_rate         = SAMPLE_RATE;
    cfg.bits_per_sample     = I2S_BITS_PER_SAMPLE_32BIT;  // codec expects 32-bit frames
    cfg.channel_format      = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags    = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count       = 8;
    cfg.dma_buf_len         = 512;
    cfg.use_apll            = true;   // APLL produces a more accurate 48 kHz than the integer divider
    cfg.tx_desc_auto_clear  = true;

    if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) return false;

    i2s_pin_config_t pins = {};
    pins.bck_io_num      = AUDIO_BCLK;
    pins.ws_io_num       = AUDIO_WCLK;
    pins.data_out_num    = AUDIO_DOUT;
    pins.data_in_num     = AUDIO_DIN;
    pins.mck_io_num      = I2S_PIN_NO_CHANGE;  // MCLK not used — codec derives clock from BCLK

    if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) return false;

    i2s_zero_dma_buffer(I2S_PORT);
    i2s_start(I2S_PORT);
    return true;
}

// =============================================================================
// Output routing helpers
// =============================================================================
void switchToSpeaker(void) {
    // Route DAC_R → LOR → SPKR (right-channel mono speaker output).
    // DAC digital gain +10 dB compensates for the Class D driver's lower sensitivity
    // compared to the headphone driver. Speaker Class D gain is set to 30 dB.
    audio.outputSelect(Aic_Output_None, Aic_Output_Spk, true);
    audio.setSpeakerVolume_dB(30.0f);
    audio.setDACVolume_dB(10.0f);
    Serial.println("[Output] Speaker");
}

void switchToHeadphones(void) {
    // Route DAC_L → HPL and DAC_R → HPR in ground-centered mode.
    // HP analog volume register set to 0 dB (unity); +10 dB DAC digital gain is
    // intentionally avoided on headphones because it approaches DAC full-scale and
    // triggers over-current limiting in the HP driver. Keep DAC at 0 dB here.
    audio.outputSelect(Aic_Output_Hp, Aic_Output_Hp, true);
    audio.setHeadphoneGain_dB(0.0f, 0.0f);
    audio.setDACVolume_dB(0.0f);
    Serial.println("[Output] Headphones");
}

// =============================================================================
// JIT headset detection
//
// Call once at the START of every playback request, before any audio flows.
// Switches the output only if the insertion state actually changed.
// Returns true if headphones are currently inserted.
// =============================================================================
bool jitDetectAndSwitch(void) {
    bool inserted = audio.checkHeadsetPresence(headphones_connected);
    if (inserted == headphones_connected) return inserted;  // no change — touch nothing

    headphones_connected = inserted;
    if (inserted) {
        switchToHeadphones();
    } else {
        switchToSpeaker();
    }
    return inserted;
}

// =============================================================================
// WAV file playback from SPIFFS
//
// Reads a standard PCM WAV from SPIFFS, sets the I2S rate to match the file's
// sample rate (only if it differs from the current bus rate), then streams the
// audio in 32-bit stereo frames.
//
// IMPORTANT: i2s_set_clk() unlocks the codec PLL and can disturb the HP analog
// stage. For this reason the rate is only changed at the START of a playback and
// is NEVER restored at the end. g_i2s_rate tracks the live bus rate so the next
// playback call knows whether a rate change is actually needed.
// =============================================================================
void playWavFile(const char *path) {
    File f = SPIFFS.open(path, "r");
    if (!f) { Serial.printf("Cannot open %s\n", path); return; }

    // --- Parse WAV header ---
    // Skip RIFF / fmt chunks to find 'data' chunk, handling extra chunks gracefully.
    uint8_t hdr[44];
    if (f.read(hdr, 44) < 44) { f.close(); return; }

    uint16_t num_channels  = hdr[22] | (hdr[23] << 8);
    uint32_t sample_rate   = hdr[24] | (hdr[25] << 8) | (hdr[26] << 16) | (hdr[27] << 24);
    uint16_t bits_per_samp = hdr[34] | (hdr[35] << 8);

    // Scan forward to the 'data' chunk (handles LIST/fact/PEAK extra chunks)
    uint32_t data_size = 0;
    f.seek(12);
    while (f.available() >= 8) {
        char id[4];
        f.read((uint8_t *)id, 4);
        uint32_t chunk_size = 0;
        f.read((uint8_t *)&chunk_size, 4);
        if (memcmp(id, "data", 4) == 0) { data_size = chunk_size; break; }
        f.seek(f.position() + chunk_size);  // skip non-data chunk
    }
    if (data_size == 0) { f.close(); return; }

    Serial.printf("[WAV] %s: %u Hz, %u ch, %u-bit, %u bytes\n",
                  path, sample_rate, num_channels, bits_per_samp, data_size);

    // --- I2S rate adjustment ---
    // Mute DAC around i2s_set_clk to suppress the transient pop caused by the
    // PLL unlock / relock sequence.
    if (g_i2s_rate != sample_rate) {
        Serial.printf("[RATE] %u -> %u Hz\n", g_i2s_rate, sample_rate);
        if (headphones_connected) audio.setDACMute(true);
        i2s_set_clk(I2S_PORT, sample_rate, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_STEREO);
        g_i2s_rate = sample_rate;
        i2s_zero_dma_buffer(I2S_PORT);
        if (headphones_connected) audio.setDACMute(false);
    }

    // Switch codec ASI1 to playback routing (I2S data → DAC)
    audio.setPlaybackMode();

    // --- Stream audio ---
    const int READ_BUF_SAMPLES = PLAYBACK_BUF_SIZE;
    int32_t *out_buf = (int32_t *)malloc(READ_BUF_SAMPLES * 2 * sizeof(int32_t));
    if (!out_buf) { f.close(); return; }

    uint32_t bytes_remaining = data_size;
    size_t bytes_written;

    while (bytes_remaining > 0 && !stop_playback) {
        // Read one buffer of PCM samples from the file
        int bytes_to_read = min((uint32_t)(READ_BUF_SAMPLES * num_channels * (bits_per_samp / 8)),
                                bytes_remaining);
        uint8_t raw[READ_BUF_SAMPLES * 4];  // worst case: stereo 16-bit
        int bytes_read = f.read(raw, bytes_to_read);
        if (bytes_read <= 0) break;
        bytes_remaining -= bytes_read;

        int samples_read = bytes_read / (num_channels * (bits_per_samp / 8));

        // Convert file samples to 32-bit stereo I2S frames.
        // The AIC3212 uses 32-bit I2S frames (matches DefaultConfig I2S_32_BITS).
        // Audio data lives in the upper 16 bits; lower 16 are zero-padded.
        for (int i = 0; i < samples_read; i++) {
            int32_t l = 0, r = 0;
            if (bits_per_samp == 16) {
                int16_t s = (int16_t)(raw[i * 2] | (raw[i * 2 + 1] << 8));
                l = (int32_t)s << 16;
                r = (num_channels == 2)
                    ? (int32_t)((int16_t)(raw[i * 4 + 2] | (raw[i * 4 + 3] << 8))) << 16
                    : l;
            } else if (bits_per_samp == 24) {
                int32_t s = (int32_t)((raw[i * 3] << 8) | (raw[i * 3 + 1] << 16) | (raw[i * 3 + 2] << 24)) >> 8;
                l = s << 8;
                r = (num_channels == 2)
                    ? (int32_t)((raw[i * 6 + 3] << 8) | (raw[i * 6 + 4] << 16) | (raw[i * 6 + 5] << 24)) >> 8 << 8
                    : l;
            }
            out_buf[2 * i]     = l;
            out_buf[2 * i + 1] = r;
        }

        i2s_write(I2S_PORT, out_buf, samples_read * 2 * sizeof(int32_t),
                  &bytes_written, pdMS_TO_TICKS(100));
    }

    free(out_buf);
    f.close();
    i2s_zero_dma_buffer(I2S_PORT);
    Serial.println("[WAV] Done");
}

// =============================================================================
// Sine tone generation
//
// Generates a continuous sine wave with a 256-sample linear fade-in at start
// and a 256-sample linear fade-out after a CMD_STOP is received. The fade-out
// allows the DMA pipeline to drain to near-zero before the buffer is cleared,
// preventing the step discontinuity (pop / over-current stress) that occurs when
// i2s_zero_dma_buffer is called on a non-zero signal.
//
// The amplitude figures here suit a system where:
//   - Speaker: up to 30,000 / 32,767 ≈ full scale is safe (Class D tolerates it)
//   - Headphones: capped below ≈ 8,000 / 32,767 to stay within the HP over-current
//     limit.  OC is configured as limit-and-continue (P1_R09 D0=0), so a trip
//     clips rather than silencing, but audible distortion starts first.
// Tune these to match your headphone impedance and desired listening level.
// =============================================================================
void playSineTone(int frequency, uint32_t duration_ms) {
    // Ensure I2S is at SAMPLE_RATE before we generate.
    // A previous WAV may have left the bus at the file's rate.
    if (g_i2s_rate != SAMPLE_RATE) {
        Serial.printf("[RATE] %u -> %d Hz before sine\n", g_i2s_rate, SAMPLE_RATE);
        if (headphones_connected) audio.setDACMute(true);
        i2s_set_clk(I2S_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_STEREO);
        g_i2s_rate = SAMPLE_RATE;
        i2s_zero_dma_buffer(I2S_PORT);
        if (headphones_connected) audio.setDACMute(false);
    }

    audio.setPlaybackMode();
    i2s_zero_dma_buffer(I2S_PORT);

    float amplitude = headphones_connected ? 8000.0f : 30000.0f;
    Serial.printf("[Sine] %d Hz, amp=%.0f, dur=%u ms\n", frequency, amplitude, duration_ms);

    const int BUF = PLAYBACK_BUF_SIZE;
    int32_t *buf = (int32_t *)malloc(BUF * 2 * sizeof(int32_t));
    if (!buf) return;

    int      phase_acc       = 0;
    uint32_t sample_count    = 0;
    bool     fade_out_active = false;
    uint32_t fade_out_count  = 0;
    uint32_t stop_at_ms      = (duration_ms > 0) ? (millis() + duration_ms) : 0;
    size_t   bytes_written;

    while (!stop_playback) {
        PlaybackCmd check;
        if (xQueueReceive(playback_queue, &check, 0) == pdTRUE) {
            if (check.type == CMD_STOP && !fade_out_active) {
                fade_out_active = true;
                fade_out_count  = 0;
            }
        }
        if (stop_at_ms > 0 && millis() >= stop_at_ms && !fade_out_active) {
            fade_out_active = true;
            fade_out_count  = 0;
        }
        if (stop_playback) break;

        for (int i = 0; i < BUF; i++) {
            double angle = 2.0 * M_PI * frequency * phase_acc / (double)SAMPLE_RATE;
            float raw = amplitude * (float)sin(angle);

            float env_in  = (sample_count < FADE_SAMPLES)
                            ? (float)(sample_count + 1) / FADE_SAMPLES : 1.0f;
            float env_out = fade_out_active
                            ? (float)(FADE_SAMPLES - min(fade_out_count, (uint32_t)FADE_SAMPLES)) / FADE_SAMPLES
                            : 1.0f;

            int32_t s = (int32_t)((int16_t)(raw * env_in * env_out)) << 16;
            buf[2 * i]     = s;
            buf[2 * i + 1] = s;

            phase_acc++;
            sample_count++;
            if (fade_out_active) {
                fade_out_count++;
                if (fade_out_count >= FADE_SAMPLES) { stop_playback = true; break; }
            }
        }

        i2s_write(I2S_PORT, buf, BUF * 2 * sizeof(int32_t), &bytes_written, pdMS_TO_TICKS(100));
    }

    free(buf);
    i2s_zero_dma_buffer(I2S_PORT);
    Serial.println("[Sine] Done");
}

// =============================================================================
// FreeRTOS playback task
//
// Runs at priority 20 on core 0. High priority prevents the UART / other tasks
// from preempting mid-buffer and causing DMA underruns (audible clicks/dropouts).
//
// Each command performs a JIT headset check BEFORE audio flows. If the insertion
// state changed since the last playback, the output is reconfigured once. If
// there is no state change, the codec is not touched at all — avoiding the 300 ms
// HP calibration wait and the PLL-disturbing i2s_set_clk on every playback.
// =============================================================================
void playback_task(void *arg) {
    PlaybackCmd cmd;

    while (1) {
        if (xQueueReceive(playback_queue, &cmd, portMAX_DELAY) != pdTRUE) continue;

        stop_playback = false;

        // ---- JIT headset detection ----
        bool was_hp = headphones_connected;
        bool is_hp  = jitDetectAndSwitch();

        switch (cmd.type) {
            case CMD_PLAY_FILE:
                Serial.printf("[Task] Play file: %s\n", cmd.filename);
                playWavFile(cmd.filename);
                break;

            case CMD_PLAY_SINE:
                Serial.printf("[Task] Sine: %d Hz, %u ms\n", cmd.frequency, cmd.duration_ms);
                playSineTone(cmd.frequency, cmd.duration_ms);
                break;

            case CMD_STOP:
                stop_playback = true;
                i2s_zero_dma_buffer(I2S_PORT);
                break;
        }
    }
}

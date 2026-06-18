/*
    ESP32_AIC3212

    Purpose: I2C control library for the TLV320AIC3212 audio codec on ESP32

    License: MIT License.  Use at your own risk.
 */

#ifndef ESP32_AIC3212_H_
#define ESP32_AIC3212_H_

#include <Arduino.h>
#include <Wire.h>

namespace esp32aic3212 {
    #define AIC3212_DEFAULT_RESET_PIN 254          // A10 on the Rev-E2
    #define AIC3212_DEFAULT_I2C_ADDRESS 0b0011000 // 7-Bit address

    enum AIC_ID
    {
        Aic_Id_1=0,
        Aic_Id_2
    };

    enum AIC_Input
    {
        Aic_Input_In1=0,
        Aic_Input_In2,
        Aic_Input_In3,
        Aic_Input_Pdm,     // PDM Mic with PDM_CLK on GPIO2, PDM_DAT on GPI1
        Aic_Input_None
    };

    enum AIC_Output
    {
        Aic_Output_Hp,         // Headphone HPL/HPR
        Aic_Output_Spk,        // Speakers (SPKLP-SPKLM)/(SPKRP-SPKRM)
        Aic_Output_None
    };

    enum AIC_Side 
    {
        Left_Chan               = 0,
        Right_Chan              = 1
    };

    enum Mic_Bias
    {
        Mic_Bias_Off            = 0x00,
        Mic_Bias_1_62           = 0x01,
        Mic_Bias_2_4            = 0x02,
        Mic_Bias_3_0            = 0x03,
        Mic_Bias_3_3            = 0x04
    };

    struct InputChannel_s 
    {
        AIC_ID      aicId;
        AIC_Input   inputChan;
        AIC_Side    sideChan;
        Mic_Bias    micBias;
    };

    struct OutputChannel_s 
    {
        AIC_ID      aicId;
        AIC_Output  outputChan;
        AIC_Side    sideChan;
    };

    enum class DAC_PowerTune_Mode : uint8_t
    {
        PTM_P4 = 0x00, // (default) also PTM_P3
        PTM_P2 = 0x01,
        PTM_P1 = 0x02
    };

    enum class ADC_PowerTune_Mode : uint8_t
    {
        PTM_R4 = 0x00, // default setting at AIC startup
        PTM_R3 = 0x01,
        PTM_R2 = 0x02,
        PTM_R1 = 0x03
    };

    enum class I2S_Clock_Dir : uint8_t
    {
        AIC_INPUT = 0x00,
        AIC_OUTPUT = 0x0C
    };

    // PLL Clock Range (See SLAU360, Table 2-41)
    enum class PLL_Clock_Range : uint8_t
    {
        LOW_RANGE = 0x00,
        HIGH_RANGE = 0x40
    };

    enum class PLL_Source : uint8_t
    {
        MCLK1 = 0x00,
        BCLK1 = 0x01,
        GPIO1 = 0x02,
        DIN1 = 0x03,
        BCLK2 = 0x04,
        GPI1 = 0x05,
        HF_REF_CLK = 0x06,
        GPIO2 = 0x07,
        GPI2 = 0x08,
        MCLK2 = 0x09
    };

    enum class ADC_DAC_Clock_Source : uint8_t
    {
        MCLK1 = 0x00,
        BCLK1 = 0x01,
        GPIO1 = 0x02,
        PLL_CLK = 0x03,
        BCLK2 = 0x04,
        GPI1 = 0x05,
        HF_REF_CLK = 0x06,
        HF_OSC_CLK = 0x07,
        MCLK2 = 0x08,
        GPIO2 = 0x09,
        GPI2 = 0x0A
    };

    enum class I2S_Word_Length : uint8_t
    {
        I2S_16_BITS = 0x00,
        I2S_20_BITS = 0x01,
        I2S_24_BITS = 0x02,
        I2S_32_BITS = 0x03
    };

    struct PLL_Config
    {
        PLL_Clock_Range range;
        PLL_Source src;
        uint8_t r;
        uint8_t j;
        uint16_t d;
        uint8_t p;
        uint8_t clkin_div;
        bool enabled;
    };

    struct DAC_Config
    {
        ADC_DAC_Clock_Source clk_src;
        uint8_t ndac;
        uint8_t mdac;
        uint16_t dosr;
        uint8_t prb_p;
        DAC_PowerTune_Mode ptm_p;
    };

    struct ADC_Config
    {
        ADC_DAC_Clock_Source clk_src;
        uint8_t nadc; // If NADC == 0, ADC_CLK is the same as DAC_CLK
        uint8_t madc; // If MADC == 0, ADC_MOD_CLK is same as DAC_MOD_CLK
        uint16_t aosr;
        uint8_t prb_r;
        ADC_PowerTune_Mode ptm_r;
    };

    struct Config
    {
        I2S_Word_Length i2s_bits;  // I2S data word length
        I2S_Clock_Dir i2s_clk_dir; // I2S clock direction
        PLL_Config pll;            // PLL configuration
        DAC_Config dac;            // DAC configuration
        ADC_Config adc;            // ADC configuration
    };

    //*******************************  Constants  ********************************//

    extern const Config DefaultConfig;

    uint8_t getAudioConnInput(InputChannel_s inputChan);
    uint8_t getAudioConnOutput(OutputChannel_s outputChan);
    
    #define AIC3212_LEFT_CHAN 1
    #define AIC3212_RIGHT_CHAN 2
    #define AIC3212_BOTH_CHAN 0

    // ---- Small I2C wrapper interface and helpers ----
    class I2CDevice {
    public:
        virtual ~I2CDevice() {}
        virtual void begin() = 0;
        virtual void beginTransmission(uint8_t addr) = 0;
        virtual size_t write(uint8_t b) = 0;
        virtual uint8_t endTransmission() = 0;
        // Use size_t here so both TwoWire::requestFrom and SoftWire::requestFrom match
        virtual size_t requestFrom(uint8_t addr, size_t count) = 0;
        virtual int available() = 0;
        virtual int read() = 0;
    };

    // Concrete wrapper for TwoWire (Wire, Wire1, Wire2)
    class TwoWireWrapper : public I2CDevice {
    public:
        TwoWireWrapper(TwoWire *w) : wire(w) {}
        virtual ~TwoWireWrapper() {}
        void begin() override { if (wire) wire->begin(); }
        void beginTransmission(uint8_t addr) override { if (wire) wire->beginTransmission(addr); }
        size_t write(uint8_t b) override { return wire ? wire->write(b) : 0; }
        uint8_t endTransmission() override { return wire ? wire->endTransmission() : 1; }
        size_t requestFrom(uint8_t addr, size_t count) override { return wire ? wire->requestFrom(addr, count) : 0; }
        int available() override { return wire ? wire->available() : 0; }
        int read() override { return wire ? wire->read() : -1; }
    private:
        TwoWire *wire;
    };

    // Generic templated wrapper for any TwoWire-like object (e.g., SoftWire)
    // This wrapper adapts to APIs that provide:
    //   begin(), beginTransmission(uint8_t), write(uint8_t), endTransmission([bool]), requestFrom(addr,count[,bool]), available(), read()
    template<typename T>
    class GenericI2CWrapper : public I2CDevice {
    public:
        GenericI2CWrapper(T *obj) : inst(obj) {}
        virtual ~GenericI2CWrapper() {}
        void begin() override { inst->begin(); }
        void beginTransmission(uint8_t addr) override { inst->beginTransmission(addr); }
        size_t write(uint8_t b) override { return inst->write(b); }
        // call endTransmission with explicit stop bit true when available; works for TwoWire and SoftWire (they both accept optional bool)
        uint8_t endTransmission() override { return (uint8_t)inst->endTransmission(true); }
        // requestFrom returns size_t for both types; we call using the 2-arg overload and use default stop-bit
        size_t requestFrom(uint8_t addr, size_t count) override { return inst->requestFrom(addr, count); }
        int available() override { return inst->available(); }
        int read() override { return inst->read(); }
    private:
        T *inst;
    };

    class ESP32AIC3212
    {
            public:
                ESP32AIC3212(void)
                {
                        debugToSerial = false;
                        myWire = new TwoWireWrapper(&Wire);
                }
                ESP32AIC3212(bool _debugToSerial)
                {
                        debugToSerial = _debugToSerial;
                        myWire = new TwoWireWrapper(&Wire);
                }
                ESP32AIC3212(int _resetPin)
                {
                        resetPinAIC = _resetPin;
                        debugToSerial = false;
                        myWire = new TwoWireWrapper(&Wire);
                }
                ESP32AIC3212(int _resetPin, int i2cBusIndex, uint8_t _i2cAddress)
                {
                        setResetPin(_resetPin);
                        setI2Cbus(i2cBusIndex);
                        i2cAddress = _i2cAddress;
                        debugToSerial = false;
                }
                ESP32AIC3212(int _resetPin, int i2cBusIndex, uint8_t _i2cAddress, bool _debugToSerial)
                {
                        setResetPin(_resetPin);
                        setI2Cbus(i2cBusIndex);
                        i2cAddress = _i2cAddress;
                        debugToSerial = _debugToSerial;
                }
                virtual ~ESP32AIC3212(void) {
                    if (myWire) {
                        delete myWire;
                        myWire = nullptr;
                    }
                };
        
        void setReset(int pin) { setResetPin(pin); }

        // Attach any TwoWire-compatible bus (SoftWire, Wire1, etc.) before calling enable()
        template<typename T>
        void setWire(T &wire) {
            if (myWire) { delete myWire; myWire = nullptr; }
            myWire = new GenericI2CWrapper<T>(&wire);
        }

        virtual bool enable(void);
        void setConfig(const Config *_pConfig) { pConfig = _pConfig; };

        bool outputSelect(AIC_Output both, bool flag_full = true) { return outputSelect(both, both, flag_full); };
        bool outputSelect(AIC_Output left, AIC_Output right, bool flag_full = true);

        float setAdcVolume_dB(float vol_dB);
        static float applyLimitsOnVolumeSetting(float vol_dB);
        float volume_dB(float vol_dB);                          // set both channels to the same volume
        float volume_dB(float vol_left_dB, float vol_right_dB); // set both channels, but to their own values
        float volume_dB(float vol_left_dB, int chan);           // set each channel seperately (0 = left; 1 = right)
                float get_volume_dB(int chan);
        float setHeadphoneGain_dB(float vol_left_dB, float vol_right_dB); // set HP volume
        float setSpeakerVolume_dB(float target_vol_dB);         // sets the volume of both Class D Speaker Outputs
        void setDACVolume_dB(float vol_dB);                     // sets DAC digital volume (P0_R41/R42)
        void writeBiquadCoefficients(void);                     // write biquad coefficients during init (called internally)
                int enableHeadphonePower(int chan, bool enable = true); //use AIC_BOTH_CHAN, AIC_LEFT_CHAN, AIC_RIGHT_CHAN
        void enableHeadsetDetection(void);           // enable codec headset insertion detection
        void disableHeadsetDetection(void);          // disable detection (prevents MICBIAS interfering with HP output)
        bool isHeadsetInserted(void);                // read current headset insertion status (requires detection enabled + debounce elapsed)
        void diagHP(void);                           // print HP driver register snapshot for diagnostics
        uint8_t readP1R1B(void);                     // read P1_R1B (HP driver power/routing) without side-effects
        uint8_t readP0R43(void);                     // read P0_R43 (headset detection / MICBIAS enable) without side-effects
        uint8_t readSCFlags(void);                   // read-and-clear P0_R44 sticky SC flags (0xC0 = both HP SC fired)
        void setDACMute(bool mute);                  // mute/unmute DAC output (P0_R40 only); does not touch HP driver state
        bool checkHeadsetPresence(bool currently_using_hp); // JIT insertion check: call once at the start of every playback request

        bool inputSelect(int both) { return inputSelect(both, both); };
        bool inputSelect(int left, int right);
        float applyLimitsOnInputGainSetting(float gain_dB);
        float setInputGain_dB(float gain_dB);           // set both channels to the same gain
        float setInputGain_dB(float gain_dB, int chan); // set each channel seperately (0 = left; 1 = right)
        bool setMicBias(Mic_Bias biasSetting);
        bool setMicBiasExt(Mic_Bias biasSetting);

        bool debugToSerial;
        unsigned int aic_readPage(uint8_t page, uint8_t reg);
        bool aic_writePage(uint8_t page, uint8_t reg, uint8_t val);

        void setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz); // first-order HP applied within this 3212 hardware, ADC (input) side
        float getHPCutoff_Hz(void) { return HP_cutoff_Hz; }
        void setHpfIIRCoeffOnADC(int chan, uint32_t *coeff);                                             // alternate way of settings the same 1st-order HP filter
        int setBiquadCoeffOnADC(int chanIndex, int biquadIndex, uint32_t *coeff_uint32);
        void writeBiquadCoeff(uint32_t *coeff_uint32, int *page_reg_table, int table_ncol);

        float getSampleRate_Hz(void) { return sample_rate_Hz; }
        bool enableAutoMuteDAC(bool, uint8_t);
        bool mixInput1toHPout(bool state);
        void muteLineOut(bool state);

        bool aic_goToBook(uint8_t book);
        bool aic_goToPage(uint8_t page);

        unsigned int aic_readRegister(uint8_t reg, uint8_t *pVal); // Assumes page has already been set
        bool aic_writeRegister(uint8_t reg, uint8_t val);          // assumes page has already been set
        bool outputSpk(void);

        // Digital microphone configuration and diagnostics
        bool configureDigitalMicOnDIN2(void);  // Configure PDM digital mic on DIN2/BCLK2 pins
        void diagnosticDigitalMic(void);        // Print comprehensive diagnostic info for digital mic setup

        // ASI1 routing mode control
        bool setRecordingMode(void);  // Set P4_R7 to route ADC→ASI1 for recording
        bool setPlaybackMode(void);   // Set P4_R7 to route ASI1→DAC for playback

    protected:
        const Config *pConfig = &DefaultConfig;
        I2CDevice *myWire = nullptr;
        uint8_t i2cAddress = AIC3212_DEFAULT_I2C_ADDRESS;
        void setI2Cbus(int i2cBus);
        void aic_reset(void);
        void aic_init(void);
        void setResetPin(int pin) { resetPinAIC = pin; }
                bool firstTime_outputSelect = true;

        int prevMicDetVal = -1;
        int resetPinAIC = AIC3212_DEFAULT_RESET_PIN;
        float HP_cutoff_Hz = 0.0f;
        float sample_rate_Hz = 44100; // only used with HP_cutoff_Hz to design HP filter on ADC, if used
        void setHpfIIRCoeffOnADC_Left(uint32_t *coeff);
        void setHpfIIRCoeffOnADC_Right(uint32_t *coeff);

        void computeFirstOrderHPCoeff_f32(float cutoff_Hz, float fs_Hz, float *coeff);
        void computeBiquadCoeff_LP_f32(float cutoff_Hz, float sampleRate_Hz, float q, float *coeff);
        void computeBiquadCoeff_HP_f32(float cutoff_Hz, float sampleRate_Hz, float q, float *coeff);
        void convertCoeff_f32_to_i32(float *coeff_f32, int32_t *coeff_i32, int ncoeff);
    };

} // namespace esp32aic3212

#endif  //ESP32_AIC3212_H_
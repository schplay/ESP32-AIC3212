/*
    ESP32_AIC3212

    Purpose: I2C control library for the TLV320AIC3212 audio codec on ESP32

    License: MIT License.  Use at your own risk.
 */

#include "ESP32_AIC3212.h"

namespace esp32aic3212
{

// Software Reset
#define AIC3212_SOFTWARE_RESET_BOOK 0x00
#define AIC3212_SOFTWARE_RESET_PAGE 0x00
#define AIC3212_SOFTWARE_RESET_REG 0x01
#define AIC3212_SOFTWARE_RESET_INITIATE 0x01

//******************* INPUT DEFINITIONS *****************************//
#define AIC3212_MICPGA_PAGE                             0x01
#define AIC3212_MICPGA_LEFT_POSITIVE_REG                0x34
#define AIC3212_MICPGA_RIGHT_POSITIVE_REG               0x37
#define AIC3212_MICPGA_LEFT_NEGATIVE_REG                0x36
#define AIC3212_MICPGA_RIGHT_NEGATIVE_REG               0x39
#define AIC3212_MICPGA_LEFT_VOLUME_REG                  0x3B
#define AIC3212_MICPGA_RIGHT_VOLUME_REG                 0x3C
#define AIC3212_MICPGA_VOLUME_ENABLE                    0b00000000

#define AIC3212_MIC_ROUTING_POSITIVE_IN1                0b11000000
#define AIC3212_MIC_ROUTING_POSITIVE_IN2                0b00110000
#define AIC3212_MIC_ROUTING_POSITIVE_IN3                0b00001100
#define AIC3212_MIC_ROUTING_POSITIVE_REVERSE            0b00000011

#define AIC3212_MIC_ROUTING_NEGATIVE_CM_TO_CM1L         0b11000000
#define AIC3212_MIC_ROUTING_NEGATIVE_IN2_REVERSE        0b00110000
#define AIC3212_MIC_ROUTING_NEGATIVE_IN3_REVERSE        0b00001100
#define AIC3212_MIC_ROUTING_NEGATIVE_CM_TO_CM2L         0b00000011

#define AIC3212_MIC_ROUTING_RESISTANCE_10k              0b01010101
#define AIC3212_MIC_ROUTING_RESISTANCE_20k              0b10101010
#define AIC3212_MIC_ROUTING_RESISTANCE_40k              0b11111111
#define AIC3212_MIC_ROUTING_RESISTANCE_DEFAULT          AIC3212_MIC_ROUTING_RESISTANCE_10k

// Mic Bias
#define AIC3212_MIC_BIAS_PAGE                       0x01
#define AIC3212_MIC_BIAS_REG                        0x33
#define AIC3212_MIC_BIAS_EXT_MASK                   0b11110000
#define AIC3212_MIC_BIAS_EXT_MANUAL_POWER           0b10000000
#define AIC3212_MIC_BIAS_EXT_POWER_ON               0b01000000
#define AIC3212_MIC_BIAS_EXT_POWER_OFF              0b00000000
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_1_62    0b00000000
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_2_4     0b00010000
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_3_0     0x00100000
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_3_3     0x00110000

#define AIC3212_MIC_BIAS_MASK                       0b00001111
#define AIC3212_MIC_BIAS_MANUAL_POWER               0b00001000
#define AIC3212_MIC_BIAS_POWER_ON                   0b00000100
#define AIC3212_MIC_BIAS_POWER_OFF                  0b00000000
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_1_62        0b00000000
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_2_4         0b00000001
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_0         0x00000010
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_3         0x00000011

// ADC Processing Block
#define AIC3212_ADC_PROCESSING_BLOCK_PAGE 0x00
#define AIC3212_ADC_PROCESSING_BLOCK_REG 0x3d

// ADC power & digital mic configuration
#define AIC3212_ADC_CHANNEL_POWER_PAGE 0x00
#define AIC3212_ADC_CHANNEL_POWER_REG 0x51
#define AIC3212_ADC_CHANNEL_POWER_REG_PWR_MASK 0b11000000
#define AIC3212_ADC_CHANNELS_ON 0b11000000

#define AIC3212_ADC_CHANNEL_POWER_REG_L_DIG_MIC_MASK 0b00111100
#define AIC3212_ADC_LEFT_CONFIGURE_FOR_DIG_MIC 0b00010000
#define AIC3212_ADC_RIGHT_CONFIGURE_FOR_DIG_MIC 0b00000100

// Mute the ADC
#define AIC3212_ADC_MUTE_PAGE 0x00
#define AIC3212_ADC_MUTE_REG 0x52
#define AIC3212_ADC_UNMUTE 0b00000000
#define AIC3212_ADC_MUTE 0b10001000

// ADC filter coefficients
#define AIC3212_ADC_IIR_FILTER_BOOK 0x28
#define AIC3212_ADC_IIR_FILTER_LEFT_PAGE 0x01
#define AIC3212_ADC_IIR_FILTER_RIGHT_PAGE 0x02

// ADC Digital Volume Control
#define AIC3212_ADC_VOLUME_BOOK 0x00
#define AIC3212_ADC_VOLUME_PAGE 0x00
#define AIC3212_ADC_VOLUME_LEFT_REGISTER 0x53
#define AIC3212_ADC_VOLUME_RIGHT_REGISTER 0x54
#define AIC3212_ADC_VOLUME_MIN ( (int8_t) -12 )
#define AIC3212_ADC_VOLUME_MAX ( (int8_t) 20 )
#define AIC3212_ADC_VOLUME_MASK 0x7F

// DAC volume registers
#define AIC3212_DAC_VOLUME_PAGE 0x00
#define AIC3212_DAC_VOLUME_LEFT_REG 0x41
#define AIC3212_DAC_VOLUME_RIGHT_REG 0x42

// Headphone volume
#define AIC3212_HP_VOLUME_PAGE 0x01
#define AIC3212_HPL_VOLUME_REG 0x1F
#define AIC3212_HPR_VOLUME_REG 0x20
#define AIC3212_HP_VOLUME_MASK 0b00111111
#define AIC3212_HP_VOLUME_MIN (int8_t)-6
#define AIC3212_HP_VOLUME_MAX (int8_t)14

// Speaker volume
#define AIC3212_SPKR_VOLUME_PAGE 0x01
#define AIC3212_SPKR_VOLUME_REG 0x30

// PDM control
#define AIC3212_BCLK2_PIN_CTRL_PAGE 0x04
#define AIC3212_BCLK2_PIN_CTRL_REG 0x46
#define AIC3212_BCLK2_DISABLED 0b00000000
#define AIC3212_BCLK2_ENABLE_PDM_CLK 0b00101000

#define AIC3212_DIN2_PIN_CTRL_PAGE 0x04
#define AIC3212_DIN2_PIN_CTRL_REG 0x48
#define AIC3212_DIN2_DISABLED 0b00000000
#define AIC3212_DIN2_ENABLED 0b00100000

#define AIC3212_GPI1_PIN_CTRL_PAGE              0x04
#define AIC3212_GPI1_PIN_CTRL_REG               0x5B

#define AIC3212_GPIO2_PIN_CTRL_PAGE             0x04
#define AIC3212_GPIO2_PIN_CTRL_REG              0x57

#define AIC3212_DIGITAL_MIC_SETTING_PAGE 0x04
#define AIC3212_DIGITAL_MIC_SETTING_REG 0x65
#define AIC3212_DIGITAL_MIC_DIN2_LEFT_RIGHT 0b00000011

#define AIC3212_ADC_POWERTUNE_PAGE 0x01
#define AIC3212_ADC_POWERTUNE_REG 0x3D

// Default configuration for 48kHz sample rate using PLL
// BCLK = 48kHz × 64 = 3.072 MHz (32-bit stereo I2S frames)
// PLL_OUT = BCLK × 12 = 36.864 MHz
// DAC_CLK = PLL_OUT / NDAC / MDAC = 36864kHz / 3 / 4 = 3072 kHz
// Sample Rate = DAC_CLK / DOSR = 3072kHz / 64 = 48,000 Hz
const Config DefaultConfig{
    .i2s_bits = I2S_Word_Length::I2S_32_BITS,  // Match ESP32 I2S config (32-bit)
    .i2s_clk_dir = I2S_Clock_Dir::AIC_INPUT,
    .pll = {
        .range = PLL_Clock_Range::LOW_RANGE,
        .src = PLL_Source::BCLK1,  // Use BCLK1 (3.072 MHz) as PLL input
        .r = 1,
        .j = 12,
        .d = 0,  // PLL_OUT = (3.072MHz / 1) * 12 = 36.864 MHz
        .p = 1,
        .clkin_div = 1,
        .enabled = true},  // PLL ENABLED for ADC clock generation
    .dac = {.clk_src = ADC_DAC_Clock_Source::PLL_CLK,  // Use PLL (36.864 MHz) — required for PRB_P1 RC=8 constraint
            .ndac = 3,  // 36.864MHz / (3×4×64×48kHz) = 36.864MHz ✓
            .mdac = 4,  // MDAC×DOSR/32 = 4×64/32 = 8 = PRB_P1 RC requirement ✓
            .dosr = 64,  // DOSR×fs = 64×48kHz = 3.072MHz ✓ (within 2.8–6.2MHz range per SLAU360)
            .prb_p = 1,  // PRB_P1: 3 BiQuads → Interp. Filter A → volume control → output (slau360)
            .ptm_p = DAC_PowerTune_Mode::PTM_P1},  // PTM_P1: best linearity/SNR for low-impedance loads (8Ω speaker, ≤64Ω headphones)
    .adc = {.clk_src = ADC_DAC_Clock_Source::PLL_CLK,  // Use PLL (36.864 MHz) for ADC
            .nadc = 1,
            .madc = 12,  // MADC=12 satisfies PRB_R7 constraint: MADC×AOSR/32=24 (≥23 required)
            .aosr = 64,  // Optimized for Decimation Filter B
            .prb_r = 7,  // PRB_R7: Decimation Filter B - full audio bandwidth (0-21.6kHz @ 48kHz)
            .ptm_r = ADC_PowerTune_Mode::PTM_R4}
            // ADC_MOD_CLK = 36.864MHz / (1×12) = 3.072 MHz (optimal for CMM-3424DT-26165-TR)
            // Sample rate = 36.864MHz / (1×12×64) = 48 kHz
            // Passband: 0-0.45×Fs = 0-21.6 kHz (vs PRB_R13: 0-5.3 kHz at 48kHz)
};

// helper functions
uint8_t getAudioConnInput(InputChannel_s inputChan) 
{
    uint8_t audioConn = 0;
    if (inputChan.aicId == Aic_Id_2) audioConn += 2;
    if (inputChan.sideChan == Right_Chan) audioConn += 1;
    return audioConn;
}

uint8_t getAudioConnOutput(OutputChannel_s outputChan) 
{
    uint8_t audioConn = 0;
    if (outputChan.aicId == Aic_Id_2) audioConn += 2;
    if (outputChan.sideChan == Right_Chan) audioConn += 1;
    return audioConn;
}

void ESP32AIC3212::setI2Cbus(int i2cBusIndex)
{
    if (myWire) { delete myWire; myWire = nullptr; }
    switch (i2cBusIndex)
    {
    case 0:
        myWire = new TwoWireWrapper(&Wire);
        break;
    case 1:
#ifdef Wire1
        myWire = new TwoWireWrapper(&Wire1);
#else
        myWire = new TwoWireWrapper(&Wire);
#endif
        break;
    case 2:
#ifdef Wire2
        myWire = new TwoWireWrapper(&Wire2);
#else
        myWire = new TwoWireWrapper(&Wire);
#endif
        break;
    default:
        myWire = new TwoWireWrapper(&Wire);
        break;
    }
}

bool ESP32AIC3212::enable(void)
{
    // Hardware reset handled by sketch before calling enable()
    aic_reset();
    delay(3);  // TI App Guide shows a 1 msec delay
    aic_init();

    aic_goToBook(0);
    aic_readPage(0, 27);

    if (debugToSerial) Serial.println("# AIC3212 enable done");
    return true;
}

bool ESP32AIC3212::inputSelect(int left, int right)
{     
    if (debugToSerial) Serial.println("# ESP32AIC3212: inputSelect");
    
    uint8_t errFlag = false;
    
    uint8_t
        b0_p0_r81 = 0x00,   // Power down left and right ADC channel, clear Digital Mic config
        b0_p0_r82 = 0x88,   // Mute ADC Fine Gain Volume
        b0_p1_r52 = 0,      // Left Mic PGA disconnected
        b0_p1_r54 = 0,      // Left Mic PGA disconnected
        b0_p1_r55 = 0,      // Right Mic PGA disconnected
        b0_p1_r57 = 0,      // Right Mic PGA disconnected
        b0_p1_r59 = 0x80,   // Mute Left MICPGA Volume
        b0_p1_r60 = 0x80,   // Mute Right MICPGA Volume
        b0_p4_r87 = 0,      // GPIO2 disabled
        b0_p4_r91 = 0,      // GPI1 disabled
        b0_p4_r101 = 0b00000000; // Left and Right DIN Both on GPI1 (B0_P4_R101)


    // LEFT
    if ( (left == Aic_Input_In1) || (left == Aic_Input_In2) || (left == Aic_Input_In3) )
    {
        b0_p1_r54 = 0b01000000;
        b0_p1_r59 = 0x00;
        b0_p0_r81 |= 0b10000000;
        b0_p0_r82 &= 0x7F;
    }
    else if (left == Aic_Input_Pdm)
    {
        b0_p0_r81 |= 0b10010000;
        b0_p0_r82 &= 0x7F;
    }

    // RIGHT
    if ( (right == Aic_Input_In1) || (right == Aic_Input_In2) || (right == Aic_Input_In3) )
    {
        b0_p1_r57 = 0b01000000;
        b0_p1_r60 = 0x00;
        b0_p0_r81 |= 0b01000000;
        b0_p0_r82 &= 0xF7;
    }
    else if (right == Aic_Input_Pdm)
    {
        b0_p0_r81 |= 0b01000100;
        b0_p0_r82 &= 0xF7;
    }

    // LEFT routing
    switch(left)
    {
        case Aic_Input_In1:
            b0_p1_r52 = (AIC3212_MIC_ROUTING_POSITIVE_IN1 & AIC3212_MIC_ROUTING_RESISTANCE_10k);
            break;
        case Aic_Input_In2:
            b0_p1_r52 = (AIC3212_MIC_ROUTING_POSITIVE_IN2 & AIC3212_MIC_ROUTING_RESISTANCE_10k);
            break;
        case Aic_Input_In3:
            b0_p1_r52 = (AIC3212_MIC_ROUTING_POSITIVE_IN3 & AIC3212_MIC_ROUTING_RESISTANCE_10k);
            break;
        case Aic_Input_Pdm:
            b0_p4_r87 = 0b00101000;
            b0_p4_r91 = 0b00000010;
            b0_p4_r101 = 0b00000000;
            break;
        case Aic_Input_None:
        default:
            break;
    }

    // RIGHT routing
    switch(right)
    {
        case Aic_Input_In1:
            b0_p1_r55 = (AIC3212_MIC_ROUTING_POSITIVE_IN1 & AIC3212_MIC_ROUTING_RESISTANCE_10k);
            break;
        case Aic_Input_In2:
            b0_p1_r55 = (AIC3212_MIC_ROUTING_POSITIVE_IN2 & AIC3212_MIC_ROUTING_RESISTANCE_10k);
            break;
        case Aic_Input_In3:
            b0_p1_r55 = (AIC3212_MIC_ROUTING_POSITIVE_IN3 & AIC3212_MIC_ROUTING_RESISTANCE_10k);
            break;
        case Aic_Input_Pdm:
            b0_p4_r87 = 0b00101000;
            b0_p4_r91 = 0b00000010;
            b0_p4_r101 = 0b00000000;
            break;
        case Aic_Input_None:
        default:
            break;
    }

    // -- Write Registers --
    aic_goToBook(0);

    errFlag &= aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_LEFT_POSITIVE_REG, b0_p1_r52);
    errFlag &= aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_LEFT_NEGATIVE_REG, b0_p1_r54);
    errFlag &= aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_LEFT_VOLUME_REG, b0_p1_r59);

    errFlag &= aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_RIGHT_POSITIVE_REG, b0_p1_r55);
    errFlag &= aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_RIGHT_NEGATIVE_REG, b0_p1_r57);
    errFlag &= aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_RIGHT_VOLUME_REG, b0_p1_r60);

    errFlag &= aic_writePage(AIC3212_GPIO2_PIN_CTRL_PAGE, AIC3212_GPIO2_PIN_CTRL_REG, b0_p4_r87);
    errFlag &= aic_writePage(AIC3212_GPI1_PIN_CTRL_PAGE, AIC3212_GPI1_PIN_CTRL_REG, b0_p4_r91);  
    errFlag &= aic_writePage(AIC3212_DIGITAL_MIC_SETTING_PAGE, AIC3212_DIGITAL_MIC_SETTING_REG, b0_p4_r101); 

    errFlag &= aic_writePage(AIC3212_ADC_CHANNEL_POWER_PAGE, AIC3212_ADC_CHANNEL_POWER_REG, b0_p0_r81);   //Power up ADC and set digital mic input (if selected)
    errFlag &= aic_writePage(AIC3212_ADC_CHANNEL_POWER_PAGE, AIC3212_ADC_MUTE_REG, b0_p0_r82);   //Unmute Fine Gain

    if (errFlag == 0) {
        return true;
    } else {
        Serial.println("ESP32AIC3212: ERROR: Unable to write to Input Select Registers");
        return false;
    }
}

bool ESP32AIC3212::setMicBias(Mic_Bias biasSetting)
{
    uint8_t buffer = 0;
    bool errStatus = false;
        
    aic_goToBook(0);
    aic_goToPage(AIC3212_MIC_BIAS_PAGE);

    aic_readRegister(AIC3212_MIC_BIAS_REG, &buffer);
    buffer &= ~AIC3212_MIC_BIAS_MASK;

    if (biasSetting == Mic_Bias_Off)
    {
        buffer |= AIC3212_MIC_BIAS_POWER_OFF;
    }
    else
    {
        buffer |= (AIC3212_MIC_BIAS_POWER_ON | AIC3212_MIC_BIAS_MANUAL_POWER);

        if (biasSetting == Mic_Bias_1_62)
        {
            buffer |= AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_1_62;
        }
        else if (biasSetting == Mic_Bias_2_4)
        {
            buffer |= AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_2_4;
        }
        else if (biasSetting == Mic_Bias_3_0)
        {
            buffer |= AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_0;
        }
        else if (biasSetting == Mic_Bias_3_3)
        {
            buffer |= AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_3;
        }
        else
        {
            Serial.println("ESP32AIC3212: ERROR: Unable to set MIC BIAS - Value not supported: ");
        }
    }

    errStatus = aic_writeRegister(AIC3212_MIC_BIAS_REG, buffer);

    if(errStatus==false)
    {
        Serial.println("ESP32AIC3212: ERROR: Unable to write to Mic Bias Register");
    }

    return errStatus;
}


 bool ESP32AIC3212::setMicBiasExt(Mic_Bias biasSetting)
 {
    uint8_t buffer = 0;
    bool errStatus = false;

    aic_goToBook(0);
    aic_goToPage(AIC3212_MIC_BIAS_PAGE);

    aic_readRegister(AIC3212_MIC_BIAS_REG, &buffer);
    buffer &= ~AIC3212_MIC_BIAS_EXT_MASK;

    if (biasSetting == Mic_Bias_Off)
    {
        buffer |= AIC3212_MIC_BIAS_EXT_POWER_OFF;
    }
    else
    {
        buffer |= (AIC3212_MIC_BIAS_EXT_POWER_ON | AIC3212_MIC_BIAS_EXT_MANUAL_POWER);

        if (biasSetting == Mic_Bias_1_62)
        {
            buffer |= AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_1_62;
        }
        else if (biasSetting == Mic_Bias_2_4)
        {
            buffer |= AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_2_4;
        }
        else if (biasSetting == Mic_Bias_3_0)
        {
            buffer |= AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_3_0;
        }
        else if (biasSetting == Mic_Bias_3_3)
        {
            buffer |= AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_3_3;
        }
        else
        {
            Serial.println("ESP32AIC3212: ERROR: Unable to set MIC BIAS Ext- Value not supported: ");
        }
    }

    errStatus = aic_writeRegister(AIC3212_MIC_BIAS_REG, buffer);

    if(errStatus==false)
    {
        Serial.println("ESP32AIC3212: ERROR: Unable to write to Mic Bias Ext Register");
    }

    return errStatus;
 }

void ESP32AIC3212::aic_reset()
{
    if (debugToSerial)
    {
        Serial.println("# INFO: Resetting AIC3212");
        Serial.print("#   7-bit I2C address: 0x");
        Serial.println(static_cast<uint8_t>(i2cAddress), HEX);
    }
    aic_goToBook(AIC3212_SOFTWARE_RESET_BOOK);
    aic_writePage(AIC3212_SOFTWARE_RESET_PAGE, AIC3212_SOFTWARE_RESET_REG, AIC3212_SOFTWARE_RESET_INITIATE);

    delay(10);
}

float ESP32AIC3212::applyLimitsOnInputGainSetting(float gain_dB)
{
    if (gain_dB < 0.0) gain_dB = 0.0;
    if (gain_dB > 47.5) gain_dB = 47.5;
    return gain_dB;
}

float ESP32AIC3212::setInputGain_dB(float orig_gain_dB, int Ichan)
{
    float gain_dB = applyLimitsOnInputGainSetting(orig_gain_dB);
    if (abs(gain_dB - orig_gain_dB) > 0.01)
    {
        Serial.println("ESP32AIC3212: WARNING: Attempting to set input gain outside allowed range");
    }

    float volume = gain_dB * 2.0;
    int8_t volume_int = (int8_t)(round(volume));

    if (debugToSerial)
    {
        Serial.print("AIC3212: Setting Input volume to ");
        Serial.print(gain_dB, 1);
        Serial.print(".  Converted to volume map => ");
        Serial.println(volume_int);
    }

    aic_goToBook(0);
    if (Ichan == 0) {
        aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_LEFT_VOLUME_REG, AIC3212_MICPGA_VOLUME_ENABLE | volume_int);
    } else {
        aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_RIGHT_VOLUME_REG, AIC3212_MICPGA_VOLUME_ENABLE | volume_int);
    }
    return gain_dB;
}

float ESP32AIC3212::setInputGain_dB(float gain_dB)
{
    gain_dB = setInputGain_dB(gain_dB, 0); // left channel
    return setInputGain_dB(gain_dB, 1);    // right channel
}

// Sets digital volume within the ADC processing block; accepts -12~20dB in steps of 0.5
float ESP32AIC3212::setAdcVolume_dB(float vol_dB)
{
    vol_dB = constrain(vol_dB, AIC3212_ADC_VOLUME_MIN, AIC3212_ADC_VOLUME_MAX);
    vol_dB = roundf(2*vol_dB);

    aic_goToBook(AIC3212_ADC_VOLUME_BOOK);
    aic_writePage( AIC3212_ADC_VOLUME_PAGE, AIC3212_ADC_VOLUME_LEFT_REGISTER, AIC3212_ADC_VOLUME_MASK & (int8_t)vol_dB );
    aic_writePage( AIC3212_ADC_VOLUME_PAGE, AIC3212_ADC_VOLUME_RIGHT_REGISTER, AIC3212_ADC_VOLUME_MASK & (int8_t)vol_dB );

    return (vol_dB/2.0f);
}

//******************* OUTPUT  *****************************//
bool ESP32AIC3212::enableAutoMuteDAC(bool enable, uint8_t mute_delay_code)
{
    if (enable)
    {
        mute_delay_code = (uint8_t)constrain((int)mute_delay_code, 0, 7);
        if (mute_delay_code == 0) enable = false;
    }
    else
    {
        mute_delay_code = 0; // this disables the auto mute
    }
    aic_goToBook(0);
    uint8_t val = aic_readPage(0, 64);
    val = val & 0b10001111;             // clear these bits
    val = val | (mute_delay_code << 4); // set these bits
    aic_writePage(0, 64, val);
    return enable;
}

// -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
float ESP32AIC3212::applyLimitsOnVolumeSetting(float vol_dB)
{
    if (vol_dB > 24.0) vol_dB = 24.0;
    if (vol_dB < -63.5) vol_dB = -63.5;
    return vol_dB;
}

float ESP32AIC3212::volume_dB(float orig_vol_dB, int Ichan)
{
    float vol_dB = applyLimitsOnVolumeSetting(orig_vol_dB);
    if (abs(vol_dB - orig_vol_dB) > 0.01)
    {
        Serial.println("ESP32AIC3212: WARNING: Attempting to set DAC Volume outside range");
    }
    int8_t volume_int = (int8_t)(round(vol_dB * 2.0));

    if (debugToSerial)
    {
        Serial.print("ESP32AIC3212: Setting DAC");
        Serial.print(Ichan);
        Serial.print(" volume to ");
        Serial.print(vol_dB, 1);
        Serial.print(".  Converted to volume map => ");
        Serial.println(volume_int);
    }

    aic_goToBook(0);
    if (Ichan == 0) {
        aic_writePage(AIC3212_DAC_VOLUME_PAGE, AIC3212_DAC_VOLUME_LEFT_REG, volume_int);
    } else {
        aic_writePage(AIC3212_DAC_VOLUME_PAGE, AIC3212_DAC_VOLUME_RIGHT_REG, volume_int);
    }
    return vol_dB;
}

float ESP32AIC3212::volume_dB(float vol_left_dB, float vol_right_dB)
{
    volume_dB(vol_right_dB, 1);
    return volume_dB(vol_left_dB, 0);
}

float ESP32AIC3212::volume_dB(float vol_dB)
{
    vol_dB = volume_dB(vol_dB, 1);
    return volume_dB(vol_dB, 0);
}

float ESP32AIC3212::get_volume_dB(int chan) {
    int8_t vol_int = -99;
    
    aic_goToBook(0);
    if (chan == 0) {
            vol_int = aic_readPage(AIC3212_DAC_VOLUME_PAGE, AIC3212_DAC_VOLUME_LEFT_REG);
    } else if (chan == 1) {
            vol_int = aic_readPage(AIC3212_DAC_VOLUME_PAGE, AIC3212_DAC_VOLUME_RIGHT_REG);
    } else {
        return -99.0;
    }
    
    float vol_dB = -99.0;
    if (vol_int > -99) vol_dB = ((float)vol_int)/2.0f;
    return vol_dB;            
}

float ESP32AIC3212::setHeadphoneGain_dB(float gain_left_dB, float gain_right_dB)
{
    unsigned int buff = 0;
    int8_t left_dB_u8 = 0;
    int8_t right_dB_u8 = 0;

    left_dB_u8 = (int)(gain_left_dB + 0.5); 
    right_dB_u8 = (int)(gain_right_dB + 0.5); 

    left_dB_u8 = constrain(left_dB_u8, AIC3212_HP_VOLUME_MIN, AIC3212_HP_VOLUME_MAX);
    right_dB_u8 = constrain(right_dB_u8, AIC3212_HP_VOLUME_MIN, AIC3212_HP_VOLUME_MAX);

    if (gain_left_dB < -90.0) left_dB_u8 = 0b00111001; //mute it
    if (gain_right_dB < -90.0) right_dB_u8 = 0b00111001; //mute it

    aic_goToBook(0);
    buff = aic_readPage(AIC3212_HP_VOLUME_PAGE, AIC3212_HPL_VOLUME_REG); //read existing register value
    buff = (buff & (~AIC3212_HP_VOLUME_MASK)) | (left_dB_u8 & AIC3212_HP_VOLUME_MASK);
    aic_writePage( AIC3212_HP_VOLUME_PAGE, AIC3212_HPL_VOLUME_REG, uint8_t(buff) );

    buff = aic_readPage(AIC3212_HP_VOLUME_PAGE, AIC3212_HPR_VOLUME_REG); //read existing register value
    buff = (buff & (~AIC3212_HP_VOLUME_MASK)) | (right_dB_u8 & AIC3212_HP_VOLUME_MASK);
    aic_writePage( AIC3212_HP_VOLUME_PAGE, AIC3212_HPR_VOLUME_REG, uint8_t(buff) );
    
    return left_dB_u8;
}

float ESP32AIC3212::setSpeakerVolume_dB(float target_vol_dB)
{
    int int_targ_vol_dB = (int)(target_vol_dB + 0.5f);

    uint8_t val = 0x00;
    if (int_targ_vol_dB == 0) {
        val = 0x00;  // Mute (SPR disabled)
    } else if (int_targ_vol_dB == 6) {
        val = 0x11;
    } else if (int_targ_vol_dB == 12) {
        val = 0x22;
    } else if (int_targ_vol_dB == 18) {
        val = 0x33;
    } else if (int_targ_vol_dB == 24) {
        val = 0x44;
    } else if (int_targ_vol_dB == 30) {
        val = 0x55;
    }

    aic_goToBook(0);
    aic_writePage(1, 48, val);     // Speaker volume

    return (float)int_targ_vol_dB;
}

void ESP32AIC3212::setDACVolume_dB(float vol_dB)
{
    // DAC digital volume control (P0_R41/R42)
    // Range: -63.5dB to +24dB in 0.5dB steps
    // Format: signed 8-bit, 0x00 = 0dB, 0x01 = +0.5dB, 0xFF = -0.5dB
    int8_t vol_int = (int8_t)(vol_dB * 2.0f);  // Convert dB to 0.5dB steps

    aic_goToBook(0);
    aic_writePage(0, 0x41, (uint8_t)vol_int);  // Left DAC volume
    aic_writePage(0, 0x42, (uint8_t)vol_int);  // Right DAC volume
}

void ESP32AIC3212::writeBiquadCoefficients(void)
{
    // Helper lambda to scale and clamp coefficients to valid range
    auto scaleAndClamp = [](float b0, float b1, float b2, float a1, float a2,
                            int32_t& n0, int32_t& n1, int32_t& n2, int32_t& d1, int32_t& d2,
                            const char* filterName) {
        // Find maximum coefficient magnitude to determine if scaling is needed
        float abs_b0 = abs(b0);
        float abs_b1 = abs(b1) / 2.0;  // b1 has factor of 2
        float abs_b2 = abs(b2);
        float abs_a1 = abs(a1) / 2.0;  // a1 has factor of 2
        float abs_a2 = abs(a2);

        float max_b = abs_b0;
        if (abs_b1 > max_b) max_b = abs_b1;
        if (abs_b2 > max_b) max_b = abs_b2;

        float max_a = abs_a1;
        if (abs_a2 > max_a) max_a = abs_a2;

        float max_coeff = (max_b > max_a) ? max_b : max_a;

        // Scale factor to bring into range (leave 10% headroom)
        float scale = 1.0;
        if (max_coeff > 0.9) {
            scale = 0.9 / max_coeff;
            Serial.printf("    [%s] Scaling by %.3f to prevent overflow (max=%.3f)\n",
                         filterName, scale, max_coeff);
        }

        // Apply scaling and convert to fixed-point
        // Note: All coefficients stored as 24-bit signed values
        // AIC3212 denominator has MINUS signs: (2^23 - 2*D1*z^-1 - D2*z^-2)
        // So D1 = -a1 * 2^22 and D2 = -a2 * 2^23
        n0 = (int32_t)((b0 * scale) * 8388608.0);
        n1 = (int32_t)((b1 * scale) * 4194304.0);
        n2 = (int32_t)((b2 * scale) * 8388608.0);
        d1 = (int32_t)((-a1 * scale) * 4194304.0);  // NEGATE a1
        d2 = (int32_t)((-a2 * scale) * 8388608.0);  // NEGATE a2

        // Clamp ALL coefficients to 24-bit signed range
        n0 = constrain(n0, -8388607, 8388607);
        n1 = constrain(n1, -8388607, 8388607);
        n2 = constrain(n2, -8388607, 8388607);
        d1 = constrain(d1, -8388607, 8388607);
        d2 = constrain(d2, -8388607, 8388607);
    };

    // FILTER 1: 1st-order High-pass @ 20Hz (DC blocking only; preserves all audible content)
    Serial.println("  Filter 1: 1st-order High-pass @ 20Hz");

    float fc_hp = 20.0;
    float fs = 48000.0;
    float wc = 2.0 * M_PI * fc_hp / fs;
    float K = tan(wc / 2.0);

    // 1st-order high-pass Butterworth (b2=0, a2=0)
    float b0_hp = 1.0 / (1.0 + K);
    float b1_hp = -b0_hp;  // -1/(1+K)
    float b2_hp = 0.0;     // 1st-order: no z^-2 term
    float a1_hp = (K - 1.0) / (K + 1.0);
    float a2_hp = 0.0;     // 1st-order: no z^-2 term

    int32_t n0_1, n1_1, n2_1, d1_1, d2_1;
    scaleAndClamp(b0_hp, b1_hp, b2_hp, a1_hp, a2_hp, n0_1, n1_1, n2_1, d1_1, d2_1, "HPF");

    Serial.printf("    N0=0x%06X, N1=0x%06X, N2=0x%06X, D1=0x%06X, D2=0x%06X\n",
                  n0_1 & 0xFFFFFF, n1_1 & 0xFFFFFF, n2_1 & 0xFFFFFF,
                  d1_1 & 0xFFFFFF, d2_1 & 0xFFFFFF);

    // FILTER 2: Peaking @ 4500Hz, +6dB, BW=2000Hz → Q=2.25
    Serial.println("  Filter 2: Peaking @ 4500Hz, +6dB, Q=2.25");

    float fc_pk1 = 4500.0;
    float gain_pk1 = 6.0;  // dB
    float Q_pk1 = fc_pk1 / 2000.0;  // Q = fc / BW = 4500 / 2000 = 2.25

    float A_pk1 = pow(10.0, gain_pk1 / 40.0);
    float w0_pk1 = 2.0 * M_PI * fc_pk1 / fs;
    float alpha_pk1 = sin(w0_pk1) / (2.0 * Q_pk1);

    float b0_pk1 = 1.0 + alpha_pk1 * A_pk1;
    float b1_pk1 = -2.0 * cos(w0_pk1);
    float b2_pk1 = 1.0 - alpha_pk1 * A_pk1;
    float a0_pk1 = 1.0 + alpha_pk1 / A_pk1;
    float a1_pk1 = -2.0 * cos(w0_pk1);
    float a2_pk1 = 1.0 - alpha_pk1 / A_pk1;

    // Normalize
    b0_pk1 /= a0_pk1; b1_pk1 /= a0_pk1; b2_pk1 /= a0_pk1;
    a1_pk1 /= a0_pk1; a2_pk1 /= a0_pk1;

    int32_t n0_2, n1_2, n2_2, d1_2, d2_2;
    scaleAndClamp(b0_pk1, b1_pk1, b2_pk1, a1_pk1, a2_pk1, n0_2, n1_2, n2_2, d1_2, d2_2, "Peak4.5k");

    Serial.printf("    N0=0x%06X, N1=0x%06X, N2=0x%06X, D1=0x%06X, D2=0x%06X\n",
                  n0_2 & 0xFFFFFF, n1_2 & 0xFFFFFF, n2_2 & 0xFFFFFF,
                  d1_2 & 0xFFFFFF, d2_2 & 0xFFFFFF);

    // FILTER 3: Peaking @ 8000Hz, -3dB, BW=1000Hz → Q=8.0
    Serial.println("  Filter 3: Peaking @ 8000Hz, -3dB, Q=8.0");

    float fc_pk2 = 8000.0;
    float gain_pk2 = -3.0;  // dB (cut)
    float Q_pk2 = fc_pk2 / 1000.0;  // Q = fc / BW = 8000 / 1000 = 8.0

    float A_pk2 = pow(10.0, gain_pk2 / 40.0);
    float w0_pk2 = 2.0 * M_PI * fc_pk2 / fs;
    float alpha_pk2 = sin(w0_pk2) / (2.0 * Q_pk2);

    float b0_pk2 = 1.0 + alpha_pk2 * A_pk2;
    float b1_pk2 = -2.0 * cos(w0_pk2);
    float b2_pk2 = 1.0 - alpha_pk2 * A_pk2;
    float a0_pk2 = 1.0 + alpha_pk2 / A_pk2;
    float a1_pk2 = -2.0 * cos(w0_pk2);
    float a2_pk2 = 1.0 - alpha_pk2 / A_pk2;

    // Normalize
    b0_pk2 /= a0_pk2; b1_pk2 /= a0_pk2; b2_pk2 /= a0_pk2;
    a1_pk2 /= a0_pk2; a2_pk2 /= a0_pk2;

    int32_t n0_3, n1_3, n2_3, d1_3, d2_3;
    scaleAndClamp(b0_pk2, b1_pk2, b2_pk2, a1_pk2, a2_pk2, n0_3, n1_3, n2_3, d1_3, d2_3, "Peak8k");

    Serial.printf("    N0=0x%06X, N1=0x%06X, N2=0x%06X, D1=0x%06X, D2=0x%06X\n",
                  n0_3 & 0xFFFFFF, n1_3 & 0xFFFFFF, n2_3 & 0xFFFFFF,
                  d1_3 & 0xFFFFFF, d2_3 & 0xFFFFFF);

    // Switch to Book 80 for biquad coefficient access
    aic_goToBook(80);
    aic_goToPage(0);

    Serial.println("  Writing calculated biquad EQ filters to Buffer A (pages 1-2):");

    // Helper lambda to write a biquad's 5 coefficients
    auto writeBiquad = [this](int biquadNum, int32_t n0, int32_t n1, int32_t n2, int32_t d1, int32_t d2,
                              int leftPage, int rightPage) {
        int leftBase = 12 + (biquadNum * 20);
        int rightBase = 20 + (biquadNum * 20);

        // Mask to 24 bits
        uint32_t n0_24 = n0 & 0xFFFFFF;
        uint32_t n1_24 = n1 & 0xFFFFFF;
        uint32_t n2_24 = n2 & 0xFFFFFF;
        uint32_t d1_24 = d1 & 0xFFFFFF;
        uint32_t d2_24 = d2 & 0xFFFFFF;

        // Left channel
        aic_goToPage(leftPage);
        aic_writePage(leftPage, leftBase,      (n0_24 >> 16) & 0xFF);
        aic_writePage(leftPage, leftBase + 1,  (n0_24 >> 8) & 0xFF);
        aic_writePage(leftPage, leftBase + 2,  n0_24 & 0xFF);
        aic_writePage(leftPage, leftBase + 4,  (n1_24 >> 16) & 0xFF);
        aic_writePage(leftPage, leftBase + 5,  (n1_24 >> 8) & 0xFF);
        aic_writePage(leftPage, leftBase + 6,  n1_24 & 0xFF);
        aic_writePage(leftPage, leftBase + 8,  (n2_24 >> 16) & 0xFF);
        aic_writePage(leftPage, leftBase + 9,  (n2_24 >> 8) & 0xFF);
        aic_writePage(leftPage, leftBase + 10, n2_24 & 0xFF);
        aic_writePage(leftPage, leftBase + 12, (d1_24 >> 16) & 0xFF);
        aic_writePage(leftPage, leftBase + 13, (d1_24 >> 8) & 0xFF);
        aic_writePage(leftPage, leftBase + 14, d1_24 & 0xFF);
        aic_writePage(leftPage, leftBase + 16, (d2_24 >> 16) & 0xFF);
        aic_writePage(leftPage, leftBase + 17, (d2_24 >> 8) & 0xFF);
        aic_writePage(leftPage, leftBase + 18, d2_24 & 0xFF);

        // Right channel
        aic_goToPage(rightPage);
        aic_writePage(rightPage, rightBase,      (n0_24 >> 16) & 0xFF);
        aic_writePage(rightPage, rightBase + 1,  (n0_24 >> 8) & 0xFF);
        aic_writePage(rightPage, rightBase + 2,  n0_24 & 0xFF);
        aic_writePage(rightPage, rightBase + 4,  (n1_24 >> 16) & 0xFF);
        aic_writePage(rightPage, rightBase + 5,  (n1_24 >> 8) & 0xFF);
        aic_writePage(rightPage, rightBase + 6,  n1_24 & 0xFF);
        aic_writePage(rightPage, rightBase + 8,  (n2_24 >> 16) & 0xFF);
        aic_writePage(rightPage, rightBase + 9,  (n2_24 >> 8) & 0xFF);
        aic_writePage(rightPage, rightBase + 10, n2_24 & 0xFF);
        aic_writePage(rightPage, rightBase + 12, (d1_24 >> 16) & 0xFF);
        aic_writePage(rightPage, rightBase + 13, (d1_24 >> 8) & 0xFF);
        aic_writePage(rightPage, rightBase + 14, d1_24 & 0xFF);
        aic_writePage(rightPage, rightBase + 16, (d2_24 >> 16) & 0xFF);
        aic_writePage(rightPage, rightBase + 17, (d2_24 >> 8) & 0xFF);
        aic_writePage(rightPage, rightBase + 18, d2_24 & 0xFF);
    };

    // PRB_P1 has 3 biquads (A, B, C) in signal path.
    // Biquad A: HPF @ 700 Hz — removes DC and sub-bass from the digital signal.
    //   The HP output is DC-coupled (charge pump, no output capacitor); any DC in the
    //   audio stream flows continuously through the headphone load and trips SC protection
    //   after ~1 second.  The HPF eliminates this at the source.
    // Biquads B and C: unity (flat) — no gain boost that could push DSP above 0dBFS.
    const int32_t unity = 0x7FFFFF;  // 24-bit maximum ≈ 1.0 in fixed-point
    const int32_t zero  = 0x000000;

    Serial.println("    Biquad A: HPF @ 700 Hz (DC blocking); Biquad B/C: flat");
    writeBiquad(0, n0_1, n1_1, n2_1, d1_1, d2_1, 1, 2);  // Buffer A: pages 1 & 2
    writeBiquad(1, unity, zero, zero, zero, zero, 1, 2);
    writeBiquad(2, unity, zero, zero, zero, zero, 1, 2);

    // WRITE TO BOTH BUFFERS to ensure coefficients are loaded regardless of which is active
    Serial.println("  Writing same coefficients to Buffer B (pages 9-10)...");
    writeBiquad(0, n0_1, n1_1, n2_1, d1_1, d2_1, 9, 10);  // Buffer B: pages 9 & 10
    writeBiquad(1, unity, zero, zero, zero, zero, 9, 10);
    writeBiquad(2, unity, zero, zero, zero, zero, 9, 10);

    // VERIFY coefficients were written - read back first coefficient of Biquad A
    aic_goToBook(80);
    aic_goToPage(1);
    uint8_t n0_msb = aic_readPage(1, 12);
    uint8_t n0_mid = aic_readPage(1, 13);
    uint8_t n0_lsb = aic_readPage(1, 14);
    uint32_t n0_readback = (n0_msb << 16) | (n0_mid << 8) | n0_lsb;
    Serial.printf("  VERIFY: Biquad A N0 written=0x%06X, read=0x%06X %s\n",
                  (uint32_t)(n0_1 & 0xFFFFFF), n0_readback,
                  (n0_readback == (n0_1 & 0xFFFFFF)) ? "[OK]" : "[MISMATCH!]");

    // Enable adaptive mode (DAC is off, so coefficients can be written)
    aic_goToBook(80);
    aic_goToPage(0);
    aic_writePage(0, 0x01, 0x04);  // D2=1: Enable adaptive mode

    uint8_t status = aic_readPage(0, 0x01);
    Serial.printf("  Adaptive mode enabled: %s\n", (status & 0x04) ? "YES" : "NO");

    // Return to Book 0
    aic_goToBook(0);
    aic_goToPage(0);

    // CRITICAL: Verify PRB_P1 is selected in DAC path
    uint8_t prb_check = aic_readPage(0, 0x3C);
    Serial.printf("  Verification: PRB register (P0_R60) = 0x%02X ", prb_check);
    Serial.println((prb_check & 0x1F) == 0x01 ? "[PRB_P1 active]" : "[WARNING: PRB_P1 NOT active!]");

    Serial.println("  Biquad EQ coefficients ready - will be active when DAC powers up");
}

int ESP32AIC3212::enableHeadphonePower(int chan, bool enable) {
    int ret_val = -1;

    aic_goToBook(0);
    uint8_t cur_val = aic_readPage(1,27);

    if ((chan == AIC3212_BOTH_CHAN) || (chan == AIC3212_LEFT_CHAN)) {
        if (enable) cur_val = cur_val | 0b00000010;
        else cur_val = cur_val & 0b11111101;
        ret_val = chan;
    }
    if ((chan == AIC3212_BOTH_CHAN) || (chan == AIC3212_RIGHT_CHAN)) {
        if (enable) cur_val = cur_val | 0b00000001;
        else cur_val = cur_val & 0b11111110;
        ret_val = chan;
    }

    Serial.print("ESP32AIC3212: enableHeadphonePower: sending ");
    Serial.println(cur_val,BIN);
    aic_writePage(1,27,cur_val);
    cur_val = aic_readPage(1,27);
    Serial.print("ESP32AIC3212: enableHeadphonePower: confirming, received ");
    Serial.println(cur_val,BIN);
    return ret_val;
}

void ESP32AIC3212::enableHeadsetDetection(void) {
    aic_goToBook(0);
    // B0_P0_R67: D7=1 (enable), D[4:2]=010 (64ms insertion debounce), D[1:0]=01 (8ms button
    // debounce).  Used for continuous monitoring in speaker mode.  Detection is kept OFF in
    // HP mode: a TRS plug's sleeve bridges the mic network (MICBIAS_EXT/MICDET) into the
    // Ring2/HPVSS_SENSE ground-sense node, so detection bias must not be live while the HP
    // drivers carry audio.  HP-mode checks use checkHeadsetPresence() instead.
    aic_writePage(0, 0x43, 0x89);
}

void ESP32AIC3212::disableHeadsetDetection(void) {
    aic_goToBook(0);
    aic_writePage(0, 0x43, 0x00);  // P0_R67: D7=0 disables detection (MICBIAS_EXT is permanently off via P1_R51=0x80)
}

bool ESP32AIC3212::isHeadsetInserted(void) {
    aic_goToBook(0);
    // Insertion = R46 D4 status flag OR R37 D[1:0] headphone-load classification.
    // With MICBIAS_EXT permanently disabled (P1_R51=0x80 — headset mic unused), it is
    // undocumented whether the R46 D4 flag still fires without the MICDET measurement,
    // so accept either indicator.  R37 D[1:0] != 00 = mono/stereo headset load detected
    // by the bias-independent HP-pin probe (valid in ground-centered mode, SLAU360 5.2.33).
    uint8_t r46 = (uint8_t)aic_readPage(0, 0x2E);
    uint8_t r37 = (uint8_t)aic_readPage(0, 0x25);
    return ((r46 & 0x10) != 0) || ((r37 & 0x03) != 0);
}

bool ESP32AIC3212::checkHeadsetPresence(bool currently_using_hp) {
    // JIT insertion check — called once at the start of every playback request, BEFORE any
    // audio flows.  SLAU360 2.12.2: P0_R46 D4 is the instantaneous insertion state behind
    // the detection debounce; it is only meaningful while detection is enabled and after at
    // least one debounce period has elapsed.  (A read taken immediately after enabling
    // detection returns the reset value, NOT the real state — that bug caused the output
    // to flip to speaker with headphones still inserted.)
    if (!currently_using_hp) {
        // Speaker mode: detection has been running continuously since outputSelect(Spk)
        // re-enabled it, so the status read is immediately valid.  Zero added latency.
        aic_goToBook(0);
        uint8_t r46 = (uint8_t)aic_readPage(0, 0x2E);
        uint8_t r37 = (uint8_t)aic_readPage(0, 0x25);
        bool inserted = ((r46 & 0x10) != 0) || ((r37 & 0x03) != 0);
        Serial.printf("[HS-CHECK] SPK mode: R46=0x%02X R37=0x%02X -> %s\n",
                      r46, r37, inserted ? "INSERTED" : "not inserted");
        Serial.flush();  // critical diagnostic — don't let TX buffer overflow mangle it
        return inserted;
    }
    // HP mode: detect -> reset -> (caller switches if changed) -> play.
    //
    // The detection engine's load probe is the only way to sense removal, but probing
    // with powered drivers under the internal-ground common mode wedges the HP analog
    // stage (proven: with the probe, every following playback was silent; without it,
    // everything played).  A reboot's aic_init heals the wedge, so after a probe that
    // confirms the headset is still inserted, re-assert the analog common-mode /
    // charge-pump configuration the probe disturbs, BEFORE any audio flows.
    // If the headset was removed, no heal is needed — the caller runs a full
    // outputSelect(speaker) which powers the HP drivers down anyway.
    aic_goToBook(0);
    aic_writePage(0, 0x43, 0x81);   // D7=1 enable, D[4:2]=000 (16ms debounce), D[1:0]=01
    delay(25);                      // > one 16ms debounce period
    uint8_t r46 = (uint8_t)aic_readPage(0, 0x2E);
    uint8_t r37 = (uint8_t)aic_readPage(0, 0x25);
    aic_writePage(0, 0x43, 0x00);   // detection back off for HP mode
    bool inserted = ((r46 & 0x10) != 0) || ((r37 & 0x03) != 0);
    Serial.printf("[HS-CHECK] HP mode: R46=0x%02X R37=0x%02X -> %s\n",
                  r46, r37, inserted ? "INSERTED" : "removed");
    Serial.flush();  // critical diagnostic — don't let TX buffer overflow mangle it
    if (inserted) {
        // HEAL: re-assert the analog config (same values, same order as aic_init).
        aic_writePage(1, 0x08, 0x03);  // common mode
        aic_writePage(1, 0x23, 0x31);  // charge pump forced on + dynamic offset cal
        aic_writePage(1, 0x22, 0xBE);  // internal-ground CM + offset correction
        delay(5);                      // let the analog stage settle before audio
    }
    return inserted;
}

void ESP32AIC3212::diagHP(void) {
    aic_goToBook(0);
    // Snapshot of all registers that control the HP output path
    uint8_t p1r09 = (uint8_t)aic_readPage(1, 0x09);  // HP driver sizing / OC config
    uint8_t p1r1B = (uint8_t)aic_readPage(1, 0x1B);  // HP driver power + routing (P1_R27)
    uint8_t p1r1F = (uint8_t)aic_readPage(1, 0x1F);  // HPL volume / GC mode / mute (P1_R31)
    uint8_t p1r20 = (uint8_t)aic_readPage(1, 0x20);  // HPR volume (P1_R32)
    uint8_t p1r23 = (uint8_t)aic_readPage(1, 0x23);  // Charge pump config (P1_R35)
    uint8_t p0r3F = (uint8_t)aic_readPage(0, 0x3F);  // DAC channel power (P0_R63)
    uint8_t p0r40 = (uint8_t)aic_readPage(0, 0x40);  // DAC mute / auto-mute (P0_R64)
    // Sticky interrupt flags — cleared on read; capture HP SC/OC events
    uint8_t p0r2C = (uint8_t)aic_readPage(0, 0x2C);  // P0_R44: sticky interrupt flags 1
    uint8_t p0r2D = (uint8_t)aic_readPage(0, 0x2D);  // P0_R45: sticky interrupt flags 2
    uint8_t p0r2E = (uint8_t)aic_readPage(0, 0x2E);  // P0_R46: real-time flags 1 (headset/HP status)
    uint8_t p0r2F = (uint8_t)aic_readPage(0, 0x2F);  // P0_R47: real-time flags 2
    Serial.printf("[HP-DIAG] P1:R09=%02X R1B=%02X R1F=%02X R20=%02X R23=%02X | P0:R3F=%02X R40=%02X | FLAGS:R2C=%02X R2D=%02X R2E=%02X R2F=%02X\n",
                  p1r09, p1r1B, p1r1F, p1r20, p1r23, p0r3F, p0r40, p0r2C, p0r2D, p0r2E, p0r2F);
}

uint8_t ESP32AIC3212::readP1R1B(void) {
    aic_goToBook(0);
    return (uint8_t)aic_readPage(1, 0x1B);
}

uint8_t ESP32AIC3212::readP0R43(void) {
    aic_goToBook(0);
    return (uint8_t)aic_readPage(0, 0x43);
}

uint8_t ESP32AIC3212::readSCFlags(void) {
    // Reads and clears the sticky HP SC flags in P0_R44.
    // D7=HPL SC, D6=HPR SC.  Returns 0x00 if no SC event has occurred since last read.
    aic_goToBook(0);
    return (uint8_t)aic_readPage(0, 0x2C);
}

void ESP32AIC3212::setDACMute(bool mute) {
    // Bracket HP driver transitions and i2s_set_clk() calls with mute/unmute to suppress pops.
    aic_goToBook(0);
    aic_writePage(0, 0x40, mute ? 0x0C : 0x00);
}

bool ESP32AIC3212::outputSelect(AIC_Output left, AIC_Output right, bool flag_full)
{
    if (debugToSerial) Serial.println("# ESP32AIC3212: outputSelect");

    if (firstTime_outputSelect) {
        flag_full = true;
        firstTime_outputSelect = false;
    }

    aic_goToBook(0);

    // === MUTE DAC FIRST TO PREVENT POPS ===
    aic_writePage(0, 0x40, 0x0C);  // Mute both DAC channels

    if (flag_full) {
        aic_writePage(1, 0x1F, 0xB9);  // HPL mute
        aic_writePage(1, 0x20, 0xB9);  // HPR mute
        aic_writePage(1, 0x16, 0x00);  // Disable LOL/LOR routing
        aic_writePage(1, 0x1B, 0x00);  // Disable HPL/HPR routing
        aic_writePage(1, 0x10, 0x00);  // Mute LOL output
        aic_writePage(1, 0x12, 0x00);  // Mute LOR output
    }

    // === CLEAR ALL OUTPUTS (AIC3212 registers only) ===
    aic_writePage(1, 0x2C, 0x00);  // HP driver power off
    aic_writePage(1, 0x1F, 0xB9);  // Mute HPL
    aic_writePage(1, 0x20, 0xB9);  // Mute HPR
    aic_writePage(1, 0x10, 0x00);  // Mute LOL
    aic_writePage(1, 0x12, 0x00);  // Mute LOR
    aic_writePage(1, 0x2D, 0x00);  // Power down SPKL and SPKR drivers
    aic_writePage(1, 0x2F, 0x3F);  // Mute SPKR amp (LOR→SPKR path, reset default)

    uint8_t p1_r16 = 0x00, p1_r1B = 0x00, p1_r1F = 0xB9, p1_r20 = 0xB9;
    uint8_t p1_r10 = 0x00, p1_r12 = 0x00;  // Line output volumes

    bool use_output = false;

    // === RIGHT SPEAKER ONLY ===
    if (right == Aic_Output_Spk) {
        p1_r16 = 0x43;       // DAC_R → LOR, power up LOL/LOR drivers
        aic_writePage(1, 0x2D, 0x03);  // Power SPKL (D1) and SPKR (D0)
        aic_writePage(1, 0x2F, 0x00);  // Route LOR to SPKR @ 0dB (per TI reference design)
        use_output = true;
    }

    // === HEADPHONES (DUAL-MONO) ===
    if (left == Aic_Output_Hp || right == Aic_Output_Hp) {
        p1_r1B |= 0x20;      // DAC_L → HPL
        p1_r1F = 0x80;       // Unmute HPL
        p1_r1B |= 0x10;      // DAC_R → HPR
        p1_r20 = 0x80;       // Unmute HPR
        // Note: P1_R44 (0x2C) is a reserved register — no write needed here
        use_output = true;
    }

    bool using_headphones = (left == Aic_Output_Hp || right == Aic_Output_Hp);

    // === POWER UP DACs AND CONFIGURE ANALOG PATH ===
    if (use_output) {
        // STEP 1: Set DAC PowerTune Mode BEFORE powering up DAC
        aic_writePage(1, 0x03, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2);  // Left DAC PTM
        aic_writePage(1, 0x04, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2);  // Right DAC PTM

        // STEP 2: Route DAC outputs and configure analog path
        aic_writePage(1, 0x16, p1_r16);  // Line out routing/power
        aic_writePage(1, 0x1B, p1_r1B);  // Headphone routing (initial)

        // STEP 3: Power up LDAC and RDAC
        uint8_t dac_power = ((left == Aic_Output_None) ? 0x00 : 0x80)
                          | ((right == Aic_Output_None) ? 0x00 : 0x40);
        aic_writePage(0, 0x3F, dac_power);

        // STEP 4: Set volume/mute registers
        aic_writePage(1, 0x1F, p1_r1F);  // HPL gain/mute
        aic_writePage(1, 0x20, p1_r20);  // HPR gain/mute
        aic_writePage(1, 0x10, p1_r10);  // LOL volume
        aic_writePage(1, 0x12, p1_r12);  // LOR volume

        // STEP 5: Headphone-specific power-up sequence (if using headphones)
        if (using_headphones) {
            // Disable headset detection before powering HP drivers.
            // A TRS plug's sleeve bridges the mic network (MICBIAS_EXT/MICDET) into the
            // Ring2/HPVSS_SENSE ground-sense node, so detection bias must not be live
            // while the HP drivers are active.
            disableHeadsetDetection();
            // Calculate headphone driver power bits
            uint8_t hp_power = 0x00;
            if (left  == Aic_Output_Hp) hp_power |= 0x22;  // HPL routing (D5) + driver (D1)
            if (right == Aic_Output_Hp) hp_power |= 0x11;  // HPR routing (D4) + driver (D0)
            // P1_R09: D[6:5]=00 (100% output stage), D4=1 (reserved, reset value), D[3:1]=100
            // (64µs SC glitch rejection), D0=0 — on over-current the driver LIMITS current and
            // keeps running instead of latching itself off.  Matches TI's ground-centered
            // example scripts (SLAU360 4.1.x) and eliminates the permanently-silenced-output
            // failure mode that required register save/restore recovery machinery.
            aic_writePage(1, 0x09, 0x18);
            // Routing (STEP 2), gain and unmute (STEP 4) are already configured — per
            // SLAU360 2.3.3.4 everything must be set BEFORE driver power-up.  Power up last:
            aic_writePage(1, 0x1B, hp_power);
            // Offset calibration runs automatically right after EVERY HP power-up and no
            // signal may be applied until it completes (SLAU360 2.3.3.4) — audio injected
            // into the cal window produces clicks/truncated audio/pops.  TI does not publish
            // the cal duration; 300ms is a conservative wait, paid only on insertion switches.
            // (A previous first-power-up-only optimization caused intermittent TRRS sine
            // failures when audio started ~10ms after a re-power.)
            delay(300);
        }

        // STEP 6: Unmute LDAC and RDAC
        aic_writePage(0, 0x40, ((left == Aic_Output_None) ? 0x08 : 0x00)
                             | ((right == Aic_Output_None) ? 0x04 : 0x00));

        // Verification
        Serial.println("\n=== Output Configuration Complete ===");
        Serial.print("DAC Power (P0 R0x3F): 0x");
        Serial.println(dac_power, HEX);
        if (using_headphones) {
            Serial.print("HP Driver/Routing (P1 R0x1B): 0x");
            Serial.println(aic_readPage(1, 0x1B), HEX);
            Serial.print("HPL Vol/GC/Mute  (P1 R0x1F): 0x");
            Serial.println(aic_readPage(1, 0x1F), HEX);
            Serial.print("HPR Vol          (P1 R0x20): 0x");
            Serial.println(aic_readPage(1, 0x20), HEX);
            Serial.print("DAC Mute         (P0 R0x40): 0x");
            Serial.println(aic_readPage(0, 0x40), HEX);
        } else {
            Serial.print("Line Out Routing (P1 R0x16): 0x");
            Serial.println(aic_readPage(1, 0x16), HEX);
            Serial.print("Speaker Amplifier Control (P1 R0x2D): 0x");
            Serial.println(aic_readPage(1, 0x2D), HEX);
            Serial.print("Speaker Gain/Mute (P1 R0x2F): 0x");
            Serial.println(aic_readPage(1, 0x2F), HEX);
        }
    } else {
        // No output selected - ensure DAC is off
        aic_writePage(0, 0x3F, 0x00);
    }

    // Re-enable headset detection whenever HP output is not active,
    // so insertion can be detected again.
    if (!using_headphones) {
        enableHeadsetDetection();
    }

    return true;
}

// Mute / unmute line out
void ESP32AIC3212::muteLineOut(bool flag)
{
    aic_goToBook(0);
    byte curValL = aic_readPage(1, 18);
    byte curValR = aic_readPage(1, 19);

    aic_goToPage(1);
    if (flag == true)
    {
        if (curValL & 0b01000000) aic_writeRegister(18, curValL & 0b10111111);
        if (curValR & 0b01000000) aic_writeRegister(19, curValR & 0b10111111);
    }
    else
    {
        if (!(curValL & 0b01000000)) aic_writeRegister(18, curValL | 0b0100000000);
        if (!(curValR & 0b01000000)) aic_writeRegister(19, curValR | 0b0100000000);
    }
}

void ESP32AIC3212::aic_init()
{
    if (debugToSerial) Serial.println("# ESP32AIC3212: Initializing AIC");
    if (debugToSerial) {
        Serial.print("# DEBUG: pConfig->i2s_bits = ");
        Serial.println(static_cast<uint8_t>(pConfig->i2s_bits));
    }
    aic_goToBook(0);
    aic_writePage(1, 0x01, 0x00);
    aic_writePage(1, 0x7a, 0x01);
    aic_writePage(1, 0x79, 0x33);
    aic_writePage(1, 0x08, 0x03);  // Common mode: D[4:3]=00 required for ground-centered HP (SLAU360), D[1:0]=11 (1.65V REC CM)
    aic_writePage(1, 0x23, 0x31);  // P1_R35: D5=1 (DC offset calibration ON — required per SLAU360 for ground-centered HP mode; without it DC offset builds on HPL/HPR over ~1s causing driver overcurrent), D4=1 (reserved, must write 1), D[1:0]=01 (forced CP power-up — VNEG stable before HP drivers activate)
    aic_writePage(1, 0x22, 0xBE);  // P1_R34: D[7:6]=10 (INTERNAL GROUND sets the output common-mode, SLAU360 5.3.27 — NOT the sense pin: on this board Ring2/HPVSS_SENSE has no DC tie to board ground, so with a TRRS plug the node floats, the servo chases it, DC builds across the earpieces until idle OC events fire [seen as sticky R2C=0xC0] and the output rails silent; TRS plugs are immune only because they short the mic network's pin clamps onto the node), D[4:2]=111 (1x CP peak current), D[1:0]=10 (DC offset correction enabled for selected routings per TI GC script SLAU360 4.7.1)
    aic_writePage(1, 0x77, 0x95);  // P1_R119: headset-detect probe tuning, default values except D0=1 — do NOT auto-slow the probe pulse period 16x when a mic is detected (SLAU360 5.3.59); the slowed probing falls into the audio band and is audible as a whine in TRRS headsets while detection runs
    aic_writePage(1, 0x33, 0x80);  // P1_R51: D7=1, D6=0 — MICBIAS_EXT permanently OFF, detached from jack/mic-detect automation (SLAU360 5.3.42). On this hardware the bias return path is THROUGH the headset mic into Ring2 = HPVSS_SENSE; when the engine auto-powered MICBIAS_EXT on mic detection (TRRS only), the DC into the ground-sense node wedged the ground-centered HP servo. Headset mic is unused by design (per David, June 2026).

    // Clock config
    aic_writePage(0, 0x04, static_cast<uint8_t>(pConfig->dac.clk_src) << 4 | static_cast<uint8_t>(pConfig->adc.clk_src));
    if (pConfig->pll.enabled)
    {
        aic_writePage(0, 0x05, static_cast<uint8_t>(pConfig->pll.range) | static_cast<uint8_t>(pConfig->pll.src) << 2);
        aic_writePage(0, 0x06, 0x80 | ((pConfig->pll.p & 0x07) << 4) | (pConfig->pll.r & 0x0F));
        aic_writePage(0, 0x07, pConfig->pll.j & 0x3F);
        aic_writePage(0, 0x08, (pConfig->pll.d >> 8) & 0x3F);
        aic_writePage(0, 0x09, pConfig->pll.d & 0xFF);
        aic_writePage(0, 0x0A, pConfig->pll.clkin_div & 0x7F);
    }
    else
    {
        aic_writePage(0, 0x06, 0x00);
    }

    // DAC Clock
    aic_writePage(0, 0x0B, (pConfig->dac.ndac != 0 ? 0x80 : 0x00) | (pConfig->dac.ndac & 0x7F));
    aic_writePage(0, 0x0C, (pConfig->dac.mdac != 0 ? 0x80 : 0x00) | (pConfig->dac.mdac & 0x7F));
    aic_writePage(0, 0x0D, (pConfig->dac.dosr >> 8) & 0x03);
    aic_writePage(0, 0x0E, pConfig->dac.dosr & 0xFF);

    // ADC Clock
    aic_writePage(0, 0x12, (pConfig->adc.nadc != 0 ? 0x80 : 0x00) | (pConfig->adc.nadc & 0x7F));
    aic_writePage(0, 0x13, (pConfig->adc.madc != 0 ? 0x80 : 0x00) | (pConfig->adc.madc & 0x7F));
    aic_writePage(0, 0x14, pConfig->adc.aosr & 0xFF);

    // ASI1 routing and I2S configuration
    // aic_writePage(4, 0x01, (static_cast<uint8_t>(pConfig->i2s_bits) << 3));
    uint8_t word_len = static_cast<uint8_t>(pConfig->i2s_bits) << 3;
    if (debugToSerial) {
        Serial.print("# DEBUG: Writing P4.R1 = 0x");
        Serial.println(word_len, HEX);
    }
    aic_writePage(4, 0x01, word_len);
    if (pConfig->i2s_clk_dir == I2S_Clock_Dir::AIC_INPUT)
    {
        aic_writePage(4, 0x0A, 0x00);
    }
    else
    {
        aic_writePage(4, 0x0A, 0x24);
    }
    aic_writePage(4, 0x41, 0x04);
    aic_writePage(4, 0x43, 0x02);
    aic_writePage(4, 0x44, 0x20);  // Enable DIN1 Pin (bit 5 = 1)
    aic_writePage(4, 0x07, 0x02);  // Route ASI1_R to DAC_R for right-only mono speaker output
    aic_writePage(4, 0x08, 0x50);  // ASI1 to DAC datapath (from original Tympan code)
    aic_writePage(4, 0x76, 0x06);

    // CRITICAL: Enable processing block FIRST (required before DAC/ADC power-up)
    aic_writePage(0, 0x3C, pConfig->dac.prb_p & 0x1F);  // Enable PRB_P
    aic_writePage(0, 0x3D, pConfig->adc.prb_r & 0x1F);  // Enable PRB_R (for ADC)

    // === VERIFY CRITICAL REGISTERS IMMEDIATELY ===
    Serial.println("\n=== Verifying critical register writes in aic_init() ===");
    uint8_t verify_val;
    verify_val = (uint8_t)aic_readPage(4, 0x44);
    Serial.print("P4 R0x44 (DIN1 enable): Wrote=0x20, Read=0x");
    Serial.print(verify_val, HEX);
    Serial.println((verify_val & 0x20) ? " [OK - DIN1 enabled]" : " [FAIL]");

    verify_val = (uint8_t)aic_readPage(4, 0x07);
    Serial.print("P4 R0x07 (ASI1_DOUT): Wrote=0x02, Read=0x");
    Serial.print(verify_val, HEX);
    Serial.println(verify_val == 0x02 ? " [OK]" : " [FAIL]");

    verify_val = (uint8_t)aic_readPage(4, 0x08);
    Serial.print("P4 R0x08 (ASI1 to DAC): Wrote=0x50, Read=0x");
    Serial.print(verify_val, HEX);
    Serial.println(verify_val == 0x50 ? " [OK]" : " [FAIL]");

    verify_val = (uint8_t)aic_readPage(0, 0x3C);
    Serial.print("P0 R0x3C (DAC PRB): Wrote=0x");
    Serial.print(pConfig->dac.prb_p & 0x1F, HEX);
    Serial.print(", Read=0x");
    Serial.print(verify_val, HEX);
    Serial.println((verify_val & 0x1F) == (pConfig->dac.prb_p & 0x1F) ? " [OK]" : " [FAIL]");
    Serial.println("=== End verification ===\n");

    // PowerTune Modes (PTM) - set AFTER PRB configuration (per Tympan reference)
    aic_writePage(1, 0x03, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2);  // Left DAC PTM
    aic_writePage(1, 0x04, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2);  // Right DAC PTM
    aic_writePage(1, 0x3D, static_cast<uint8_t>(pConfig->adc.ptm_r) << 6);  // ADC PTM

    // Write biquad coefficients while DAC is powered down (required — locked when DAC runs)
    writeBiquadCoefficients();
}

// I2C read/write helpers using wrapper
unsigned int ESP32AIC3212::aic_readPage(uint8_t page, uint8_t reg)
{
    unsigned int val;

    if (aic_goToPage(page))
    {
        if (!myWire) {
            if (debugToSerial) Serial.println("ESP32AIC3212: ERROR: No I2C interface set");
            return 700;
        }

        myWire->beginTransmission(static_cast<uint8_t>(i2cAddress));
        myWire->write(reg);
        unsigned int result = myWire->endTransmission();
        if (result != 0)
        {
            Serial.print("ESP32AIC3212: ERROR: Read Page.  Page: ");
            Serial.print(page);
            Serial.print(" Reg: ");
            Serial.print(reg);
            Serial.print(".  Received Error During Read Page: ");
            Serial.println(result);
            val = 300 + result;
            return val;
        }
        if (myWire->requestFrom(static_cast<uint8_t>(i2cAddress), 1U) < 1)
        {
            Serial.print("ESP32AIC3212: ERROR: Read Page.  Page: ");
            Serial.print(page);
            Serial.print(" Reg: ");
            Serial.print(reg);
            Serial.println(".  Nothing to return");
            val = 400;
            return val;
        }
        if (myWire->available() >= 1)
        {
            uint16_t v = (uint16_t)myWire->read();
            if (debugToSerial)
            {
                Serial.print("# ESP32AIC3212: Read Page.   Page: ");
                Serial.print(page);
                Serial.print(" Reg: ");
                Serial.print(reg);
                Serial.print(".  Received: 0x");
                Serial.println(v, HEX);
            }
            return v;
        }
    }
    else
    {
        Serial.print("ESP32AIC3212: INFO: Read Page.   Page: ");
        Serial.print(page);
        Serial.print(" Reg: ");
        Serial.print(reg);
        Serial.println(".  Failed to go to read page.  Could not go there.");
        val = 500;
        return val;
    }
    val = 600;
    return val;
}

bool ESP32AIC3212::aic_writePage(uint8_t page, uint8_t reg, uint8_t val)
{
    bool success = true;

    if (debugToSerial)
    {
        Serial.print("# ESP32AIC3212: Write Page.  Page: ");
        Serial.print(page);
        Serial.print(" Reg: ");
        Serial.print(reg);
        Serial.print(" Val: 0x");
        Serial.println(val, HEX);
    }

    success = aic_goToPage(page);
    if (success)
    {
        success = aic_writeRegister(reg, val);
    }
    else
    {
        Serial.print("ESP32AIC3212: Received Error During aic_goToPage()");
    }

    return success;
}

unsigned int ESP32AIC3212::aic_readRegister(uint8_t reg, uint8_t *pVal)
{
    unsigned int errcode = 0;

    if (!myWire) {
        Serial.println("ESP32AIC3212: ERROR: No I2C interface set");
        return 700;
    }

    myWire->beginTransmission(static_cast<uint8_t>(i2cAddress));
    myWire->write(reg);
    unsigned int result = myWire->endTransmission();
    if (result != 0)
    {
        Serial.print("ESP32AIC3212: ERROR: Read register.  Reg: ");
        Serial.print(reg);
        Serial.print(".  Received Error During Write Address: ");
        Serial.println(result);
        errcode = 300 + result;
    }

    if (errcode == 0)
    {
        if (myWire->requestFrom(static_cast<uint8_t>(i2cAddress), 1U) < 1)
        {
            Serial.print("ESP32AIC3212: ERROR: Read register.  Reg: ");
            Serial.print(reg);
            Serial.println(".  Received Error During Request.");
            errcode = 400;
        }
    }

    if (errcode == 0)
    {
        if (myWire->available() >= 1)
        {
            *pVal = (uint8_t)myWire->read();
        }
        else
        {
            Serial.print("ESP32AIC3212: ERROR: Read register.  Reg: ");
            Serial.print(reg);
            Serial.println(".  Empty Response.");
            errcode = 500;
        }
    }

    return errcode;
}

bool ESP32AIC3212::aic_writeRegister(uint8_t reg, uint8_t val)
{ // assumes page has already been set
    if (debugToSerial)
    {
        Serial.print("w ");
        Serial.print(i2cAddress, HEX);
        Serial.print(" ");
        Serial.print(reg, HEX);
        Serial.print(" ");
        Serial.println(val, HEX);
    }

    if (!myWire) {
        Serial.println("ESP32AIC3212: ERROR: No I2C interface set");
        return false;
    }

    myWire->beginTransmission(static_cast<uint8_t>(i2cAddress));
    myWire->write(reg);
    myWire->write(val);
    uint8_t result = myWire->endTransmission();
    if (result == 0)
    {
        return true;
    }
    else
    {
        Serial.print("ESP32AIC3212: Received Error During writeRegister(): Error = ");
        Serial.println(result);
    }
    return false;
}

bool ESP32AIC3212::aic_goToPage(uint8_t page)
{
    return aic_writeRegister(0x00, page);
}

bool ESP32AIC3212::aic_goToBook(uint8_t book)
{
    bool success = true;

    if (debugToSerial)
    {
        Serial.print("# ESP32AIC3212: Go To Book ");
        Serial.print(book);
        Serial.println(".");
    }
    // Go to page 0 of current book
    success = aic_goToPage(0x00);
    if (success)
    {
        // Select book by writing to register 0x7F (Book Select Register)
        success = aic_writeRegister(0x7F, book);
    }
    else
    {
        // Already failed
    }
    return success;
}

// ---------- Biquad & IIR coefficient helper functions (restored) ----------
void ESP32AIC3212::computeBiquadCoeff_LP_f32(float freq_Hz, float sampleRate_Hz, float q, float *coeff)
{
    double w0 = freq_Hz * (2.0 * 3.141592654 / sampleRate_Hz);
    double sinW0 = sin(w0);
    double alpha = sinW0 / ((double)q * 2.0);
    double cosW0 = cos(w0);
    double scale = 1.0 / (1.0 + alpha);
    /* b0 */ coeff[0] = ((1.0 - cosW0) / 2.0) * scale;
    /* b1 */ coeff[1] = (1.0 - cosW0) * scale;
    /* b2 */ coeff[2] = coeff[0];
    /* a0 = 1.0 in Matlab style */
    /* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
    /* a2 */ coeff[4] = (1.0 - alpha) * scale;

    coeff[1] = coeff[1] / 2.0;
    coeff[3] = -coeff[3] / 2.0;
    coeff[4] = -coeff[4];
}

void ESP32AIC3212::computeBiquadCoeff_HP_f32(float freq_Hz, float sampleRate_Hz, float q, float *coeff)
{
    double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
    double sinW0 = sin(w0);
    double alpha = sinW0 / ((double)q * 2.0);
    double cosW0 = cos(w0);
    double scale = 1.0 / (1.0 + alpha);
    coeff[0] = ((1.0 + cosW0) / 2.0) * scale;
    coeff[1] = -(1.0 + cosW0) * scale;
    coeff[2] = coeff[0];
    coeff[3] = (-2.0 * cosW0) * scale;
    coeff[4] = (1.0 - alpha) * scale;

    coeff[1] = coeff[1] / 2.0;
    coeff[3] = -coeff[3] / 2.0;
    coeff[4] = -coeff[4];
}

#define CONST_2_31_m1 (2147483647)

void ESP32AIC3212::convertCoeff_f32_to_i32(float *coeff_f32, int32_t *coeff_i32, int ncoeff)
{
    for (int i = 0; i < ncoeff; i++)
    {
        coeff_f32[i] *= (float)CONST_2_31_m1;
        coeff_i32[i] = (int32_t)coeff_f32[i];
    }
}

int ESP32AIC3212::setBiquadCoeffOnADC(int chanIndex, int biquadIndex, uint32_t *coeff_uint32)
{
    aic_goToBook(0);
    uint32_t prev_state = aic_readPage(0x00, 0x51);
    aic_writePage(0x00, 0x51, prev_state & (0b00111111));

    int page_reg_table[] = {
        8, 36, 9, 44, // N0, start of Biquad A
        8, 40, 9, 48, // N1
        8, 44, 9, 52, // N2
        8, 48, 9, 56, // D1
        8, 52, 8, 64, // D2
        8, 56, 9, 64, // start of biquad B
        8, 60, 9, 68,
        8, 64, 9, 72,
        8, 68, 9, 76,
        8, 72, 9, 80,
        8, 76, 9, 84, // start of Biquad C
        8, 80, 9, 88,
        8, 84, 9, 92,
        8, 88, 9, 96,
        8, 92, 9, 100,
        8, 96, 9, 104, // start of Biquad D
        8, 100, 9, 108,
        8, 104, 9, 112,
        8, 108, 9, 116,
        8, 112, 9, 120,
        8, 116, 9, 124, // start of Biquad E
        8, 120, 10, 8,
        8, 124, 10, 12,
        9, 8, 10, 16,
        9, 12, 10, 20};

    const int rows_per_biquad = 5;
    const int table_ncol = 4;
    int chan_offset;

    switch (chanIndex)
    {
    case Left_Chan:
        chan_offset = 0;
        writeBiquadCoeff(coeff_uint32, page_reg_table + chan_offset + biquadIndex * rows_per_biquad * table_ncol, table_ncol);
        break;
    case Right_Chan:
        chan_offset = 1;
        writeBiquadCoeff(coeff_uint32, page_reg_table + chan_offset + biquadIndex * rows_per_biquad * table_ncol, table_ncol);
        break;
    default:
        return -1;
        break;
    }

    aic_writePage(0x00, 0x51, prev_state);
    return 0;
}

void ESP32AIC3212::writeBiquadCoeff(uint32_t *coeff_uint32, int *page_reg_table, int table_ncol)
{
    int page, reg;
    uint32_t c;
    for (int i = 0; i < 5; i++)
    {
        page = page_reg_table[i * table_ncol];
        reg = page_reg_table[i * table_ncol + 1];
        c = coeff_uint32[i];
        aic_writePage(page, reg, (uint8_t)(c >> 24));
        aic_writePage(page, reg + 1, (uint8_t)(c >> 16));
        aic_writePage(page, reg + 2, (uint8_t)(c >> 8));
    }
    return;
}

void ESP32AIC3212::setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz)
{
    uint32_t coeff[3];
    if (enable)
    {
        HP_cutoff_Hz = cutoff_Hz;
        float coeff_f32[3];
        computeFirstOrderHPCoeff_f32(cutoff_Hz, fs_Hz, coeff_f32);
        convertCoeff_f32_to_i32(coeff_f32, (int32_t *)coeff, 3);
    }
    else
    {
        HP_cutoff_Hz = cutoff_Hz;
        coeff[0] = 0x7FFFFFFF;
        coeff[1] = 0;
        coeff[2] = 0;
    }

    setHpfIIRCoeffOnADC(Left_Chan, coeff);
    setHpfIIRCoeffOnADC(Right_Chan, coeff);
}

void ESP32AIC3212::computeFirstOrderHPCoeff_f32(float cutoff_Hz, float fs_Hz, float *coeff)
{
    const float pi = 3.141592653589793;
    float T = 1.0f / fs_Hz; // sample period
    float w = cutoff_Hz * 2.0 * pi;
    float A = 1.0f / (tan((w * T) / 2.0));
    coeff[0] = A / (1.0 + A);
    coeff[1] = -coeff[0];
    coeff[2] = (1.0 - A) / (1.0 + A);
    coeff[2] = -coeff[2];
}

void ESP32AIC3212::setHpfIIRCoeffOnADC(int chan, uint32_t *coeff)
{
    aic_goToBook(0);
    uint32_t prev_state = aic_readPage(0x00, 0x51);
    aic_writePage(0x00, 0x51, prev_state & (0b00111111));

    if (chan == Left_Chan)
    {
        setHpfIIRCoeffOnADC_Left(coeff);
    }
    else if (chan == Right_Chan)
    {
        setHpfIIRCoeffOnADC_Right(coeff);
    }

    aic_writePage(0x00, 0x51, prev_state);
}

void ESP32AIC3212::setHpfIIRCoeffOnADC_Left(uint32_t *coeff)
{
    uint32_t c;

    aic_goToBook(AIC3212_ADC_IIR_FILTER_BOOK);

    c = coeff[0];
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 24, (uint8_t)(c >> 24));
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 25, (uint8_t)(c >> 16));
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 26, (uint8_t)(c >> 8));

    c = coeff[1];
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 28, (uint8_t)(c >> 24));
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 29, (uint8_t)(c >> 16));
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 30, (uint8_t)(c >> 8));

    c = coeff[2];
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 32, (uint8_t)(c >> 24));
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 33, (uint8_t)(c >> 16));
    aic_writePage(AIC3212_ADC_IIR_FILTER_LEFT_PAGE, 34, (uint8_t)(c >> 8));

    aic_goToBook(0);
}

void ESP32AIC3212::setHpfIIRCoeffOnADC_Right(uint32_t *coeff)
{
    uint32_t c;

    aic_goToBook(AIC3212_ADC_IIR_FILTER_BOOK);

    c = coeff[0];
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 32, (uint8_t)(c >> 24));
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 33, (uint8_t)(c >> 16));
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 34, (uint8_t)(c >> 8));

    c = coeff[1];
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 36, (uint8_t)(c >> 24));
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 37, (uint8_t)(c >> 16));
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 38, (uint8_t)(c >> 8));

    c = coeff[2];
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 40, (uint8_t)(c >> 24));
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 41, (uint8_t)(c >> 16));
    aic_writePage(AIC3212_ADC_IIR_FILTER_RIGHT_PAGE, 42, (uint8_t)(c >> 8));

    aic_goToBook(0);
}

bool ESP32AIC3212::mixInput1toHPout(bool state)
{
    int page = 1;
    int reg;
    uint8_t val;

    aic_goToBook(0);
    for (reg = 12; reg <= 13; reg++)
    { // reg 12 is Left, reg 13 is right
        val = aic_readPage(page, reg);
        if (state == true)
        {                           // activate
            val = val | 0b00000100; // set this bit.  Route IN1L to HPL
        }
        else
        {
            val = val & 0b11111011; // clear this bit.  Un-do routing of IN1L to HPL
        }
        aic_writePage(page, reg, val);
    }
    return state;
}

// ============================================================================
// Digital Microphone Configuration
// Configure PDM digital microphone on DIN2/BCLK2 pins
// ============================================================================
bool ESP32AIC3212::configureDigitalMicOnDIN2(void) {
    if (debugToSerial) Serial.println("ESP32AIC3212: Configuring codec for digital microphone...");

    // ============================================================================
    // IMPORTANT: Do NOT write P0_R27 or P0_R28 here!
    // These are aliases for P4_R1 and P4_R2 which were already configured by
    // aic_init(). Writing to them will overwrite the I2S format and clock settings.
    // ============================================================================

    // IMPORTANT: Do NOT overwrite clock configuration here!
    // P0_R4 (clock source), P0_R18 (NADC), P0_R19 (MADC), P0_R20 (AOSR), and
    // P0_R61 (PRB_R) are already configured by aic_init() from DefaultConfig.
    // These values are carefully calculated to satisfy the PRB_R7 RC constraint:
    //   MADC × AOSR / 32 = RC (must be ≥23 for PRB_R7)
    //
    // DefaultConfig sets:
    //   - PLL enabled: 3.072 MHz (BCLK1) → 36.864 MHz (PLL_CLK)
    //   - ADC uses PLL_CLK, DAC uses PLL_CLK
    //   - NADC=1, MADC=12, AOSR=64, PRB_R=7 (Filter B - full audio bandwidth 0-21.6kHz)
    //   - ADC_MOD_CLK = 36.864 MHz / (1 × 12) = 3.072 MHz (optimal for CMM-3424DT) ✓
    //   - Sample rate = 36.864 MHz / (1 × 12 × 64) = 48 kHz ✓
    //   - RC constraint = 12 × 64 / 32 = 24 (PRB_R7 needs ≥23) ✓
    //   - Digital volume at +20dB (maximum) to compensate for PDM mic's -26dBFS sensitivity

    // Power down ADC briefly to allow IIR coefficient writes
    aic_writePage(0, 0x51, 0x00);  // Power down both ADC channels
    delay(5);

    // ============================================================================
    // OPTIONAL: Configure 1st Order IIR as High-Pass Filter to Remove DC Offset
    // PDM digital microphones can produce DC offset in their output
    // The 1st-order IIR in PRB_R7 can be programmed as HPF to remove DC if needed
    //
    // Signal path: PDM → Decimation Filter → 1st Order IIR → AGC → Volume → Output
    //
    // NOTE: Initially DISABLED to verify if DC offset is actually present.
    // Enable only if measurements show significant DC component in output.
    // Unnecessary filtering can add noise and reduce dynamic range.
    //
    // Transfer function: H(z) = (N0 + N1*z^-1) / (1 - D1*z^-1)
    // High-pass filter coefficients for 20Hz cutoff @ 48kHz sample rate:
    // N0 (C36) = 0x7FC000 (+0.9974)
    // N1 (C37) = 0x804000 (-0.9974, 2's complement)
    // D1 (C39) = 0x7F8000 (+0.9948)
    // ============================================================================

    // ENABLED: High-pass filter to remove DC offset from PDM microphone
    // 20Hz cutoff @ 48kHz sample rate removes DC while preserving voice (>80Hz)
    uint32_t hpfCoeff[3];
    hpfCoeff[0] = 0x7FC00000;  // N0 = 0x7FC000 (shifted left 8 bits)
    hpfCoeff[1] = 0x80400000;  // N1 = 0x804000 (shifted left 8 bits)
    hpfCoeff[2] = 0x7F800000;  // D1 = 0x7F8000 (shifted left 8 bits)
    setHpfIIRCoeffOnADC(Right_Chan, hpfCoeff);

    if (debugToSerial) {
        Serial.println("ESP32AIC3212: HPF ENABLED - 20Hz high-pass removes DC offset");
    }

    // NOTE: P1_R8 common mode voltage is already configured by aic_init()
    // User has it set to 0x1B (Input CM=0.9V for 1.8V AVDD, Output CM=1.65V)
    // Do NOT modify P1_R8 here as it will break the correct configuration

    // ============================================================================
    // CRITICAL: Bypass Analog PGA - Route Digital Microphone Directly to ADC Modulator
    // SLAU360 B0_P1_R61 Bit D2: Route RIGHT ADC Modulator input directly from pin
    // This bypasses the entire analog PGA path, preventing analog circuitry from
    // processing the PDM bitstream which causes decimation filter saturation.
    //
    // ROOT CAUSE OF OVERFLOW: Without this, PDM bitstream flows through:
    //   1. Analog input pins (treat PDM as analog voltage swings)
    //   2. Analog PGA amplifier (processes PDM transitions as "analog signal")
    //   3. ADC delta-sigma modulator (receives already-digital PDM as "analog" input)
    //   4. CIC decimation filter accumulator integrates PDM → OVERFLOW
    //
    // This is why digital volume control doesn't prevent overflow - it's applied
    // AFTER the decimation filter where overflow occurs.
    // ============================================================================

    // Read current PTM setting (bits 7:6) configured by aic_init()
    uint8_t p1_r61_current = aic_readPage(1, 0x3D);
    uint8_t ptm_bits = p1_r61_current & 0xC0;  // Preserve PTM_R setting (bits 7:6)

    // Set bit D2=1 to route RIGHT ADC Modulator input directly from DIN2 pin
    // This bypasses the analog PGA completely for digital microphone mode
    aic_writePage(1, 0x3D, ptm_bits | 0x04);  // PTM setting + bit D2 set

    if (debugToSerial) {
        Serial.println("ESP32AIC3212: Configured P1_R61 to bypass analog PGA for digital mic");
        Serial.printf("  Previous value: 0x%02X, New value: 0x%02X\n", p1_r61_current, ptm_bits | 0x04);
    }

    // ============================================================================
    // Disable Analog Input Routing to RIGHT PGA
    // Even though we've bypassed the PGA with P1_R61_D2, explicitly disable
    // analog input routing to ensure no analog signals interfere with digital path
    // ============================================================================

    aic_writePage(1, 0x37, 0x00);  // P1_R55: Disable all inputs to RIGHT PGA P-terminal
    aic_writePage(1, 0x39, 0x00);  // P1_R57: Disable all inputs to RIGHT PGA M-terminal
    aic_writePage(1, 0x3C, 0x80);  // P1_R60: RIGHT PGA gain disabled (muted)

    // Power down mixer amplifiers (not used for digital microphone)
    aic_writePage(1, 0x11, 0x00);  // P1_R17: Power down MAL and MAR

    if (debugToSerial) {
        Serial.println("ESP32AIC3212: Disabled analog input routing and powered down mixers");
    }

    // NOTE: For PDM microphones, we DON'T use ASI2 BCLK/WCLK generation
    // Instead, we output ADC_MOD_CLK directly from the BCLK2 pin
    // ADC_MOD_CLK = 1.536 MHz (calculated above with MADC=2)

    // P4 R70: BCLK2 Pin Control - Configure BCLK2 to output ADC_MOD_CLK for PDM mic
    // Bits [5:3] = 101 (0x28 >> 3 = 5) outputs ADC_MOD_CLK as PDM_CLK
    // This outputs 1.536 MHz clock to the CMM-3424DT microphone (within 1.0-4.8 MHz spec)
    aic_writePage(4, 0x46, 0x28);  // BCLK2 pin = ADC_MOD_CLK output @ 1.536 MHz

    // P4 R72: DIN2 Pin Control - Enable DIN2 as digital audio input
    aic_writePage(4, 0x48, 0x20);  // Enable DIN2 (bit 5 = 1)

    // P4 R101: Digital Microphone Input Configuration
    // NOTE: Reverting to original 0x03 - P0_R81 bit 3 controls ADC edge detection
    // P4_R101 may only affect ASI2 routing, not ADC sampling
    // 0x03 = 0b00000011 - bits[3:2]=00, bits[1:0]=11
    aic_writePage(4, 0x65, 0x03);  // DIN2: Original configuration

    // ============================================================================
    // CRITICAL: P4_R7 must be set to route ADC data to ASI1 DOUT
    // P4_R7 bits [2:0] control ASI1 data source:
    //   000: ASI1 output disabled (tri-stated)
    //   001: ASI1 sourced from ADC Data Output ← NEEDED FOR RECORDING
    //   010: ASI1-to-ASI1 loopback (set by aic_init for playback)
    //   011: ASI2-to-ASI1 loopback
    //
    // aic_init() sets P4_R7 = 0x02 for playback (ASI1→ASI1→DAC).
    // For recording, we need 0x01 to route ADC→ASI1 DOUT.
    // Without this, ADC FIFO overflows (P0_R42 = 0x0E) because data has nowhere to go.
    // ============================================================================
    aic_writePage(4, 0x07, 0x01);  // P4_R7: ASI1 sourced from ADC output

    // ============================================================================
    // CRITICAL: Enable ADC and DAC Clock Domain Synchronization with ASI1
    // P4_R119 bits [7:6] control DAC engine synchronization:
    //   00: Enable DAC syncing with ASI1
    //   01: Enable DAC syncing with ASI2
    //   10: Enable DAC syncing with ASI3
    //   11: Disable DAC syncing
    // P4_R119 bits [5:4] control ADC engine synchronization:
    //   00: Enable ADC syncing with ASI1 (REQUIRED for ADC→ASI1 routing)
    //   01: Enable ADC syncing with ASI2
    //   10: Enable ADC syncing with ASI3
    //   11: Disable ADC syncing (default - causes overflow!)
    //
    // CRITICAL: Digital microphone PDM decimation filter requires frame sync from ASI1
    // Expert analysis: P4_R119 bits[5:4]=11 (internal timing) causes overflow with
    // digital mic mode because PDM decimation FIFO fills faster than ASI1 reads it.
    // Digital mic mode specifically needs ASI1 synchronization for proper timing.
    // ============================================================================
    aic_writePage(4, 0x77, 0x00);  // P4_R119: Both DAC and ADC sync with ASI1
                                    // 0x00 = bits [7:6]=00 (DAC syncs with ASI1)
                                    //        bits [5:4]=00 (ADC syncs with ASI1)

    // P0 R81: ADC Channel Power Control
    // Bit D6 = 1: RIGHT ADC powered up
    // Bits D5-D4 = 00: LEFT digital mic mode DISABLED (no mic on LEFT channel)
    // Bits D3-D2 = 01: RIGHT digital mic mode ENABLED (mic on RIGHT channel)
    // Expert: Only enable digital mic mode on channel that has microphone connected
    // With PRB_R7, MADC=12, AOSR=64: ADC_MOD_CLK = 3.072 MHz (optimal for CMM-3424DT)
    aic_writePage(0, 0x51, 0x44);  // 0b01000100 - RIGHT ADC power + RIGHT digital mic only

    // P0 R82: ADC Fine Gain - Mute LEFT (unused), Unmute RIGHT (valid data)
    // Bit 7 = LEFT mute, Bit 3 = RIGHT mute
    aic_writePage(0, 0x52, 0x80);  // Mute LEFT (0x80), Unmute RIGHT (0x00)

    // P0 R83: Left ADC Volume Control - muted anyway, set to 0dB
    aic_writePage(0, 0x53, 0x00);  // 0dB (LEFT channel not used)

    // P0 R84: Right ADC Volume Control
    // CRITICAL: PDM microphones output -26dBFS at 94dB SPL (normal speech level)
    // At 0dB digital gain, output is only ±550-±1900 counts (1.7-5.8% of full scale)
    // This is NORMAL and expected. We need to ADD digital gain for proper output level.
    // Range per datasheet (slau360.txt): -12dB to +20dB in 0.5dB steps
    //   0x00 = 0dB (default)
    //   0x18 = +12dB (24 steps × 0.5dB)
    //   0x28 = +20dB (40 steps × 0.5dB, maximum gain)
    //   0x68 = -12dB (maximum attenuation)
    // Start with +20dB (maximum gain) to bring -26dBFS mic output to -6dBFS
    aic_writePage(0, 0x54, 0x28);  // digital VOLUME

    delay(10);  // Allow configuration to settle

    // Explicitly clear ADC overflow flags by writing to P0_R42
    // Bit 2 = RIGHT ADC overflow (write 1 to clear)
    uint8_t overflow_flags = aic_readPage(0, 0x42);  // Read current flags
    if (overflow_flags != 0) {
        aic_writePage(0, 0x42, overflow_flags);  // Write back to clear
        if (debugToSerial) {
            Serial.printf("ESP32AIC3212: Cleared ADC overflow flags: 0x%02X\n", overflow_flags);
        }
        delay(5);
    }

    // Verify critical registers
    uint8_t p0_r4 = aic_readPage(0, 4);
    uint8_t p0_r18 = aic_readPage(0, 0x12);  // NADC
    uint8_t p0_r19 = aic_readPage(0, 0x13);  // MADC
    uint8_t p0_r20 = aic_readPage(0, 0x14);  // AOSR
    uint8_t p0_r51 = aic_readPage(0, 0x51);  // ADC Power
    uint8_t p0_r61 = aic_readPage(0, 0x3D);  // PRB_R
    uint8_t p0_r84 = aic_readPage(0, 0x54);  // RIGHT ADC Volume
    uint8_t p4_r1 = aic_readPage(4, 0x01);   // ASI1 Format
    uint8_t p4_r7 = aic_readPage(4, 0x07);   // ASI1→DAC routing
    uint8_t p4_r10 = aic_readPage(4, 0x0A);  // ASI1 Clock direction
    uint8_t p4_r70 = aic_readPage(4, 0x46);  // BCLK2
    uint8_t p4_r72 = aic_readPage(4, 0x48);  // DIN2
    uint8_t p4_r101 = aic_readPage(4, 0x65); // Digital Mic Config
    uint8_t p4_r119 = aic_readPage(4, 0x77); // ADC/DAC Sync

    if (debugToSerial) {
        Serial.println("\n=== Digital Microphone Register Verification ===");
        Serial.println("--- Clock Configuration (PLL-Based for PRB_R7 with Filter B) ---");
        Serial.printf("  P0_R4 (Clock Source): 0x%02X (expected 0x33 - DAC:PLL_CLK, ADC:PLL_CLK)\n", p0_r4);
        Serial.printf("  P0_R18 (NADC): 0x%02X (expected 0x81 - Enabled, divider=1)\n", p0_r18);
        Serial.printf("  P0_R19 (MADC): 0x%02X (expected 0x8C - Enabled, divider=12)\n", p0_r19);
        Serial.printf("  P0_R20 (AOSR): 0x%02X (expected 0x40 - Oversampling=64)\n", p0_r20);

        // Calculate and display actual clock frequencies
        uint8_t nadc_val = p0_r18 & 0x7F;
        uint8_t madc_val = p0_r19 & 0x7F;
        uint8_t aosr_val = p0_r20;
        float pll_clk = 36.864;  // MHz (from PLL: 3.072 * 12)
        float adc_mod_clk = pll_clk / (nadc_val * madc_val);
        float adc_fs = pll_clk * 1000000.0 / (nadc_val * madc_val * aosr_val);
        float rc_check = (madc_val * aosr_val) / 32.0;

        Serial.printf("  PLL_CLK = %.3f MHz (from BCLK1 3.072MHz * 12)\n", pll_clk);
        Serial.printf("  ADC_MOD_CLK = %.3f MHz (optimal for CMM-3424DT: 1.0-4.8 MHz)\n", adc_mod_clk);
        Serial.printf("  ADC Sample Rate = %.0f Hz\n", adc_fs);
        Serial.printf("  RC Constraint Check: MADC*AOSR/32 = %d*%d/32 = %.1f (PRB_R7 needs ≥23)\n",
                      madc_val, aosr_val, rc_check);

        Serial.println("\n--- ASI1 Configuration ---");
        Serial.printf("  P4_R1 (ASI1 Format): 0x%02X (should be 0x00 for 16-bit I2S)\n", p4_r1);
        Serial.printf("  P4_R10 (ASI1 Clock): 0x%02X (should be 0x00 - Slave mode)\n", p4_r10);
        Serial.printf("  P4_R7 (ASI1 Source): 0x%02X (expected 0x01 - ADC→ASI1 DOUT)\n", p4_r7);
        Serial.printf("  P4_R119 (ADC/DAC Sync): 0x%02X (expected 0x00 - ADC and DAC sync ASI1)\n", p4_r119);

        Serial.println("\n--- ADC Configuration ---");
        Serial.printf("  P0_R61 (PRB_R): 0x%02X (expected 0x07 - PRB_R7 Decimation Filter B)\n", p0_r61);
        Serial.printf("  P0_R81 (ADC Power): 0x%02X (expected 0x44 - RIGHT ADC + RIGHT digital mic only)\n", p0_r51);

        Serial.println("\n--- PDM Microphone Configuration ---");
        Serial.printf("  P4_R70 (BCLK2 Pin): 0x%02X (wrote 0x28 for ADC_MOD_CLK output @ 3.072MHz)\n", p4_r70);
        Serial.printf("  P4_R72 (DIN2 Pin): 0x%02X (wrote 0x20 to enable DIN2, extra bits are status)\n", p4_r72);
        Serial.printf("  P4_R101 (Digital Mic): 0x%02X (expected 0x03 - RIGHT on DIN2 falling edge)\n", p4_r101);
        Serial.printf("  P0_R84 (RIGHT Vol): 0x%02X (expected 0x28 - +20dB maximum gain)\n", p0_r84);

        // Verify analog PGA bypass configuration (CRITICAL for preventing overflow)
        uint8_t p1_r61 = aic_readPage(1, 0x3D);
        uint8_t p1_r55 = aic_readPage(1, 0x37);
        uint8_t p1_r57 = aic_readPage(1, 0x39);
        uint8_t p1_r60 = aic_readPage(1, 0x3C);

        Serial.println("\n--- Analog PGA Bypass Configuration (Critical for Overflow Prevention) ---");
        Serial.printf("  P1_R61 (ADC PTM): 0x%02X (bit 2=%d, MUST be 1 for RIGHT bypass)\n",
                      p1_r61, (p1_r61 >> 2) & 0x01);
        Serial.printf("  P1_R55 (RIGHT PGA P-inputs): 0x%02X (should be 0x00 - all disabled)\n", p1_r55);
        Serial.printf("  P1_R57 (RIGHT PGA M-inputs): 0x%02X (should be 0x00 - all disabled)\n", p1_r57);
        Serial.printf("  P1_R60 (RIGHT PGA Control): 0x%02X (bit 7=%d, should be 1 for disabled)\n",
                      p1_r60, (p1_r60 >> 7) & 0x01);

        // Calculate and display ADC_CLK / Sample Rate / PDM Clock
        uint8_t nadc = p0_r18 & 0x7F;
        uint8_t madc = p0_r19 & 0x7F;
        uint8_t aosr = p0_r20;

        // Determine ADC clock source from P0_R4
        uint8_t adc_clk_src = p0_r4 & 0x0F;
        float adc_clkin_mhz;
        const char* clk_src_name;

        if (adc_clk_src == 0x03) {  // PLL_CLK
            adc_clkin_mhz = 19.968f;  // PLL output: 3.072 MHz * 6.5
            clk_src_name = "PLL_CLK";
        } else if (adc_clk_src == 0x01) {  // BCLK1
            adc_clkin_mhz = 3.072f;
            clk_src_name = "BCLK1";
        } else {
            adc_clkin_mhz = 0.0f;
            clk_src_name = "UNKNOWN";
        }

        float adc_mod_clk_mhz = adc_clkin_mhz / (nadc * madc);  // ADC_CLKIN / (NADC * MADC)
        float sample_rate_khz = (adc_clkin_mhz * 1000.0f) / (nadc * madc * aosr);

        Serial.printf("\n--- Calculated Frequencies ---");
        Serial.printf("\n  ADC_CLKIN: %.3f MHz (from %s)\n", adc_clkin_mhz, clk_src_name);
        Serial.printf("  ADC_MOD_CLK (PDM_CLK): %.3f MHz (ADC_CLKIN / (NADC=%d * MADC=%d))\n", adc_mod_clk_mhz, nadc, madc);
        Serial.printf("  ADC Sample Rate: %.1f kHz (%.3fMHz / (NADC=%d * MADC=%d * AOSR=%d))\n",
                      sample_rate_khz, adc_clkin_mhz, nadc, madc, aosr);
        Serial.printf("  CMM-3424DT-26165-TR valid ranges: 0.35-0.8 MHz, 1.0-2.475 MHz, or 3.0-4.8 MHz\n");

        bool in_spec = (adc_mod_clk_mhz >= 0.35 && adc_mod_clk_mhz <= 0.8) ||
                       (adc_mod_clk_mhz >= 1.0 && adc_mod_clk_mhz <= 2.475) ||
                       (adc_mod_clk_mhz >= 3.0 && adc_mod_clk_mhz <= 4.8);

        if (!in_spec) {
            Serial.printf("  WARNING: PDM clock %.3f MHz is OUT OF SPEC!\n", adc_mod_clk_mhz);
        } else {
            Serial.printf("  PDM clock is within spec ✓\n");
        }
        Serial.printf("  IMPORTANT: If sample rate != 48.0 kHz, check MADC value!\n");
        Serial.println("\n=== Configuration Complete ===");
    }

    return true;
}

// ============================================================================
// Digital Microphone Diagnostic
// Comprehensive diagnostic output for debugging digital microphone setup
// ============================================================================
void ESP32AIC3212::diagnosticDigitalMic(void) {
    Serial.println("\n========== DIGITAL MICROPHONE DIAGNOSTIC ==========");

    // === Clock Configuration ===
    Serial.println("\n--- Clock Configuration ---");
    Serial.printf("P0_R4 (Clock Src): 0x%02X\n", aic_readPage(0, 4));
    Serial.printf("P0_R11 (NDAC): 0x%02X\n", aic_readPage(0, 0x0B));
    Serial.printf("P0_R12 (MDAC): 0x%02X\n", aic_readPage(0, 0x0C));
    Serial.printf("P0_R18 (NADC): 0x%02X\n", aic_readPage(0, 0x12));
    Serial.printf("P0_R19 (MADC): 0x%02X\n", aic_readPage(0, 0x13));
    Serial.printf("P0_R20 (AOSR): 0x%02X\n", aic_readPage(0, 0x14));

    // === ASI2 Clock Generation ===
    Serial.println("\n--- ASI2 Clock Generation ---");
    Serial.printf("P4_R39 (ASI2 BCLK Src): 0x%02X\n", aic_readPage(4, 0x27));
    Serial.printf("P4_R40 (ASI2 BCLK Div): 0x%02X\n", aic_readPage(4, 0x28));
    Serial.printf("P4_R42 (ASI2 WCLK Div): 0x%02X\n", aic_readPage(4, 0x2A));
    Serial.printf("P4_R26 (ASI2 CLK Out): 0x%02X\n", aic_readPage(4, 0x1A));

    // === ASI1 Configuration ===
    Serial.println("\n--- ASI1 (Output to MCU) Configuration ---");
    Serial.printf("P4_R1 (ASI1 Format): 0x%02X\n", aic_readPage(4, 0x01));
    Serial.printf("P4_R2 (ASI1 Offset): 0x%02X\n", aic_readPage(4, 0x02));
    Serial.printf("P4_R7 (ASI1 Input): 0x%02X\n", aic_readPage(4, 0x07));
    Serial.printf("P4_R8 (ASI1->DAC): 0x%02X\n", aic_readPage(4, 0x08));
    Serial.printf("P4_R10 (ASI1 CLK): 0x%02X\n", aic_readPage(4, 0x0A));
    Serial.printf("P4_R119 (ADC/DAC Sync): 0x%02X\n", aic_readPage(4, 0x77));

    // === ASI2 Configuration ===
    Serial.println("\n--- ASI2 (Input from Mic) Configuration ---");
    Serial.printf("P4_R23 (ASI2 Format): 0x%02X\n", aic_readPage(4, 0x17));
    Serial.printf("P4_R24 (ASI2 Offset): 0x%02X\n", aic_readPage(4, 0x18));

    // === Pin Configuration ===
    Serial.println("\n--- Pin Configuration ---");
    Serial.printf("P4_R68 (DIN1): 0x%02X\n", aic_readPage(4, 0x44));
    Serial.printf("P4_R70 (BCLK2): 0x%02X\n", aic_readPage(4, 0x46));
    Serial.printf("P4_R72 (DIN2): 0x%02X\n", aic_readPage(4, 0x48));
    Serial.printf("P4_R101 (DigMic): 0x%02X\n", aic_readPage(4, 0x65));

    // === ADC Configuration ===
    Serial.println("\n--- ADC Configuration ---");
    Serial.printf("P0_R60 (ADC PRB): 0x%02X\n", aic_readPage(0, 0x3C));  // Should show PRB
    Serial.printf("P0_R61 (ADC PRB): 0x%02X\n", aic_readPage(0, 0x3D));
    Serial.printf("P0_R81 (ADC Power): 0x%02X\n", aic_readPage(0, 0x51));
    Serial.printf("P0_R82 (ADC Mute): 0x%02X\n", aic_readPage(0, 0x52));
    Serial.printf("P0_R83 (ADC Vol L): 0x%02X\n", aic_readPage(0, 0x53));
    Serial.printf("P0_R84 (ADC Vol R): 0x%02X\n", aic_readPage(0, 0x54));

    // === ADC Flag Registers (Status) ===
    Serial.println("\n--- ADC Status Flags ---");
    Serial.printf("P0_R36 (ADC Flag): 0x%02X\n", aic_readPage(0, 0x24));
    Serial.printf("P0_R37 (ADC Flag): 0x%02X\n", aic_readPage(0, 0x25));
    Serial.printf("P0_R42 (ADC Flag): 0x%02X\n", aic_readPage(0, 0x2A));

    Serial.println("\n===================================================\n");
}

// ============================================================================
// Set codec to recording mode (ADC→ASI1)
// P4_R7 = 0x01: Route ADC data to ASI1 DOUT for recording
// This must be called before starting audio recording
// ============================================================================
bool ESP32AIC3212::setRecordingMode(void) {
    if (debugToSerial) Serial.println("ESP32AIC3212: Setting recording mode (ADC→ASI1)");
    return aic_writePage(4, 0x07, 0x01);  // P4_R7: ASI1 sourced from ADC output
}

// ============================================================================
// Set codec to playback mode (ASI1-to-ASI1 loopback)
// P4_R7 = 0x02: ASI1-to-ASI1 loopback, routes I2S→DAC→Speaker
// This must be called before starting audio playback
// ============================================================================
bool ESP32AIC3212::setPlaybackMode(void) {
    if (debugToSerial) Serial.println("ESP32AIC3212: Setting playback mode (ASI1→DAC)");
    return aic_writePage(4, 0x07, 0x02);  // P4_R7: ASI1-to-ASI1 loopback
}

} // namespace esp32aic3212
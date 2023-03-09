#include <Arduino.h>
#include <driver/i2s.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <arduinoFFT.h>

#define SAMPLES         512          // Must be a power of 2
#define SAMPLING_FREQ   40000         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define NOISE           1000           // Used as a crude noise filter, values below this are ignored
#define BANDFALLOFF     1
#define MAXPEAKGRAVITY  0.7

int16_t samples[SAMPLES];
double vReal[SAMPLES];
double vImag[SAMPLES];

int bandValues[16];
uint8_t barValues[16];
float peakGravity[16];
float peakValues[16];

arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);
U8G2_MAX7219_64X8_F_4W_SW_SPI u8g2(U8G2_R2, 18, 23, 5, U8X8_PIN_NONE, U8X8_PIN_NONE);

void setup() {
    // setup i2s
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLING_FREQ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Interrupt level 1, default 0
        .dma_buf_count = 4,
        .dma_buf_len = SAMPLES,
        .use_apll = false,
    };
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_0db);
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);

    for (int i = 0; i<16; i++)
        peakValues[i] = 0;

    u8g2.begin();
    u8g2.setContrast(0);
    u8g2.firstPage();
}

void loop() {

    // read bytes
    size_t bytes_read;
    i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    for (uint16_t i = 0; i < SAMPLES; i++) {
        vReal[i] = samples[i];
        vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
    }

    // Compute FFT
    FFT.DCRemoval();
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(FFT_FORWARD);
    FFT.ComplexToMagnitude();

    // Reset bandValues[]
    for (int i = 0; i<16; i++)
        bandValues[i] = 0;

    // Analyse FFT results
    for (int i = 2; i < (SAMPLES/2); i++){       // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
        if (vReal[i] > NOISE) {                    // Add a crude noise filter

        /*8 bands, 12kHz top band
          if (i<=3 )           bandValues[0]  += (int)vReal[i];
          if (i>3   && i<=6  ) bandValues[1]  += (int)vReal[i];
          if (i>6   && i<=13 ) bandValues[2]  += (int)vReal[i];
          if (i>13  && i<=27 ) bandValues[3]  += (int)vReal[i];
          if (i>27  && i<=55 ) bandValues[4]  += (int)vReal[i];
          if (i>55  && i<=112) bandValues[5]  += (int)vReal[i];
          if (i>112 && i<=229) bandValues[6]  += (int)vReal[i];
          if (i>229          ) bandValues[7]  += (int)vReal[i];*/

        //16 bands, 12kHz top band
          if (i<=2 )           bandValues[0]  += (int)vReal[i];
          if (i>2   && i<=3  ) bandValues[1]  += (int)vReal[i];
          if (i>3   && i<=5  ) bandValues[2]  += (int)vReal[i];
          if (i>5   && i<=7  ) bandValues[3]  += (int)vReal[i];
          if (i>7   && i<=9  ) bandValues[4]  += (int)vReal[i];
          if (i>9   && i<=13 ) bandValues[5]  += (int)vReal[i];
          if (i>13  && i<=18 ) bandValues[6]  += (int)vReal[i];
          if (i>18  && i<=25 ) bandValues[7]  += (int)vReal[i];
          if (i>25  && i<=36 ) bandValues[8]  += (int)vReal[i];
          if (i>36  && i<=50 ) bandValues[9]  += (int)vReal[i];
          if (i>50  && i<=69 ) bandValues[10] += (int)vReal[i];
          if (i>69  && i<=97 ) bandValues[11] += (int)vReal[i];
          if (i>97  && i<=135) bandValues[12] += (int)vReal[i];
          if (i>135 && i<=189) bandValues[13] += (int)vReal[i];
          if (i>189 && i<=264) bandValues[14] += (int)vReal[i];
          if (i>264          ) bandValues[15] += (int)vReal[i];
        }
    }

    // draw to display
    u8g2.firstPage();
    for (int i = 0; i < 16; i++) {
        // calculate bar heights
        uint8_t newValue = map(bandValues[i], 0, 20000, 0, 16);
        barValues[i] = max(newValue > 16 ? 16 : newValue, barValues[i] - BANDFALLOFF);

        // calculate peak positions
        if (newValue > peakValues[i]) {
            peakGravity[i] = 0.1;
            peakValues[i] = barValues[i];
        } else {
            peakGravity[i] += peakGravity[i] <= MAXPEAKGRAVITY ? 0.025 : 0;
            peakValues[i] = peakValues <= 0 ? 0 :peakValues[i] - peakGravity[i];
        }
        

        // draw bars
        uint8_t topValue = barValues[i] > 7 ? barValues[i] - 8 : 0;
        uint8_t bottomValue = barValues[i] > 8 ? 8 : barValues[i];
        u8g2.drawBox(i*2,       8 - topValue,       2, topValue);
        u8g2.drawBox(32+i*2,    8 - bottomValue,    2, bottomValue);

        // draw peaks
        uint8_t peakX = (peakValues[i] > 8 ? 0 : 32) + i*2;
        uint8_t peakY = 7 - ((uint8_t)(peakValues[i]) % 8);
        u8g2.drawHLine(peakX, peakY, 2);

        //printf("%10d", bandValues[i]);
    }
    u8g2.nextPage();
    //printf("\n");
}
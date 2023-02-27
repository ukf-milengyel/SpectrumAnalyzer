#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <arduinoFFT.h>

#define SAMPLES         1024          // Must be a power of 2
#define SAMPLING_FREQ   40000         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE       1000          // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define AUDIO_IN_PIN    36            // Signal in on this pin
#define MAX_MILLIAMPS   1000          // Careful with the amount of power here if running off USB port
#define NOISE           1500           // Used as a crude noise filter, values below this are ignored

// Sampling and FFT stuff
unsigned int sampling_period_us;
byte peak[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};              // The length of these arrays must be >= NUM_BANDS
int oldBarHeights[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int bandValues[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

U8G2_MAX7219_32X8_F_4W_SW_SPI u8g2(U8G2_R2, 18, 23, 5, U8X8_PIN_NONE, U8X8_PIN_NONE);

void setup() {
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
  u8g2.begin();
  u8g2.setContrast(0);
  u8g2.firstPage();
}

void loop() {

    for (int i = 0; i < SAMPLES; i++) {
        newTime = micros();
        vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 9.7uS on an ESP32
        vImag[i] = 0;
        while ((micros() - newTime) < sampling_period_us);
    }

    // Compute FFT
    FFT.DCRemoval();
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(FFT_FORWARD);
    FFT.ComplexToMagnitude();

    // Reset bandValues[]
    for (int i = 0; i<16; i++){
      bandValues[i] = 0;
    }

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

    u8g2.clear();
    for (int i = 0; i < 16; i++) {
        //printf("%6d", bandValues[i]);
        u8g2.drawBox(i*2, 0, 2, map(bandValues[i], 1500, 20000, 0, 8));
    }
    u8g2.nextPage();
    //printf("\n");

    // brightness test
    /*
    u8g2.firstPage();
    for(int i = 0; i < 255; i++){
        u8g2.setContrast(i);
        u8g2.drawBox(0,4,i/7,4);
        u8g2.drawBox(0,0,32,4);
        u8g2.nextPage();
        delay(20);
    }
    */

    // display text
    /*
    do {
      u8g2.setFont(u8g2_font_tiny_simon_tr);
      //u8g2.drawStr(0,5,"Ayo diego");
      u8g2.drawBox(0,0,32,8);
      delay(1000);
    } while ( u8g2.nextPage() );
    delay(1000);
    */
}
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>

U8G2_MAX7219_32X8_F_4W_SW_SPI u8g2(U8G2_R2, 32, 33, 25, U8X8_PIN_NONE, U8X8_PIN_NONE);

void setup() {
  u8g2.begin();
  u8g2.setContrast(0);
}

void loop() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_tiny_simon_tr);
    u8g2.drawStr(0,5,"Ayo Diego");
    delay(1000);
  } while ( u8g2.nextPage() );
  delay(1000);
}

static const unsigned char PROGMEM BT_bmp[] = {
 0b00000001,0b00000000,
 0b00000001,0b10000000,
 0b00100001,0b01000000,
 0b00010001,0b00100000,
 0b00001001,0b00010000,
 0b00000101,0b00100000,
 0b00000011,0b01000000,
 0b00000001,0b10000000,
 0b00000001,0b10000000,
 0b00000011,0b01000000,
 0b00000101,0b00100000,
 0b00001001,0b00010000,
 0b00010001,0b00100000,
 0b00100001,0b01000000,
 0b00000001,0b10000000,
 0b00000001,0b00000000 
};

uint32_t last_status = now;
String last_status_str = "starting up";

void OledStatus() { OledStatus(""); }
void OledStatus(String text) { OledStatus(text, 1000); }
void OledStatus(String text, uint16_t status_ttl) {
  display.clearDisplay();

  if (device_name.length() > 0) {
    int size = 2;
    display.setTextSize(size);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    
    display.setCursor(0,0);
    
    display.print(device_name.substring(0,int((SCREEN_WIDTH-BMP_WIDTH)/OLED_CHAR_SIZE)));
  }
  if (text.length() > 0) {
    last_status = now + status_ttl;
    last_status_str = text;
  }
  // display statusi for a second
  if (last_status_str.length() > 0 && now < last_status) {
    display.setTextSize(1);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // draw status messages in while on black text
    display.setCursor(0, SCREEN_HEIGHT - 8);              // Start at bottom-left corner
    display.print(last_status_str);
  }
  else if (now >= last_status) {
    last_status_str = "";
  }
  if (connected) {
    display.drawBitmap(SCREEN_WIDTH-BMP_WIDTH,0,BT_bmp, BMP_WIDTH, BMP_HEIGHT, 1);
  }
  else {
    int size = 2;
    display.setTextSize(size);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    String disconnected = "--";
    display.setCursor(SCREEN_WIDTH - (OLED_CHAR_SIZE * size * disconnected.length()),0);
    display.print(disconnected);
  }
  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // draw status messages in while on black text
  display.setCursor(SCREEN_WIDTH-8, SCREEN_HEIGHT - 8); 
  display.print(mode);

  display.display();
  delay(2);
}



void oledStatus() { oledStatus(""); }
void oledStatus(String text) { oledStatus(text, 1000); }
void oledStatus(String text, uint16_t status_ttl) {
  display.clearDisplay();

  if (device_name.length() > 0) {
    display.setTextSize(1);              // Normal 1:1 pixel scale
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
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // white on black text
    display.setCursor(0, SCREEN_HEIGHT - 8);              // bottom-left corner
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
    display.setTextSize(size); 
    display.setTextColor(SSD1306_WHITE);
    String disconnected = "--";
    display.setCursor(SCREEN_WIDTH - (OLED_CHAR_SIZE * size * disconnected.length()),0);
    display.print(disconnected);
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); 
  for (int i = 0; i<4; i++) {
    String str = oled_menu[i];
    if (!str || str.length() == 0) {
      continue;
    }
    str = str.substring(0,MENU_MAX_CHARS+1);
    switch(i) {
      case 0: // RIGHT
        display.setCursor(SCREEN_WIDTH*.5 + (OLED_CHAR_SIZE), OLED_CHAR_SIZE*4);
        break;
      case 1: // DOWN
        display.setCursor(SCREEN_WIDTH*.5 - (int)(str.length()/2)*OLED_CHAR_SIZE, OLED_CHAR_SIZE*5);                    
        break;
      case 2: // LEFT
        display.setCursor(SCREEN_WIDTH*.5 - (OLED_CHAR_SIZE) - (str.length()*OLED_CHAR_SIZE), OLED_CHAR_SIZE*4);   
        break;
      case 3: // UP
        display.setCursor(SCREEN_WIDTH*.5 - (int)(str.length()/2)*OLED_CHAR_SIZE, OLED_CHAR_SIZE*3);   
        break;
    }
    display.print(str);
  }
  
  display.setTextSize(1);              
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
  display.setCursor(SCREEN_WIDTH-8, SCREEN_HEIGHT - 8); 
  display.print(mode);

  display.display();
  delay(2);
}

void clearOledMenu() {
  oled_menu[0] = "";
  oled_menu[1] = "";
  oled_menu[2] = "";
  oled_menu[3] = "";
}

void updateOledMenu(BLEDevice central, BLECharacteristic menuChar) {
  if (menuChar.canRead()) {
    // read the characteristic value
    menuChar.read();
    if (menuChar.valueLength() > 0) {
      // print out the value of the characteristic
      int oled_menu_item = 0;
      oled_menu[oled_menu_item] = "";
      for (int i=0; i<menuChar.valueLength();i++) {
        char c = (menuChar.value())[i];
        if (c == MENU_DELIM) {
          oled_menu_item++; 
          oled_menu[oled_menu_item] = "";
          continue;
        }
        oled_menu[oled_menu_item] +=  c;
      }
    }
  }
}
